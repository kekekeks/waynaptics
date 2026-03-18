#include "synshared.h"
#include <string>
#include <map>
#include <stdexcept>
#include <cstring>
#include <strings.h>


class Options
{
public:
    std::map<std::string, std::string> options;

};

extern "C" {

const char *xf86CheckStrOption(XF86OptionPtr options, const char *name, const char *def) {
    auto &map = ((Options *) options)->options;
    auto found = map.find(name);
    if (found != map.end()) {
        return found->second.c_str();
    }
    return def;
}

int xf86SetIntOption(XF86OptionPtr options, const char *name, int def) {
    auto str = xf86CheckStrOption(options, name, nullptr);
    if (str) {
        try {
            return std::stoi(str);
        } catch (std::invalid_argument &) {
            return def;
        }
    }
    return def;
}
double xf86SetRealOption(XF86OptionPtr options, const char *name, double def)
{
    auto str = xf86CheckStrOption(options, name, nullptr);
    if (str) {
        try {
            return std::stod(str);
        } catch (std::invalid_argument &) {
            return def;
        }
    }
    return def;
}

int xf86SetBoolOption(XF86OptionPtr options, const char *name, int def) {
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

char *xf86SetStrOption(XF86OptionPtr options, const char *name, const char *def) {
    auto str = xf86CheckStrOption(options, name, nullptr);
    if (str)
        return strdup(str);
    if (def)
        return strdup(def);
    return nullptr;
}

double xf86CheckPercentOption(XF86OptionPtr options, const char *name, double def) {
    auto str = xf86CheckStrOption(options, name, nullptr);
    if (str) {
        size_t len = strlen(str);
        if (len > 0 && str[len - 1] == '%') {
            try {
                return std::stod(std::string(str, len - 1));
            } catch (std::invalid_argument &) {
                return def;
            }
        }
    }
    return def;
}

double xf86SetPercentOption(XF86OptionPtr options, const char *name, double def) {
    auto str = xf86CheckStrOption(options, name, nullptr);
    if (str) {
        size_t len = strlen(str);
        if (len > 0 && str[len - 1] == '%') {
            try {
                return std::stod(std::string(str, len - 1));
            } catch (std::invalid_argument &) {
                return def;
            }
        }
        try {
            return std::stod(str);
        } catch (std::invalid_argument &) {
            return def;
        }
    }
    return def;
}

XF86OptionPtr xf86ReplaceStrOption(XF86OptionPtr options, const char *name, const char *value) {
    auto &map = ((Options *) options)->options;
    map[name] = value;
    return options;
}

}