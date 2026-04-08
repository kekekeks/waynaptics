#include "synshared.h"
#include "options.h"
#include <string>
#include <map>
#include <stdexcept>
#include <cstring>
#include <cmath>
#include <strings.h>

extern "C" const char *xf86CheckStrOption(XF86OptionPtr options, const char *name, const char *def) {
    auto &map = static_cast<Options *>(options)->options;
    auto found = map.find(name);
    if (found != map.end()) {
        return found->second.c_str();
    }
    return def;
}

extern "C" int xf86SetIntOption(XF86OptionPtr options, const char *name, int def) {
    auto str = xf86CheckStrOption(options, name, nullptr);
    if (str) {
        try {
            return std::stoi(str);
        } catch (const std::exception &) {
            return def;
        }
    }
    return def;
}

extern "C" double xf86SetRealOption(XF86OptionPtr options, const char *name, double def)
{
    auto str = xf86CheckStrOption(options, name, nullptr);
    if (str) {
        try {
            return std::stod(str);
        } catch (const std::exception &) {
            return def;
        }
    }
    return def;
}

extern "C" int xf86SetBoolOption(XF86OptionPtr options, const char *name, int def) {
    auto str = xf86CheckStrOption(options, name, nullptr);
    if (str) {
        if (strcasecmp(str, "1") == 0 || strcasecmp(str, "on") == 0 ||
            strcasecmp(str, "yes") == 0 || strcasecmp(str, "true") == 0)
            return 1;
        if (strcasecmp(str, "0") == 0 || strcasecmp(str, "off") == 0 ||
            strcasecmp(str, "no") == 0 || strcasecmp(str, "false") == 0)
            return 0;
    }
    return def;
}

extern "C" char *xf86SetStrOption(XF86OptionPtr options, const char *name, const char *def) {
    auto str = xf86CheckStrOption(options, name, nullptr);
    if (str)
        return strdup(str);
    if (def)
        return strdup(def);
    return nullptr;
}

extern "C" double xf86CheckPercentOption(XF86OptionPtr options, const char *name, double def) {
    auto str = xf86CheckStrOption(options, name, nullptr);
    if (str) {
        size_t len = strlen(str);
        if (len > 0 && str[len - 1] == '%') {
            try {
                return std::stod(std::string(str, len - 1));
            } catch (const std::exception &) {
                return def;
            }
        }
    }
    return def;
}

extern "C" double xf86SetPercentOption(XF86OptionPtr options, const char *name, double def) {
    // First try percent format (e.g. "50%")
    double pct = xf86CheckPercentOption(options, name, NAN);
    if (!isnan(pct))
        return pct;

    // Fall back to plain numeric value
    auto str = xf86CheckStrOption(options, name, nullptr);
    if (str) {
        try {
            return std::stod(str);
        } catch (const std::exception &) {
            return def;
        }
    }
    return def;
}

extern "C" XF86OptionPtr xf86ReplaceStrOption(XF86OptionPtr options, const char *name, const char *value) {
    auto &map = static_cast<Options *>(options)->options;
    map[name] = value;
    return options;
}