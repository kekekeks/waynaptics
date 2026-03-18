#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <signal.h>
#include <getopt.h>
#include <glib.h>
#include <string>
#include <map>

#include "include/synshared.h"
#include "include/output_backend.h"

// Options class (matches options.cpp)
class Options {
public:
    std::map<std::string, std::string> options;
};

extern "C" {
    extern InputDriverRec SYNAPTICS;
}

extern bool g_verbose_mouse_events;  // defined in event_posting.cpp

bool g_verbose_evdev_events = false;

extern "C" bool waynaptics_load_synclient_config(const char *path, DeviceIntPtr dev);

// UinputBackend and DryBackend are defined in output_backend.cpp but not
// declared in the header. Forward-declare the factory we need.
// Since classes are defined in another TU, we create them via extern helpers.
OutputBackend *waynaptics_create_uinput_backend();
OutputBackend *waynaptics_create_dry_backend();

static GMainLoop *g_main_loop_instance = nullptr;

static void signal_handler(int sig) {
    (void)sig;
    if (g_main_loop_instance)
        g_main_loop_quit(g_main_loop_instance);
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
        "      --dry             Dry mode: don't grab device, don't create uinput\n"
        "      --log-evdev       Log raw evdev touchpad events to stderr\n"
        "      --log-output      Log produced mouse/scroll output events to stderr\n"
        "  -h, --help            Print this help and exit\n",
        prog);
}

int main(int argc, char *argv[]) {
    const char *config_path = nullptr;
    const char *device_path = nullptr;
    bool dry = false;
    bool log_evdev = false;
    bool log_output = false;

    static struct option long_options[] = {
        {"config",     required_argument, nullptr, 'c'},
        {"device",     required_argument, nullptr, 'd'},
        {"dry",        no_argument,       nullptr, 1},
        {"log-evdev",  no_argument,       nullptr, 2},
        {"log-output", no_argument,       nullptr, 3},
        {"help",       no_argument,       nullptr, 'h'},
        {nullptr, 0, nullptr, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "c:d:h", long_options, nullptr)) != -1) {
        switch (opt) {
            case 'c': config_path = optarg; break;
            case 'd': device_path = optarg; break;
            case 1:   dry = true; break;
            case 2:   log_evdev = true; break;
            case 3:   log_output = true; break;
            case 'h': print_usage(argv[0]); return 0;
            default:  print_usage(argv[0]); return 1;
        }
    }

    g_verbose_mouse_events = log_output;
    g_verbose_evdev_events = log_evdev;

    // --- Create Options ---
    auto *opts = new Options();
    if (device_path)
        opts->options["Device"] = device_path;
    if (dry)
        opts->options["GrabEventDevice"] = "0";

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
        g_output_backend = waynaptics_create_uinput_backend();

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
