#include "synshared.h"
#include "config_socket.h"
#include "synclient_loader.h"
#include "xi_properties.h"
#include "unique_fd.h"
#include "log.h"
#include <glib.h>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cerrno>
#include <string>
#include <functional>
#include <vector>
#include <algorithm>
#include <memory>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <signal.h>

static constexpr int MAX_CLIENTS = 32;
static constexpr size_t MAX_CLIENT_BUF = 4096;
static constexpr int CLIENT_SNDBUF_SIZE = 16384;

struct ClientContext;

struct SocketContext {
    UniqueFd listen_fd;
    std::string config_path;
    std::string runtime_config_path;
    DeviceIntPtr dev = nullptr;  // raw — driver-owned
    std::vector<ClientContext *> touch_subscribers;
    int num_clients = 0;
};

struct ClientContext {
    UniqueFd fd;
    std::string buf;
    SocketContext *srv = nullptr;  // non-owning back-pointer
    bool subscribed_touches = false;
};

static SocketContext g_socket_ctx;
static bool g_socket_active = false;
static WaynapticsTouchState g_last_touch_state = {};

// Printf-style reply over socket (kept variadic for format convenience)
static void sock_reply(int fd, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
static void sock_reply(int fd, const char *fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n > 0) {
        size_t len = static_cast<size_t>(std::min(n, static_cast<int>(sizeof(buf)) - 1));
        send(fd, buf, len, MSG_NOSIGNAL | MSG_DONTWAIT);
    }
}

static void emit_to_fd(const std::string &line, int fd) {
    send(fd, line.data(), line.size(), MSG_NOSIGNAL | MSG_DONTWAIT);
    send(fd, "\n", 1, MSG_NOSIGNAL | MSG_DONTWAIT);
}

static void emit_to_file(const std::string &line, FILE *f) {
    fprintf(f, "%s\n", line.c_str());
}

static void handle_command(ClientContext *client, const std::string &cmd) {
    int cfd = client->fd.get();
    wlog("socket", "command: %s", cmd.c_str());

    if (cmd == "get_capabilities") {
        Atom cap_atom = XIGetKnownProperty("Synaptics Capabilities");
        if (cap_atom == 0) {
            sock_reply(cfd, "FAIL property not registered\n");
            wlog("socket", "get_capabilities: property not registered");
            return;
        }
        auto val = waynaptics_get_property_value(cap_atom);
        if (!val || val->size < 7) {
            sock_reply(cfd, "FAIL property has no data\n");
            wlog("socket", "get_capabilities: no data");
            return;
        }
        static const char *cap_names[] = {
            "HasLeftButton", "HasMiddleButton", "HasRightButton",
            "TwoFingerDetection", "ThreeFingerDetection",
            "PressureDetection", "FingerOrPalmWidthDetection"
        };
        for (int i = 0; i < 7; i++)
            sock_reply(cfd, "    %-24s = %d\n", cap_names[i], val->read<uint8_t>(i));
        client->fd.reset();
        wlog("socket", "get_capabilities: dumped and closed");
        return;
    }

    if (cmd == "get_config") {
        int fd_val = client->fd.get();
        waynaptics_dump_config([fd_val](const std::string &line) { emit_to_fd(line, fd_val); });
        client->fd.reset();
        wlog("socket", "get_config: dumped and closed");
        return;
    }

    if (cmd == "get_dimensions") {
        int minx, maxx, miny, maxy;
        if (waynaptics_get_dimensions(client->srv->dev, &minx, &maxx, &miny, &maxy)) {
            sock_reply(cfd, "    %-24s = %d\n", "MinX", minx);
            sock_reply(cfd, "    %-24s = %d\n", "MaxX", maxx);
            sock_reply(cfd, "    %-24s = %d\n", "MinY", miny);
            sock_reply(cfd, "    %-24s = %d\n", "MaxY", maxy);
        } else {
            sock_reply(cfd, "FAIL dimensions not available\n");
        }
        client->fd.reset();
        wlog("socket", "get_dimensions: dumped and closed");
        return;
    }

    if (cmd.rfind("set_option ", 0) == 0) {
        std::string arg = cmd.substr(11);
        size_t eq = arg.find('=');
        if (eq == std::string::npos) {
            sock_reply(cfd, "FAIL missing '=' in option\n");
            wlog("socket", "set_option: missing '='");
            return;
        }
        std::string name = arg.substr(0, eq);
        std::string value = arg.substr(eq + 1);
        while (!name.empty() && name.back() == ' ') name.pop_back();
        while (!value.empty() && value.front() == ' ') value.erase(value.begin());

        auto err = waynaptics_apply_option(name, value, client->srv->dev);
        if (err) {
            sock_reply(cfd, "FAIL %s\n", err->c_str());
            wlog("socket", "set_option %s=%s: FAIL %s", name.c_str(), value.c_str(), err->c_str());
        } else {
            sock_reply(cfd, "OK\n");
            wlog("socket", "set_option %s=%s: OK", name.c_str(), value.c_str());
        }
        return;
    }

    if (cmd == "save") {
        const std::string &save_path = !client->srv->runtime_config_path.empty()
            ? client->srv->runtime_config_path
            : client->srv->config_path;
        if (save_path.empty()) {
            sock_reply(cfd, "FAIL no config file specified\n");
            wlog("socket", "save: no config file");
            return;
        }
        // Atomic save: write to .tmp, fsync, rename
        std::string tmp_path = save_path + ".tmp";
        FILE *f = fopen(tmp_path.c_str(), "w");
        if (!f) {
            sock_reply(cfd, "FAIL cannot open %s for writing: %s\n",
                       tmp_path.c_str(), strerror(errno));
            wlog_errno("socket", "save: cannot open %s", tmp_path.c_str());
            return;
        }
        waynaptics_dump_config([f](const std::string &line) { emit_to_file(line, f); });
        bool write_ok = (fflush(f) == 0 && fsync(fileno(f)) == 0);
        int close_err = fclose(f);
        if (!write_ok || close_err != 0) {
            sock_reply(cfd, "FAIL write error: %s\n", strerror(errno));
            wlog_errno("socket", "save: write error for %s", tmp_path.c_str());
            unlink(tmp_path.c_str());
            return;
        }
        if (rename(tmp_path.c_str(), save_path.c_str()) != 0) {
            sock_reply(cfd, "FAIL rename failed: %s\n", strerror(errno));
            wlog_errno("socket", "save: rename %s -> %s failed", tmp_path.c_str(), save_path.c_str());
            unlink(tmp_path.c_str());
            return;
        }
        sock_reply(cfd, "OK\n");
        wlog("socket", "save: wrote to %s", save_path.c_str());
        return;
    }

    if (cmd == "reload") {
        waynaptics_reload_config(client->srv->config_path,
                                 client->srv->runtime_config_path,
                                 client->srv->dev);
        sock_reply(cfd, "OK\n");
        wlog("socket", "reload: restored defaults + config");
        return;
    }

    if (cmd == "subscribe_touches") {
        if (!client->subscribed_touches) {
            client->subscribed_touches = true;
            client->srv->touch_subscribers.push_back(client);
            wlog("socket", "subscribe_touches: fd=%d subscribed", cfd);
        }
        sock_reply(cfd, "OK\n");
        return;
    }

    sock_reply(cfd, "FAIL unknown command\n");
    wlog("socket", "unknown command: %s", cmd.c_str());
}

static void remove_subscriber(ClientContext *client) {
    if (client->subscribed_touches && client->srv) {
        auto &subs = client->srv->touch_subscribers;
        subs.erase(std::remove(subs.begin(), subs.end(), client), subs.end());
    }
}

static void destroy_client(gpointer data) {
    auto *client = static_cast<ClientContext *>(data);
    remove_subscriber(client);
    if (client->srv)
        client->srv->num_clients--;
    delete client;
}

static gboolean on_client_data(GIOChannel *source, GIOCondition cond, gpointer data) {
    auto *client = static_cast<ClientContext *>(data);

    if (!client->fd)
        return FALSE;

    if (cond & (G_IO_HUP | G_IO_ERR | G_IO_NVAL))
        return FALSE;

    char buf[1024];
    ssize_t n = read(client->fd.get(), buf, sizeof(buf));
    if (n <= 0)
        return FALSE;

    client->buf.append(buf, n);

    if (client->buf.size() > MAX_CLIENT_BUF) {
        wlog("socket", "fd=%d: buffer overflow, disconnecting", client->fd.get());
        client->fd.reset();
        return FALSE;
    }

    size_t pos;
    while ((pos = client->buf.find('\n')) != std::string::npos) {
        std::string line = client->buf.substr(0, pos);
        client->buf.erase(0, pos + 1);
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (!line.empty()) {
            handle_command(client, line);
            if (!client->fd)
                return FALSE;
        }
    }

    return TRUE;
}

static gboolean on_new_connection(GIOChannel *source, GIOCondition cond, gpointer data) {
    auto *srv = static_cast<SocketContext *>(data);

    int client_fd = accept(srv->listen_fd.get(), nullptr, nullptr);
    if (client_fd < 0)
        return TRUE;

    if (srv->num_clients >= MAX_CLIENTS) {
        wlog("socket", "connection limit reached (%d), rejecting fd=%d",
             MAX_CLIENTS, client_fd);
        close(client_fd);
        return TRUE;
    }

    // Ensure send buffer is large enough for get_config responses
    int sndbuf = CLIENT_SNDBUF_SIZE;
    setsockopt(client_fd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));

    wlog("socket", "new connection (fd=%d, clients=%d)",
         client_fd, srv->num_clients + 1);

    auto client = std::make_unique<ClientContext>();
    client->fd.reset(client_fd);
    client->srv = srv;
    srv->num_clients++;

    GIOChannel *ch = g_io_channel_unix_new(client_fd);
    // GLib takes ownership via destroy_client when the source is removed
    g_io_add_watch_full(ch, G_PRIORITY_DEFAULT,
                        static_cast<GIOCondition>(G_IO_IN | G_IO_HUP | G_IO_ERR),
                        on_client_data, client.release(), destroy_client);
    g_io_channel_unref(ch);

    return TRUE;
}

void waynaptics_config_socket_start(const std::string &socket_path,
                                    const std::string &config_path,
                                    const std::string &runtime_config_path,
                                    DeviceIntPtr dev) {
    // Ignore SIGPIPE consistently with sigaction (main.cpp uses sigaction for SIGINT/SIGTERM)
    struct sigaction sa_pipe{};
    sa_pipe.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa_pipe, nullptr);

    unlink(socket_path.c_str());

    UniqueFd fd(socket(AF_UNIX, SOCK_STREAM, 0));
    if (!fd) {
        wlog_errno("socket", "Failed to create socket");
        return;
    }

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    if (socket_path.size() >= sizeof(addr.sun_path)) {
        wlog("socket", "Socket path too long: %s", socket_path.c_str());
        return;
    }
    strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(fd.get(), reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0) {
        wlog_errno("socket", "Failed to bind %s", socket_path.c_str());
        return;
    }

    chmod(socket_path.c_str(), 0666);

    if (listen(fd.get(), 4) < 0) {
        wlog_errno("socket", "Failed to listen");
        return;
    }

    g_socket_ctx.listen_fd = std::move(fd);
    g_socket_ctx.config_path = config_path;
    g_socket_ctx.runtime_config_path = runtime_config_path;
    g_socket_ctx.dev = dev;
    g_socket_active = true;

    GIOChannel *ch = g_io_channel_unix_new(g_socket_ctx.listen_fd.get());
    g_io_add_watch(ch, G_IO_IN, on_new_connection, &g_socket_ctx);
    g_io_channel_unref(ch);

    wlog("main", "config socket at %s", socket_path.c_str());
}

void waynaptics_broadcast_touches(DeviceIntPtr dev) {
    if (!g_socket_active || g_socket_ctx.touch_subscribers.empty())
        return;

    WaynapticsTouchState state = {};
    waynaptics_get_touch_state(dev, &state);

    // Compare with previous state to avoid spamming unchanged data
    if (state.num_contacts == g_last_touch_state.num_contacts) {
        bool same = true;
        for (int i = 0; i < state.num_contacts && same; i++) {
            same = (state.contacts[i].x == g_last_touch_state.contacts[i].x &&
                    state.contacts[i].y == g_last_touch_state.contacts[i].y &&
                    state.contacts[i].z == g_last_touch_state.contacts[i].z &&
                    state.contacts[i].active == g_last_touch_state.contacts[i].active);
        }
        if (same)
            return;
    }
    g_last_touch_state = state;

    // Build message: "TOUCHES <count>\n<x> <y> <z>\n...\nEND\n"
    std::string msg;
    msg.reserve(256);
    msg += "TOUCHES ";
    msg += std::to_string(state.num_contacts);
    msg += '\n';
    for (int i = 0; i < state.num_contacts; i++) {
        msg += std::to_string(state.contacts[i].x);
        msg += ' ';
        msg += std::to_string(state.contacts[i].y);
        msg += ' ';
        msg += std::to_string(state.contacts[i].z);
        msg += '\n';
    }
    msg += "END\n";

    // Broadcast to all subscribers, removing dead ones
    auto &subs = g_socket_ctx.touch_subscribers;
    for (auto it = subs.begin(); it != subs.end(); ) {
        ClientContext *c = *it;
        ssize_t sent = send(c->fd.get(), msg.data(), msg.size(), MSG_NOSIGNAL | MSG_DONTWAIT);
        if (sent <= 0) {
            c->subscribed_touches = false;
            c->fd.reset();
            it = subs.erase(it);
        } else {
            ++it;
        }
    }
}
