#include "shim.h"
#include "include/synshared.h"
#include "include/xi_properties.h"
#include "include/synclient_loader.h"
#include "include/options.h"
#include <synaptics-properties.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include <map>

enum ParamType { PT_INT = 0, PT_BOOL = 1, PT_DOUBLE = 2 };

struct ParamMapping {
    const char *name;
    ParamType type;
    const char *prop_name;
    int format;
    int offset;
};

static const ParamMapping params[] = {
    {"LeftEdge",              PT_INT,    SYNAPTICS_PROP_EDGES,                      32, 0},
    {"RightEdge",             PT_INT,    SYNAPTICS_PROP_EDGES,                      32, 1},
    {"TopEdge",               PT_INT,    SYNAPTICS_PROP_EDGES,                      32, 2},
    {"BottomEdge",            PT_INT,    SYNAPTICS_PROP_EDGES,                      32, 3},
    {"FingerLow",             PT_INT,    SYNAPTICS_PROP_FINGER,                     32, 0},
    {"FingerHigh",            PT_INT,    SYNAPTICS_PROP_FINGER,                     32, 1},
    {"MaxTapTime",            PT_INT,    SYNAPTICS_PROP_TAP_TIME,                   32, 0},
    {"MaxTapMove",            PT_INT,    SYNAPTICS_PROP_TAP_MOVE,                   32, 0},
    {"MaxDoubleTapTime",      PT_INT,    SYNAPTICS_PROP_TAP_DURATIONS,              32, 1},
    {"SingleTapTimeout",      PT_INT,    SYNAPTICS_PROP_TAP_DURATIONS,              32, 0},
    {"ClickTime",             PT_INT,    SYNAPTICS_PROP_TAP_DURATIONS,              32, 2},
    {"EmulateMidButtonTime",  PT_INT,    SYNAPTICS_PROP_MIDDLE_TIMEOUT,             32, 0},
    {"EmulateTwoFingerMinZ",  PT_INT,    SYNAPTICS_PROP_TWOFINGER_PRESSURE,         32, 0},
    {"EmulateTwoFingerMinW",  PT_INT,    SYNAPTICS_PROP_TWOFINGER_WIDTH,            32, 0},
    {"VertScrollDelta",       PT_INT,    SYNAPTICS_PROP_SCROLL_DISTANCE,            32, 0},
    {"HorizScrollDelta",      PT_INT,    SYNAPTICS_PROP_SCROLL_DISTANCE,            32, 1},
    {"VertEdgeScroll",        PT_BOOL,   SYNAPTICS_PROP_SCROLL_EDGE,                 8, 0},
    {"HorizEdgeScroll",       PT_BOOL,   SYNAPTICS_PROP_SCROLL_EDGE,                 8, 1},
    {"CornerCoasting",        PT_BOOL,   SYNAPTICS_PROP_SCROLL_EDGE,                 8, 2},
    {"VertTwoFingerScroll",   PT_BOOL,   SYNAPTICS_PROP_SCROLL_TWOFINGER,            8, 0},
    {"HorizTwoFingerScroll",  PT_BOOL,   SYNAPTICS_PROP_SCROLL_TWOFINGER,            8, 1},
    {"MinSpeed",              PT_DOUBLE, SYNAPTICS_PROP_SPEED,                       0, 0},
    {"MaxSpeed",              PT_DOUBLE, SYNAPTICS_PROP_SPEED,                       0, 1},
    {"AccelFactor",           PT_DOUBLE, SYNAPTICS_PROP_SPEED,                       0, 2},
    {"UpDownScrolling",       PT_BOOL,   SYNAPTICS_PROP_BUTTONSCROLLING,             8, 0},
    {"LeftRightScrolling",    PT_BOOL,   SYNAPTICS_PROP_BUTTONSCROLLING,             8, 1},
    {"UpDownScrollRepeat",    PT_BOOL,   SYNAPTICS_PROP_BUTTONSCROLLING_REPEAT,      8, 0},
    {"LeftRightScrollRepeat", PT_BOOL,   SYNAPTICS_PROP_BUTTONSCROLLING_REPEAT,      8, 1},
    {"ScrollButtonRepeat",    PT_INT,    SYNAPTICS_PROP_BUTTONSCROLLING_TIME,        32, 0},
    {"TouchpadOff",           PT_INT,    SYNAPTICS_PROP_OFF,                          8, 0},
    {"LockedDrags",           PT_BOOL,   SYNAPTICS_PROP_LOCKED_DRAGS,                8, 0},
    {"LockedDragTimeout",     PT_INT,    SYNAPTICS_PROP_LOCKED_DRAGS_TIMEOUT,        32, 0},
    {"RTCornerButton",        PT_INT,    SYNAPTICS_PROP_TAP_ACTION,                  8, 0},
    {"RBCornerButton",        PT_INT,    SYNAPTICS_PROP_TAP_ACTION,                  8, 1},
    {"LTCornerButton",        PT_INT,    SYNAPTICS_PROP_TAP_ACTION,                  8, 2},
    {"LBCornerButton",        PT_INT,    SYNAPTICS_PROP_TAP_ACTION,                  8, 3},
    {"TapButton1",            PT_INT,    SYNAPTICS_PROP_TAP_ACTION,                  8, 4},
    {"TapButton2",            PT_INT,    SYNAPTICS_PROP_TAP_ACTION,                  8, 5},
    {"TapButton3",            PT_INT,    SYNAPTICS_PROP_TAP_ACTION,                  8, 6},
    {"ClickFinger1",          PT_INT,    SYNAPTICS_PROP_CLICK_ACTION,                8, 0},
    {"ClickFinger2",          PT_INT,    SYNAPTICS_PROP_CLICK_ACTION,                8, 1},
    {"ClickFinger3",          PT_INT,    SYNAPTICS_PROP_CLICK_ACTION,                8, 2},
    {"CircularScrolling",     PT_BOOL,   SYNAPTICS_PROP_CIRCULAR_SCROLLING,          8, 0},
    {"CircScrollDelta",       PT_DOUBLE, SYNAPTICS_PROP_CIRCULAR_SCROLLING_DIST,     0, 0},
    {"CircScrollTrigger",     PT_INT,    SYNAPTICS_PROP_CIRCULAR_SCROLLING_TRIGGER,  8, 0},
    {"CircularPad",           PT_BOOL,   SYNAPTICS_PROP_CIRCULAR_PAD,                8, 0},
    {"PalmDetect",            PT_BOOL,   SYNAPTICS_PROP_PALM_DETECT,                 8, 0},
    {"PalmMinWidth",          PT_INT,    SYNAPTICS_PROP_PALM_DIMENSIONS,             32, 0},
    {"PalmMinZ",              PT_INT,    SYNAPTICS_PROP_PALM_DIMENSIONS,             32, 1},
    {"CoastingSpeed",         PT_DOUBLE, SYNAPTICS_PROP_COASTING_SPEED,              0, 0},
    {"CoastingFriction",      PT_DOUBLE, SYNAPTICS_PROP_COASTING_SPEED,              0, 1},
    {"PressureMotionMinZ",    PT_INT,    SYNAPTICS_PROP_PRESSURE_MOTION,             32, 0},
    {"PressureMotionMaxZ",    PT_INT,    SYNAPTICS_PROP_PRESSURE_MOTION,             32, 1},
    {"PressureMotionMinFactor",  PT_DOUBLE, SYNAPTICS_PROP_PRESSURE_MOTION_FACTOR,   0, 0},
    {"PressureMotionMaxFactor",  PT_DOUBLE, SYNAPTICS_PROP_PRESSURE_MOTION_FACTOR,   0, 1},
    {"GrabEventDevice",       PT_BOOL,   SYNAPTICS_PROP_GRAB,                        8, 0},
    {"TapAndDragGesture",     PT_BOOL,   SYNAPTICS_PROP_GESTURES,                    8, 0},
    {"AreaLeftEdge",          PT_INT,    SYNAPTICS_PROP_AREA,                        32, 0},
    {"AreaRightEdge",         PT_INT,    SYNAPTICS_PROP_AREA,                        32, 1},
    {"AreaTopEdge",           PT_INT,    SYNAPTICS_PROP_AREA,                        32, 2},
    {"AreaBottomEdge",        PT_INT,    SYNAPTICS_PROP_AREA,                        32, 3},
    {"HorizHysteresis",       PT_INT,    SYNAPTICS_PROP_NOISE_CANCELLATION,          32, 0},
    {"VertHysteresis",        PT_INT,    SYNAPTICS_PROP_NOISE_CANCELLATION,          32, 1},
    {"ClickPad",              PT_BOOL,   SYNAPTICS_PROP_CLICKPAD,                     8, 0},
    {"RightButtonAreaLeft",   PT_INT,    SYNAPTICS_PROP_SOFTBUTTON_AREAS,            32, 0},
    {"RightButtonAreaRight",  PT_INT,    SYNAPTICS_PROP_SOFTBUTTON_AREAS,            32, 1},
    {"RightButtonAreaTop",    PT_INT,    SYNAPTICS_PROP_SOFTBUTTON_AREAS,            32, 2},
    {"RightButtonAreaBottom", PT_INT,    SYNAPTICS_PROP_SOFTBUTTON_AREAS,            32, 3},
    {"MiddleButtonAreaLeft",  PT_INT,    SYNAPTICS_PROP_SOFTBUTTON_AREAS,            32, 4},
    {"MiddleButtonAreaRight", PT_INT,    SYNAPTICS_PROP_SOFTBUTTON_AREAS,            32, 5},
    {"MiddleButtonAreaTop",   PT_INT,    SYNAPTICS_PROP_SOFTBUTTON_AREAS,            32, 6},
    {"MiddleButtonAreaBottom",PT_INT,    SYNAPTICS_PROP_SOFTBUTTON_AREAS,            32, 7},
    {nullptr,                 PT_INT,    nullptr,                                     0, 0},
};

static const ParamMapping *find_param(const char *name) {
    for (const ParamMapping *p = params; p->name != nullptr; p++) {
        if (strcmp(p->name, name) == 0)
            return p;
    }
    return nullptr;
}

static std::string trim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

/*
 * Pre-load synclient config into the Options map BEFORE SynapticsPreInit.
 * This ensures set_default_parameters() reads the correct values
 * (especially MinSpeed/MaxSpeed/AccelFactor which affect acceleration setup).
 */
extern "C" bool waynaptics_preload_synclient_options(const char *path, XF86OptionPtr opts_ptr) {
    auto *opts = static_cast<Options *>(opts_ptr);
    std::ifstream file(path);
    if (!file.is_open())
        return false;

    std::string line;
    while (std::getline(file, line)) {
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed == "Parameter settings:")
            continue;
        size_t eq_pos = trimmed.find('=');
        if (eq_pos == std::string::npos)
            continue;
        std::string name = trim(trimmed.substr(0, eq_pos));
        std::string value_str = trim(trimmed.substr(eq_pos + 1));
        if (name.empty() || value_str.empty())
            continue;
        if (name == "GrabEventDevice" || name == "TouchpadOff")
            continue;
        /* Only inject if it's a known synclient parameter */
        if (find_param(name.c_str()) != nullptr)
            opts->options[name] = value_str;
    }
    return true;
}

extern "C" bool waynaptics_load_synclient_config(const char *path, DeviceIntPtr dev) {
    std::ifstream file(path);
    if (!file.is_open()) {
        fprintf(stderr, "[CONFIG] Failed to open config file: %s\n", path);
        return false;
    }

    std::string line;
    int line_num = 0;

    while (std::getline(file, line)) {
        line_num++;
        std::string trimmed = trim(line);

        if (trimmed.empty() || trimmed == "Parameter settings:")
            continue;

        // Parse "name = value"
        size_t eq_pos = trimmed.find('=');
        if (eq_pos == std::string::npos) {
            fprintf(stderr, "[CONFIG] Warning: skipping malformed line %d: %s\n",
                    line_num, trimmed.c_str());
            continue;
        }

        std::string name = trim(trimmed.substr(0, eq_pos));
        std::string value_str = trim(trimmed.substr(eq_pos + 1));

        if (name.empty() || value_str.empty())
            continue;

        // GrabEventDevice is controlled by --dry flag, not config file
        // TouchpadOff gets set to 2 by DEs with "disable while typing" —
        // not a real user preference, ignore it
        if (name == "GrabEventDevice" || name == "TouchpadOff") {
            fprintf(stderr, "[CONFIG] Ignoring %s (not a portable setting)\n", name.c_str());
            continue;
        }

        const ParamMapping *mapping = find_param(name.c_str());
        if (!mapping) {
            fprintf(stderr, "[CONFIG] Warning: unknown parameter '%s', skipping\n",
                    name.c_str());
            continue;
        }

        Atom prop_atom = XIGetKnownProperty(mapping->prop_name);
        if (prop_atom == 0) {
            fprintf(stderr, "[CONFIG] Warning: property '%s' not registered, skipping '%s'\n",
                    mapping->prop_name, name.c_str());
            continue;
        }

        XIPropertyValuePtr val = waynaptics_get_property_value(prop_atom);
        if (!val || !val->data) {
            fprintf(stderr, "[CONFIG] Warning: property '%s' has no value, skipping '%s'\n",
                    mapping->prop_name, name.c_str());
            continue;
        }

        // Parse and apply value based on type and format
        if (mapping->format == 0) {
            // Float property
            char *endptr;
            double dval = strtod(value_str.c_str(), &endptr);
            if (endptr == value_str.c_str()) {
                fprintf(stderr, "[CONFIG] Warning: invalid float value '%s' for '%s'\n",
                        value_str.c_str(), name.c_str());
                continue;
            }
            ((float *)val->data)[mapping->offset] = (float)dval;
        } else {
            // Integer property (8, 16, or 32 bit)
            char *endptr;
            long ival = strtol(value_str.c_str(), &endptr, 10);
            if (endptr == value_str.c_str()) {
                fprintf(stderr, "[CONFIG] Warning: invalid integer value '%s' for '%s'\n",
                        value_str.c_str(), name.c_str());
                continue;
            }

            switch (mapping->format) {
                case 8:
                    ((uint8_t *)val->data)[mapping->offset] = (uint8_t)ival;
                    break;
                case 16:
                    ((uint16_t *)val->data)[mapping->offset] = (uint16_t)ival;
                    break;
                case 32:
                    ((int32_t *)val->data)[mapping->offset] = (int32_t)ival;
                    break;
                default:
                    fprintf(stderr, "[CONFIG] Warning: unexpected format %d for '%s'\n",
                            mapping->format, name.c_str());
                    continue;
            }
        }

        waynaptics_call_set_property_handler(dev, prop_atom, val, FALSE);

        fprintf(stderr, "[CONFIG] %s = %s (property: %s[%d])\n",
                name.c_str(), value_str.c_str(), mapping->prop_name, mapping->offset);
    }

    return true;
}

extern "C" const char *waynaptics_apply_option(const char *name, const char *value, DeviceIntPtr dev) {
    static char errbuf[256];

    if (strcmp(name, "GrabEventDevice") == 0 || strcmp(name, "TouchpadOff") == 0) {
        snprintf(errbuf, sizeof(errbuf), "parameter '%s' is not supported", name);
        return errbuf;
    }

    const ParamMapping *mapping = find_param(name);
    if (!mapping) {
        snprintf(errbuf, sizeof(errbuf), "unknown parameter '%s'", name);
        return errbuf;
    }

    Atom prop_atom = XIGetKnownProperty(mapping->prop_name);
    if (prop_atom == 0) {
        snprintf(errbuf, sizeof(errbuf), "property '%s' not registered", mapping->prop_name);
        return errbuf;
    }

    XIPropertyValuePtr val = waynaptics_get_property_value(prop_atom);
    if (!val || !val->data) {
        snprintf(errbuf, sizeof(errbuf), "property '%s' has no value", mapping->prop_name);
        return errbuf;
    }

    if (mapping->format == 0) {
        char *endptr;
        double dval = strtod(value, &endptr);
        if (endptr == value) {
            snprintf(errbuf, sizeof(errbuf), "invalid float value '%s'", value);
            return errbuf;
        }
        ((float *)val->data)[mapping->offset] = (float)dval;
    } else {
        char *endptr;
        long ival = strtol(value, &endptr, 10);
        if (endptr == value) {
            snprintf(errbuf, sizeof(errbuf), "invalid integer value '%s'", value);
            return errbuf;
        }
        switch (mapping->format) {
            case 8:  ((uint8_t *)val->data)[mapping->offset] = (uint8_t)ival; break;
            case 16: ((uint16_t *)val->data)[mapping->offset] = (uint16_t)ival; break;
            case 32: ((int32_t *)val->data)[mapping->offset] = (int32_t)ival; break;
            default:
                snprintf(errbuf, sizeof(errbuf), "unexpected format %d", mapping->format);
                return errbuf;
        }
    }

    waynaptics_call_set_property_handler(dev, prop_atom, val, FALSE);
    return nullptr;
}

extern "C" void waynaptics_dump_config(void (*emit_line)(const char *line, void *ctx), void *ctx) {
    char buf[256];
    for (const ParamMapping *p = params; p->name != nullptr; p++) {
        Atom prop_atom = XIGetKnownProperty(p->prop_name);
        if (prop_atom == 0)
            continue;
        XIPropertyValuePtr val = waynaptics_get_property_value(prop_atom);
        if (!val || !val->data)
            continue;

        if (p->format == 0) {
            float fval = ((float *)val->data)[p->offset];
            snprintf(buf, sizeof(buf), "    %-24s = %g", p->name, fval);
        } else {
            long ival = 0;
            switch (p->format) {
                case 8:  ival = ((uint8_t *)val->data)[p->offset]; break;
                case 16: ival = ((uint16_t *)val->data)[p->offset]; break;
                case 32: ival = ((int32_t *)val->data)[p->offset]; break;
            }
            snprintf(buf, sizeof(buf), "    %-24s = %ld", p->name, ival);
        }
        emit_line(buf, ctx);
    }
}
