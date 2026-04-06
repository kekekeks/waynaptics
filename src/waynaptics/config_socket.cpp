#include "include/synshared.h"
#include "include/config_socket.h"
#include "include/synclient_loader.h"
#include "include/xi_properties.h"
#include "include/touch_state.h"
#include <glib.h>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <signal.h>
#include <cerrno>

struct ClientContext;

struct SocketContext {
    int listen_fd;
    const char *config_path;
    const char *runtime_config_path;  // writable save target (falls back to config_path)
    DeviceIntPtr dev;
    std::vector<ClientContext *> touch_subscribers;
};

struct ClientContext {
    int fd;
    std::string buf;
    SocketContext *srv;
    bool subscribed_touches = false;
};

static SocketContext *g_socket_ctx = nullptr;
static WaynapticsTouchState g_last_touch_state = {};

static void sock_reply(int fd, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
static void sock_reply(int fd, const char *fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n > 0)
        send(fd, buf, n, MSG_NOSIGNAL);
}

static void emit_to_fd(const char *line, void *ctx) {
    int fd = *(int *)ctx;
    const char *nl = "\n";
    size_t len = strlen(line);
    // Use send with MSG_NOSIGNAL to avoid SIGPIPE
    send(fd, line, len, MSG_NOSIGNAL);
    send(fd, nl, 1, MSG_NOSIGNAL);
}

static void emit_to_file(const char *line, void *ctx) {
    FILE *f = (FILE *)ctx;
    fprintf(f, "%s\n", line);
}

static void handle_command(ClientContext *client, const std::string &cmd) {
    fprintf(stderr, "[SOCKET] command: %s\n", cmd.c_str());

    if (cmd == "get_capabilities") {
        Atom cap_atom = XIGetKnownProperty("Synaptics Capabilities");
        if (cap_atom == 0) {
            sock_reply(client->fd, "FAIL property not registered\n");
            fprintf(stderr, "[SOCKET] get_capabilities: property not registered\n");
            return;
        }
        XIPropertyValuePtr val = waynaptics_get_property_value(cap_atom);
        if (!val || !val->data || val->size < 7) {
            sock_reply(client->fd, "FAIL property has no data\n");
            fprintf(stderr, "[SOCKET] get_capabilities: no data\n");
            return;
        }
        uint8_t *caps = (uint8_t *)val->data;
        static const char *cap_names[] = {
            "HasLeftButton", "HasMiddleButton", "HasRightButton",
            "TwoFingerDetection", "ThreeFingerDetection",
            "PressureDetection", "FingerOrPalmWidthDetection"
        };
        for (int i = 0; i < 7; i++)
            sock_reply(client->fd, "    %-24s = %d\n", cap_names[i], caps[i]);
        close(client->fd);
        client->fd = -1;
        fprintf(stderr, "[SOCKET] get_capabilities: dumped and closed\n");
        return;
    }

    if (cmd == "get_config") {
        waynaptics_dump_config(emit_to_fd, &client->fd);
        close(client->fd);
        client->fd = -1;
        fprintf(stderr, "[SOCKET] get_config: dumped and closed\n");
        return;
    }

    if (cmd == "get_dimensions") {
        int minx, maxx, miny, maxy;
        if (waynaptics_get_dimensions(client->srv->dev, &minx, &maxx, &miny, &maxy)) {
            sock_reply(client->fd, "    %-24s = %d\n", "MinX", minx);
            sock_reply(client->fd, "    %-24s = %d\n", "MaxX", maxx);
            sock_reply(client->fd, "    %-24s = %d\n", "MinY", miny);
            sock_reply(client->fd, "    %-24s = %d\n", "MaxY", maxy);
        } else {
            sock_reply(client->fd, "FAIL dimensions not available\n");
        }
        close(client->fd);
        client->fd = -1;
        fprintf(stderr, "[SOCKET] get_dimensions: dumped and closed\n");
        return;
    }

    if (cmd.rfind("set_option ", 0) == 0) {
        std::string arg = cmd.substr(11);
        size_t eq = arg.find('=');
        if (eq == std::string::npos) {
            sock_reply(client->fd, "FAIL missing '=' in option\n");
            fprintf(stderr, "[SOCKET] set_option: missing '='\n");
            return;
        }
        std::string name = arg.substr(0, eq);
        std::string value = arg.substr(eq + 1);
        // trim
        while (!name.empty() && name.back() == ' ') name.pop_back();
        while (!value.empty() && value.front() == ' ') value.erase(value.begin());

        const char *err = waynaptics_apply_option(name.c_str(), value.c_str(), client->srv->dev);
        if (err) {
            sock_reply(client->fd, "FAIL %s\n", err);
            fprintf(stderr, "[SOCKET] set_option %s=%s: FAIL %s\n", name.c_str(), value.c_str(), err);
        } else {
            sock_reply(client->fd, "OK\n");
            fprintf(stderr, "[SOCKET] set_option %s=%s: OK\n", name.c_str(), value.c_str());
        }
        return;
    }

    if (cmd == "save") {
        const char *save_path = client->srv->runtime_config_path
            ? client->srv->runtime_config_path
            : client->srv->config_path;
        if (!save_path) {
            sock_reply(client->fd, "FAIL no config file specified\n");
            fprintf(stderr, "[SOCKET] save: no config file\n");
            return;
        }
        FILE *f = fopen(save_path, "w");
        if (!f) {
            sock_reply(client->fd, "FAIL cannot open %s for writing: %s\n",
                       save_path, strerror(errno));
            fprintf(stderr, "[SOCKET] save: cannot open %s: %s\n",
                    save_path, strerror(errno));
            return;
        }
        waynaptics_dump_config(emit_to_file, f);
        fclose(f);
        sock_reply(client->fd, "OK\n");
        fprintf(stderr, "[SOCKET] save: wrote to %s\n", save_path);
        return;
    }

    if (cmd == "reload") {
        // Restore to initial defaults (snapshot taken at startup before any config load)
        waynaptics_restore_initial_config(client->srv->dev);

        // Re-apply system config (if available)
        if (client->srv->config_path) {
            if (!waynaptics_load_synclient_config(client->srv->config_path, client->srv->dev)) {
                sock_reply(client->fd, "FAIL failed to reload config from %s\n", client->srv->config_path);
                fprintf(stderr, "[SOCKET] reload: failed to reload from %s\n", client->srv->config_path);
                return;
            }
        }
        // Apply runtime config overrides (if different and exists)
        const char *rt = client->srv->runtime_config_path;
        if (rt && (!client->srv->config_path || strcmp(rt, client->srv->config_path) != 0)) {
            if (access(rt, R_OK) == 0) {
                if (!waynaptics_load_synclient_config(rt, client->srv->dev)) {
                    fprintf(stderr, "[SOCKET] reload: warning: failed to reload runtime config from %s\n", rt);
                }
            }
        }
        sock_reply(client->fd, "OK\n");
        fprintf(stderr, "[SOCKET] reload: restored defaults + config\n");
        return;
    }

    if (cmd == "subscribe_touches") {
        if (!client->subscribed_touches) {
            client->subscribed_touches = true;
            client->srv->touch_subscribers.push_back(client);
            fprintf(stderr, "[SOCKET] subscribe_touches: fd=%d subscribed\n", client->fd);
        }
        sock_reply(client->fd, "OK\n");
        return;
    }

    sock_reply(client->fd, "FAIL unknown command\n");
    fprintf(stderr, "[SOCKET] unknown command: %s\n", cmd.c_str());
}

static void remove_subscriber(ClientContext *client) {
    if (client->subscribed_touches && client->srv) {
        auto &subs = client->srv->touch_subscribers;
        subs.erase(std::remove(subs.begin(), subs.end(), client), subs.end());
    }
}

static gboolean on_client_data(GIOChannel *source, GIOCondition cond, gpointer data) {
    auto *client = static_cast<ClientContext *>(data);

    if (client->fd < 0) {
        remove_subscriber(client);
        delete client;
        return FALSE;
    }

    if (cond & (G_IO_HUP | G_IO_ERR | G_IO_NVAL)) {
        remove_subscriber(client);
        close(client->fd);
        delete client;
        return FALSE;
    }

    char buf[1024];
    ssize_t n = read(client->fd, buf, sizeof(buf));
    if (n <= 0) {
        remove_subscriber(client);
        close(client->fd);
        delete client;
        return FALSE;
    }

    client->buf.append(buf, n);

    size_t pos;
    while ((pos = client->buf.find('\n')) != std::string::npos) {
        std::string line = client->buf.substr(0, pos);
        client->buf.erase(0, pos + 1);
        // strip \r
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (!line.empty()) {
            handle_command(client, line);
            if (client->fd < 0) {
                delete client;
                return FALSE;
            }
        }
    }

    return TRUE;
}

static gboolean on_new_connection(GIOChannel *source, GIOCondition cond, gpointer data) {
    auto *srv = static_cast<SocketContext *>(data);

    int client_fd = accept(srv->listen_fd, nullptr, nullptr);
    if (client_fd < 0)
        return TRUE;

    fprintf(stderr, "[SOCKET] new connection (fd=%d)\n", client_fd);

    auto *client = new ClientContext();
    client->fd = client_fd;
    client->srv = srv;

    GIOChannel *ch = g_io_channel_unix_new(client_fd);
    g_io_add_watch(ch, (GIOCondition)(G_IO_IN | G_IO_HUP | G_IO_ERR), on_client_data, client);
    g_io_channel_unref(ch);

    return TRUE;
}

extern "C" void waynaptics_config_socket_start(const char *socket_path,
                                               const char *config_path,
                                               const char *runtime_config_path,
                                               DeviceIntPtr dev) {
    // Ignore SIGPIPE so broken client connections don't kill us
    signal(SIGPIPE, SIG_IGN);

    unlink(socket_path);

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        fprintf(stderr, "[SOCKET] Failed to create socket: %m\n");
        return;
    }

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "[SOCKET] Failed to bind %s: %m\n", socket_path);
        close(fd);
        return;
    }

    // Allow non-root users to connect to the config socket
    chmod(socket_path, 0666);

    if (listen(fd, 4) < 0) {
        fprintf(stderr, "[SOCKET] Failed to listen: %m\n");
        close(fd);
        return;
    }

    auto *srv = new SocketContext();
    srv->listen_fd = fd;
    srv->config_path = config_path;
    srv->runtime_config_path = runtime_config_path;
    srv->dev = dev;

    g_socket_ctx = srv;

    GIOChannel *ch = g_io_channel_unix_new(fd);
    g_io_add_watch(ch, G_IO_IN, on_new_connection, srv);
    g_io_channel_unref(ch);

    fprintf(stderr, "waynaptics: config socket at %s\n", socket_path);
}

extern "C" void waynaptics_broadcast_touches(DeviceIntPtr dev) {
    if (!g_socket_ctx || g_socket_ctx->touch_subscribers.empty())
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
    char msg[512];
    int offset = snprintf(msg, sizeof(msg), "TOUCHES %d\n", state.num_contacts);
    for (int i = 0; i < state.num_contacts && offset < (int)sizeof(msg) - 32; i++) {
        offset += snprintf(msg + offset, sizeof(msg) - offset, "%d %d %d\n",
                           state.contacts[i].x, state.contacts[i].y, state.contacts[i].z);
    }
    offset += snprintf(msg + offset, sizeof(msg) - offset, "END\n");

    // Broadcast to all subscribers, removing dead ones
    auto &subs = g_socket_ctx->touch_subscribers;
    for (auto it = subs.begin(); it != subs.end(); ) {
        ClientContext *c = *it;
        ssize_t sent = send(c->fd, msg, offset, MSG_NOSIGNAL);
        if (sent <= 0) {
            c->subscribed_touches = false;
            close(c->fd);
            c->fd = -1;
            it = subs.erase(it);
        } else {
            ++it;
        }
    }
}
