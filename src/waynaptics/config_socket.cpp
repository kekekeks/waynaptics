#include "include/synshared.h"
#include "include/config_socket.h"
#include "include/synclient_loader.h"
#include <glib.h>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>

struct SocketContext {
    int listen_fd;
    const char *config_path;
    DeviceIntPtr dev;
};

struct ClientContext {
    int fd;
    std::string buf;
    SocketContext *srv;
};

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

    if (cmd == "get_config") {
        waynaptics_dump_config(emit_to_fd, &client->fd);
        close(client->fd);
        client->fd = -1;
        fprintf(stderr, "[SOCKET] get_config: dumped and closed\n");
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
        if (!client->srv->config_path) {
            sock_reply(client->fd, "FAIL no config file specified\n");
            fprintf(stderr, "[SOCKET] save: no config file\n");
            return;
        }
        FILE *f = fopen(client->srv->config_path, "w");
        if (!f) {
            sock_reply(client->fd, "FAIL cannot open %s for writing\n", client->srv->config_path);
            fprintf(stderr, "[SOCKET] save: cannot open %s\n", client->srv->config_path);
            return;
        }
        int file_fd = fileno(f);
        waynaptics_dump_config(emit_to_file, f);
        fclose(f);
        sock_reply(client->fd, "OK\n");
        fprintf(stderr, "[SOCKET] save: wrote to %s\n", client->srv->config_path);
        return;
    }

    sock_reply(client->fd, "FAIL unknown command\n");
    fprintf(stderr, "[SOCKET] unknown command: %s\n", cmd.c_str());
}

static gboolean on_client_data(GIOChannel *source, GIOCondition cond, gpointer data) {
    auto *client = static_cast<ClientContext *>(data);

    if (client->fd < 0) {
        delete client;
        return FALSE;
    }

    if (cond & (G_IO_HUP | G_IO_ERR | G_IO_NVAL)) {
        close(client->fd);
        delete client;
        return FALSE;
    }

    char buf[1024];
    ssize_t n = read(client->fd, buf, sizeof(buf));
    if (n <= 0) {
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

    if (listen(fd, 4) < 0) {
        fprintf(stderr, "[SOCKET] Failed to listen: %m\n");
        close(fd);
        return;
    }

    auto *srv = new SocketContext();
    srv->listen_fd = fd;
    srv->config_path = config_path;
    srv->dev = dev;

    GIOChannel *ch = g_io_channel_unix_new(fd);
    g_io_add_watch(ch, G_IO_IN, on_new_connection, srv);
    g_io_channel_unref(ch);

    fprintf(stderr, "waynaptics: config socket at %s\n", socket_path);
}
