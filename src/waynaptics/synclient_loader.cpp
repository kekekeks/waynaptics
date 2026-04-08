#include "synshared.h"
#include "xi_properties.h"
#include "synclient_loader.h"
#include "log.h"
#include <synaptics-properties.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <map>
#include <optional>
#include <functional>
#include <stdexcept>

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

static const ParamMapping *find_param(const std::string &name) {
    for (const ParamMapping *p = params; p->name != nullptr; p++) {
        if (name == p->name)
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

// Parse a string value and write it into the correct offset of a PropertyValue
// based on the mapping's format. Returns error string on failure, nullopt on success.
static std::optional<std::string> parse_and_write(const ParamMapping *mapping,
                                                  const std::string &value_str,
                                                  PropertyValue &val) {
    if (mapping->format == 0) {
        double dval;
        try {
            dval = std::stod(value_str);
        } catch (const std::exception &) {
            return std::string("invalid float value '") + value_str + "'";
        }
        val.write<float>(mapping->offset, static_cast<float>(dval));
    } else {
        long ival;
        try {
            ival = std::stol(value_str);
        } catch (const std::exception &) {
            return std::string("invalid integer value '") + value_str + "'";
        }
        switch (mapping->format) {
            case 8:  val.write<uint8_t>(mapping->offset, static_cast<uint8_t>(ival)); break;
            case 16: val.write<uint16_t>(mapping->offset, static_cast<uint16_t>(ival)); break;
            case 32: val.write<int32_t>(mapping->offset, static_cast<int32_t>(ival)); break;
            default:
                return "unexpected format " + std::to_string(mapping->format);
        }
    }
    return std::nullopt;
}

bool waynaptics_load_synclient_config(const std::string &path, DeviceIntPtr dev) {
    std::ifstream file(path);
    if (!file.is_open()) {
        wlog("config", "Failed to open config file: %s", path.c_str());
        return false;
    }

    std::string line;
    int line_num = 0;

    while (std::getline(file, line)) {
        line_num++;
        std::string trimmed = trim(line);

        if (trimmed.empty() || trimmed == "Parameter settings:")
            continue;

        size_t eq_pos = trimmed.find('=');
        if (eq_pos == std::string::npos) {
            wlog("config", "Warning: skipping malformed line %d: %s",
                 line_num, trimmed.c_str());
            continue;
        }

        std::string name = trim(trimmed.substr(0, eq_pos));
        std::string value_str = trim(trimmed.substr(eq_pos + 1));

        if (name.empty() || value_str.empty())
            continue;

        auto err = waynaptics_apply_option(name, value_str, dev);
        if (err) {
            wlog("config", "Warning: %s (line %d)", err->c_str(), line_num);
            continue;
        }

        wlog("config", "%s = %s", name.c_str(), value_str.c_str());
    }

    return true;
}

std::optional<std::string> waynaptics_apply_option(const std::string &name, const std::string &value, DeviceIntPtr dev) {
    if (name == "GrabEventDevice" || name == "TouchpadOff")
        return std::string("parameter '") + name + "' is not supported";

    const ParamMapping *mapping = find_param(name);
    if (!mapping)
        return std::string("unknown parameter '") + name + "'";

    Atom prop_atom = XIGetKnownProperty(mapping->prop_name);
    if (prop_atom == 0)
        return std::string("property '") + mapping->prop_name + "' not registered";

    auto val = waynaptics_get_property_value(prop_atom);
    if (!val)
        return std::string("property '") + mapping->prop_name + "' has no value";

    if (mapping->offset >= val->size)
        return std::string("property '") + mapping->prop_name + "' offset out of bounds";

    auto err = parse_and_write(mapping, value, *val);
    if (err)
        return err;

    waynaptics_update_property(dev, prop_atom, val->data);
    return std::nullopt;
}

void waynaptics_dump_config(std::function<void(const std::string &)> emit_line) {
    char buf[256];
    for (const ParamMapping *p = params; p->name != nullptr; p++) {
        Atom prop_atom = XIGetKnownProperty(p->prop_name);
        if (prop_atom == 0)
            continue;
        auto val = waynaptics_get_property_value(prop_atom);
        if (!val)
            continue;
        if (p->offset >= val->size) {
            wlog("config", "Warning: dump: property '%s' offset %d out of bounds (size %ld)",
                 p->prop_name, p->offset, val->size);
            continue;
        }

        if (p->format == 0) {
            float fval = val->read<float>(p->offset);
            snprintf(buf, sizeof(buf), "    %-24s = %g", p->name, fval);
        } else {
            long ival = 0;
            switch (p->format) {
                case 8:  ival = val->read<uint8_t>(p->offset); break;
                case 16: ival = val->read<uint16_t>(p->offset); break;
                case 32: ival = val->read<int32_t>(p->offset); break;
            }
            snprintf(buf, sizeof(buf), "    %-24s = %ld", p->name, ival);
        }
        emit_line(buf);
    }
}

// Initial config snapshot: stores name→value pairs captured after DEVICE_INIT
static std::map<std::string, std::string> g_initial_config;

static void snapshot_emit(const std::string &line) {
    auto eq = line.find('=');
    if (eq == std::string::npos)
        return;
    std::string name = trim(line.substr(0, eq));
    std::string value = trim(line.substr(eq + 1));
    g_initial_config[name] = value;
}

void waynaptics_snapshot_initial_config() {
    g_initial_config.clear();
    waynaptics_dump_config(snapshot_emit);
    wlog("config", "Snapshot: captured %zu initial parameter values",
         g_initial_config.size());
}

void waynaptics_restore_initial_config(DeviceIntPtr dev) {
    wlog("config", "Restoring %zu initial parameter values",
         g_initial_config.size());
    for (const auto &[name, value] : g_initial_config) {
        waynaptics_apply_option(name, value, dev);
    }
}

void waynaptics_reload_config(const std::string &config_path,
                              const std::string &runtime_config_path,
                              DeviceIntPtr dev) {
    waynaptics_restore_initial_config(dev);

    if (!config_path.empty()) {
        if (!waynaptics_load_synclient_config(config_path, dev))
            wlog("config", "warning: failed to load config '%s'", config_path.c_str());
    }

    if (!runtime_config_path.empty() && runtime_config_path != config_path) {
        if (access(runtime_config_path.c_str(), R_OK) == 0) {
            if (!waynaptics_load_synclient_config(runtime_config_path, dev))
                wlog("config", "warning: failed to load runtime config '%s'",
                     runtime_config_path.c_str());
            else
                wlog("config", "loaded runtime config overrides from '%s'",
                     runtime_config_path.c_str());
        }
    }
}
