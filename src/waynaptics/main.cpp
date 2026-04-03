#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <signal.h>
#include <getopt.h>
#include <glib.h>
#include <string>
#include <map>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/input.h>

#include "include/synshared.h"
#include "include/output_backend.h"
#include "include/ptrveloc.h"
#include "include/options.h"
#include "include/synclient_loader.h"
#include "include/config_socket.h"

extern "C" {
    extern InputDriverRec SYNAPTICS;
}

extern bool g_verbose_mouse_events;  // defined in event_posting.cpp

bool g_verbose_evdev_events = false;

// UinputBackend and DryBackend are defined in output_backend.cpp but not
// declared in the header. Forward-declare the factory we need.
// Since classes are defined in another TU, we create them via extern helpers.
OutputBackend *waynaptics_create_uinput_backend(bool hires_scroll, bool lores_scroll,
                                                bool emulate_scrollpoint, double scroll_factor);
OutputBackend *waynaptics_create_dry_backend();

static GMainLoop *g_main_loop_instance = nullptr;

static void signal_handler(int sig) {
    (void)sig;
    if (g_main_loop_instance)
        g_main_loop_quit(g_main_loop_instance);
}

/*
 * Resolve a device name (e.g. "SYNA801B:00 06CB:CEC7 Touchpad") to its
 * /dev/input/eventN path by scanning all event devices.
 */
static std::string resolve_device_name(const char *name) {
    DIR *dir = opendir("/dev/input");
    if (!dir) {
        fprintf(stderr, "waynaptics: cannot open /dev/input\n");
        return "";
    }

    std::string result;
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strncmp(entry->d_name, "event", 5) != 0)
            continue;

        std::string path = std::string("/dev/input/") + entry->d_name;
        int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0)
            continue;

        char dev_name[256] = {0};
        if (ioctl(fd, EVIOCGNAME(sizeof(dev_name)), dev_name) >= 0) {
            if (strstr(dev_name, name) != nullptr) {
                result = path;
                close(fd);
                break;
            }
        }
        close(fd);
    }
    closedir(dir);

    if (result.empty())
        fprintf(stderr, "waynaptics: no device matching name '%s' found\n", name);
    else
        fprintf(stderr, "waynaptics: resolved '%s' → %s\n", name, result.c_str());

    return result;
}

static void print_usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s [OPTIONS]\n"
        "\n"
        "Synaptics touchpad → PS/2 mouse emulator via uinput.\n"
        "\n"
        "Options:\n"
        "  -c, --config <file>   Path to synclient parameter dump\n"
        "  -d, --device <path>   Specific evdev device path (auto-detect if omitted)\n"
        "  -n, --device-name <name>\n"
        "                        Match evdev device by name substring (e.g. \"Touchpad\")\n"
        "      --mouse-type <type>\n"
        "                        scroll-point (default) or generic\n"
        "      --scroll-factor <N>\n"
        "                        Scroll speed multiplier (default: 5 for scroll-point)\n"
        "      --dry             Dry mode: don't grab device, don't create uinput\n"
        "      --no-hires-scroll Disable hi-res scroll events (REL_WHEEL_HI_RES)\n"
        "      --no-lores-scroll Disable low-res scroll events (REL_WHEEL)\n"
        "  -s, --socket <path>   Config socket path (for runtime control)\n"
        "      --log-evdev       Log raw evdev touchpad events to stderr\n"
        "      --log-output      Log produced mouse/scroll output events to stderr\n"
        "  -h, --help            Print this help and exit\n",
        prog);
}

int main(int argc, char *argv[]) {
    const char *config_path = nullptr;
    const char *device_path = nullptr;
    const char *device_name = nullptr;
    const char *socket_path = nullptr;
    bool dry = false;
    bool hires_scroll = true;
    bool lores_scroll = true;
    const char *mouse_type = "scroll-point";
    double scroll_factor = -1.0;  // -1 = use default for mouse type
    bool log_evdev = false;
    bool log_output = false;

    static struct option long_options[] = {
        {"config",      required_argument, nullptr, 'c'},
        {"device",      required_argument, nullptr, 'd'},
        {"device-name", required_argument, nullptr, 'n'},
        {"socket",      required_argument, nullptr, 's'},
        {"dry",             no_argument,       nullptr, 1},
        {"log-evdev",       no_argument,       nullptr, 2},
        {"log-output",      no_argument,       nullptr, 3},
        {"no-hires-scroll", no_argument,       nullptr, 4},
        {"no-lores-scroll", no_argument,       nullptr, 5},
        {"mouse-type",      required_argument, nullptr, 6},
        {"scroll-factor",   required_argument, nullptr, 7},
        {"help",        no_argument,       nullptr, 'h'},
        {nullptr, 0, nullptr, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "c:d:n:s:h", long_options, nullptr)) != -1) {
        switch (opt) {
            case 'c': config_path = optarg; break;
            case 'd': device_path = optarg; break;
            case 'n': device_name = optarg; break;
            case 's': socket_path = optarg; break;
            case 1:   dry = true; break;
            case 2:   log_evdev = true; break;
            case 3:   log_output = true; break;
            case 4:   hires_scroll = false; break;
            case 5:   lores_scroll = false; break;
            case 6:   mouse_type = optarg; break;
            case 7:   scroll_factor = atof(optarg); break;
            case 'h': print_usage(argv[0]); return 0;
            default:  print_usage(argv[0]); return 1;
        }
    }

    bool is_scrollpoint = (strcmp(mouse_type, "scroll-point") == 0);
    if (!is_scrollpoint && strcmp(mouse_type, "generic") != 0) {
        fprintf(stderr, "waynaptics: unknown --mouse-type '%s' (use scroll-point or generic)\n", mouse_type);
        return 1;
    }
    if (scroll_factor < 0)
        scroll_factor = is_scrollpoint ? 5.0 : 1.0;

    if (device_path && device_name) {
        fprintf(stderr, "waynaptics: --device and --device-name are mutually exclusive\n");
        return 1;
    }

    // Resolve device name to path
    std::string resolved_path;
    if (device_name) {
        resolved_path = resolve_device_name(device_name);
        if (resolved_path.empty())
            return 1;
        device_path = resolved_path.c_str();
    }

    g_verbose_mouse_events = log_output;
    g_verbose_evdev_events = log_evdev;

    // --- Create Options ---
    auto *opts = new Options();
    if (device_path)
        opts->options["Device"] = device_path;
    if (dry)
        opts->options["GrabEventDevice"] = "0";

    // Pre-load synclient config into options BEFORE PreInit
    // so set_default_parameters() reads correct MinSpeed/MaxSpeed/AccelFactor
    if (config_path) {
        waynaptics_preload_synclient_options(config_path, (XF86OptionPtr)opts);
    }

    // --- Create device structures ---
    DeviceIntRec devRec = {};
    InputInfoRec info = {};
    info.name = strdup("waynaptics");
    info.dev = &devRec;
    info.options = (XF86OptionPtr)opts;
    devRec.pub.devicePrivate = &info;

    // --- Set up output backend ---
    if (dry)
        g_output_backend = waynaptics_create_dry_backend();
    else
        g_output_backend = waynaptics_create_uinput_backend(hires_scroll, lores_scroll,
                                                            is_scrollpoint, scroll_factor);

    if (!g_output_backend->init()) {
        fprintf(stderr, "waynaptics: failed to initialize output backend\n");
        free(info.name);
        delete opts;
        return 1;
    }

    // --- SynapticsPreInit ---
    int rc = SYNAPTICS.PreInit(&SYNAPTICS, &info, 0);
    if (rc != 0) {
        fprintf(stderr, "waynaptics: SynapticsPreInit failed (%d)\n", rc);
        g_output_backend->destroy();
        delete g_output_backend;
        free(info.name);
        delete opts;
        return 1;
    }

    // --- DEVICE_INIT ---
    if (info.device_control(&devRec, DEVICE_INIT) != 0) {
        fprintf(stderr, "waynaptics: DEVICE_INIT failed\n");
        g_output_backend->destroy();
        delete g_output_backend;
        free(info.name);
        delete opts;
        return 1;
    }

    // --- Initialize pointer acceleration (after driver registered its profile) ---
    waynaptics_accel_init();

    // --- Snapshot initial config (hardware defaults + pre-loaded options) ---
    waynaptics_snapshot_initial_config();

    // --- Load synclient config (after properties are initialized) ---
    if (config_path) {
        if (!waynaptics_load_synclient_config(config_path, &devRec)) {
            fprintf(stderr, "waynaptics: warning: failed to load config '%s'\n", config_path);
        }
    }

    // --- DEVICE_ON ---
    if (info.device_control(&devRec, DEVICE_ON) != 0) {
        fprintf(stderr, "waynaptics: DEVICE_ON failed\n");
        info.device_control(&devRec, DEVICE_CLOSE);
        g_output_backend->destroy();
        delete g_output_backend;
        free(info.name);
        delete opts;
        return 1;
    }

    // --- Signal handlers ---
    struct sigaction sa = {};
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    fprintf(stderr, "waynaptics: running (dry=%s)...\n", dry ? "yes" : "no");

    // --- Config socket ---
    if (socket_path) {
        waynaptics_config_socket_start(socket_path, config_path, &devRec);
    }

    // --- GLib main loop ---
    g_main_context_acquire(g_main_context_default());
    g_main_loop_instance = g_main_loop_new(nullptr, FALSE);
    g_main_loop_run(g_main_loop_instance);
    g_main_loop_unref(g_main_loop_instance);
    g_main_loop_instance = nullptr;

    // --- Cleanup ---
    fprintf(stderr, "waynaptics: shutting down...\n");
    info.device_control(&devRec, DEVICE_OFF);
    info.device_control(&devRec, DEVICE_CLOSE);
    g_output_backend->destroy();
    delete g_output_backend;
    g_output_backend = nullptr;
    free(info.name);
    delete opts;

    return 0;
}
