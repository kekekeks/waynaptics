#include "synshared.h"
#include "xi_properties.h"
#include "log.h"

#include <map>
#include <vector>
#include <optional>
#include <cstring>

struct PropertyData {
    Atom type;
    short format;
    unsigned long len; // element count
    std::vector<uint8_t> data;
    Bool deletable;

    XIPropertyValueRec as_value_rec() {
        return { type, format, static_cast<long>(len), data.data() };
    }
};

static std::map<Atom, PropertyData> property_store;

static int (*registered_set_property)(DeviceIntPtr, Atom, XIPropertyValuePtr, BOOL) = nullptr;

static size_t data_size_bytes(int format, unsigned long len) {
    if (format == 0)
        return len * sizeof(float);
    return len * (format / 8);
}

// Boundary functions — called by synaptics driver C code

extern "C" int XIChangeDeviceProperty(DeviceIntPtr dev, Atom property, Atom type,
                                      int format, int mode, unsigned long len,
                                      const void *value, Bool sendevent)
{
    (void)dev;
    (void)sendevent;

    if (mode != PropModeReplace)
        return Success;

    size_t nbytes = data_size_bytes(format, len);

    PropertyData &prop = property_store[property];
    prop.type = type;
    prop.format = static_cast<short>(format);
    prop.len = len;
    prop.data.resize(nbytes);
    if (nbytes > 0 && value)
        std::memcpy(prop.data.data(), value, nbytes);

    return Success;
}

extern "C" void XISetDevicePropertyDeletable(DeviceIntPtr dev, Atom property,
                                             Bool deletable)
{
    (void)dev;
    auto it = property_store.find(property);
    if (it != property_store.end())
        it->second.deletable = deletable;
}

extern "C" void XIDeleteDeviceProperty(DeviceIntPtr dev, Atom property, Bool sendevent)
{
    (void)dev;
    (void)sendevent;
    property_store.erase(property);
}

extern "C" int XIRegisterPropertyHandler(DeviceIntPtr dev,
                                         int (*SetProperty)(DeviceIntPtr, Atom,
                                                            XIPropertyValuePtr, BOOL),
                                         int (*GetProperty)(DeviceIntPtr, Atom),
                                         int (*DeleteProperty)(DeviceIntPtr, Atom))
{
    (void)dev;
    (void)GetProperty;
    (void)DeleteProperty;
    registered_set_property = SetProperty;
    return 1;
}

std::optional<PropertyValue> waynaptics_get_property_value(Atom property)
{
    auto it = property_store.find(property);
    if (it == property_store.end())
        return std::nullopt;

    auto &pd = it->second;
    PropertyValue pv;
    pv.type = pd.type;
    pv.format = pd.format;
    pv.size = static_cast<long>(pd.len);
    pv.data = pd.data;  // copy
    return pv;
}

int waynaptics_update_property(DeviceIntPtr dev, Atom property,
                               const std::vector<uint8_t> &data)
{
    auto it = property_store.find(property);
    if (it == property_store.end())
        return BadValue;

    if (data.size() != it->second.data.size()) {
        wlog("props", "Rejecting update for atom %lu: data size mismatch "
             "(store=%zu, provided=%zu)",
             static_cast<unsigned long>(property),
             it->second.data.size(), data.size());
        return BadValue;
    }

    it->second.data = data;

    if (!registered_set_property)
        return BadValue;
    XIPropertyValueRec val = it->second.as_value_rec();
    return registered_set_property(dev, property, &val, FALSE);
}
