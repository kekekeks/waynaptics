#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <signal.h>
#include <getopt.h>
#include <glib.h>
#include <string>
#include <map>
#include <memory>
#include <stdexcept>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <linux/input.h>

#include "synshared.h"
#include "output_backend.h"
#include "unique_fd.h"
#include "accel.h"
#include "options.h"
#include "synclient_loader.h"
#include "config_socket.h"
#include "log.h"

extern "C" InputDriverRec SYNAPTICS;

extern bool g_verbose_mouse_events;  // defined in event_posting.cpp

bool g_verbose_evdev_events = false;

// Factory functions return unique_ptr — declared here since class definitions are in the .cpp
std::unique_ptr<OutputBackend> waynaptics_create_uinput_backend(
    bool hires_scroll, bool lores_scroll, bool emulate_scrollpoint, double scroll_factor);
std::unique_ptr<OutputBackend> waynaptics_create_dry_backend();

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
    auto dir = std::unique_ptr<DIR, decltype(&closedir)>(opendir("/dev/input"), closedir);
    if (!dir) {
        wlog("main", "cannot open /dev/input");
        return "";
    }

    std::string result;
    struct dirent *entry;
    while ((entry = readdir(dir.get())) != nullptr) {
        std::string d_name(entry->d_name);
        if (!d_name.starts_with("event"))
            continue;

        std::string path = "/dev/input/" + d_name;
        UniqueFd fd(open(path.c_str(), O_RDONLY | O_NONBLOCK));
        if (!fd)
            continue;

        char dev_name[256] = {0};
        if (ioctl(fd.get(), EVIOCGNAME(sizeof(dev_name)), dev_name) >= 0) {
            if (std::string(dev_name).find(name) != std::string::npos) {
                result = path;
                break;
            }
        }
    }

    if (result.empty())
        wlog("main", "no device matching name '%s' found", name);
    else
        wlog("main", "resolved '%s' → %s", name, result.c_str());

    return result;
}

static bool drop_privileges(const char *username) {
    struct passwd *pw = getpwnam(username);
    if (!pw) {
        wlog("main", "user '%s' not found", username);
        return false;
    }

    if (setgroups(0, nullptr) != 0) {
        wlog_errno("main", "setgroups");
        return false;
    }
    if (setgid(pw->pw_gid) != 0) {
        wlog_errno("main", "setgid");
        return false;
    }
    if (setuid(pw->pw_uid) != 0) {
        wlog_errno("main", "setuid");
        return false;
    }
    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) {
        wlog_errno("main", "prctl(NO_NEW_PRIVS)");
        return false;
    }

    wlog("main", "dropped privileges to %s (uid=%d gid=%d)",
         username, pw->pw_uid, pw->pw_gid);
    return true;
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
        "      --drop-user <user>\n"
        "                        Drop privileges to this user after setup\n"
        "      --runtime-config <file>\n"
        "                        Writable config for save (default: same as --config)\n"
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
    const char *drop_user = nullptr;
    const char *runtime_config_path = nullptr;
    bool dry = false;
    bool hires_scroll = true;
    bool lores_scroll = true;
    std::string mouse_type = "scroll-point";
    double scroll_factor = -1.0;  // -1 = use default for mouse type
    bool log_evdev = false;
    bool log_output = false;

    static struct option long_options[] = {
        {"config",         required_argument, nullptr, 'c'},
        {"device",         required_argument, nullptr, 'd'},
        {"device-name",    required_argument, nullptr, 'n'},
        {"socket",         required_argument, nullptr, 's'},
        {"dry",            no_argument,       nullptr, 1},
        {"log-evdev",      no_argument,       nullptr, 2},
        {"log-output",     no_argument,       nullptr, 3},
        {"no-hires-scroll", no_argument,      nullptr, 4},
        {"no-lores-scroll", no_argument,      nullptr, 5},
        {"mouse-type",     required_argument, nullptr, 6},
        {"scroll-factor",  required_argument, nullptr, 7},
        {"drop-user",      required_argument, nullptr, 8},
        {"runtime-config", required_argument, nullptr, 9},
        {"help",           no_argument,       nullptr, 'h'},
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
            case 7:
                try {
                    scroll_factor = std::stod(optarg);
                } catch (const std::exception &) {
                    wlog("main", "invalid --scroll-factor value: %s", optarg);
                    return 1;
                }
                break;
            case 8:   drop_user = optarg; break;
            case 9:   runtime_config_path = optarg; break;
            case 'h': print_usage(argv[0]); return 0;
            default:  print_usage(argv[0]); return 1;
        }
    }

    bool is_scrollpoint = (mouse_type == "scroll-point");
    if (!is_scrollpoint && mouse_type != "generic") {
        wlog("main", "unknown --mouse-type '%s' (use scroll-point or generic)", mouse_type.c_str());
        return 1;
    }
    if (scroll_factor < 0)
        scroll_factor = is_scrollpoint ? 5.0 : 1.0;

    if (device_path && device_name) {
        wlog("main", "--device and --device-name are mutually exclusive");
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
    auto opts = std::make_unique<Options>();
    if (device_path)
        opts->options["Device"] = device_path;
    if (dry)
        opts->options["GrabEventDevice"] = "0";

    // --- Create device structures ---
    DeviceIntRec devRec = {};
    InputInfoRec info = {};
    std::string dev_name_str = "waynaptics";
    info.name = dev_name_str.data();
    info.dev = &devRec;
    info.options = static_cast<XF86OptionPtr>(opts.get());
    devRec.pub.devicePrivate = &info;

    // --- Set up output backend ---
    if (dry)
        g_output_backend = waynaptics_create_dry_backend();
    else
        g_output_backend = waynaptics_create_uinput_backend(hires_scroll, lores_scroll,
                                                            is_scrollpoint, scroll_factor);

    if (!g_output_backend->init()) {
        wlog("main", "failed to initialize output backend");
        return 1;
    }

    // --- SynapticsPreInit ---
    int rc = SYNAPTICS.PreInit(&SYNAPTICS, &info, 0);
    if (rc != 0) {
        wlog("main", "SynapticsPreInit failed (%d)", rc);
        return 1;
    }

    // --- DEVICE_INIT ---
    if (info.device_control(&devRec, DEVICE_INIT) != 0) {
        wlog("main", "DEVICE_INIT failed");
        return 1;
    }

    // --- Initialize pointer acceleration (after driver registered its profile) ---
    waynaptics_accel_init();

    // --- DEVICE_ON ---
    if (info.device_control(&devRec, DEVICE_ON) != 0) {
        wlog("main", "DEVICE_ON failed");
        info.device_control(&devRec, DEVICE_CLOSE);
        return 1;
    }

    // --- Signal handlers ---
    struct sigaction sa = {};
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    // --- Config socket ---
    if (socket_path) {
        waynaptics_config_socket_start(socket_path,
                                       config_path ? config_path : "",
                                       runtime_config_path ? runtime_config_path : "",
                                       &devRec);
    }

    // --- Drop privileges ---
    if (drop_user) {
        if (!drop_privileges(drop_user)) {
            wlog("main", "failed to drop privileges, aborting");
            info.device_control(&devRec, DEVICE_OFF);
            info.device_control(&devRec, DEVICE_CLOSE);
            return 1;
        }
    }

    wlog("main", "running (dry=%s)...", dry ? "yes" : "no");

    // --- Load config AFTER dropping privileges ---
    // Snapshot is taken before loading, so "reload" restores to hardware defaults
    waynaptics_snapshot_initial_config();
    waynaptics_reload_config(config_path ? config_path : "",
                             runtime_config_path ? runtime_config_path : "",
                             &devRec);

    // --- GLib main loop ---
    g_main_context_acquire(g_main_context_default());
    g_main_loop_instance = g_main_loop_new(nullptr, FALSE);
    g_main_loop_run(g_main_loop_instance);
    g_main_loop_unref(g_main_loop_instance);
    g_main_loop_instance = nullptr;

    // --- Cleanup ---
    wlog("main", "shutting down...");
    info.device_control(&devRec, DEVICE_OFF);
    info.device_control(&devRec, DEVICE_CLOSE);
    g_output_backend.reset();

    return 0;
}
