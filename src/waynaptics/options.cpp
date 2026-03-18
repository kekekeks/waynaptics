#include "synshared.h"
#include <string>
#include <map>
#include <stdexcept>


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
    return nullptr;
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
}