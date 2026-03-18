#include "synshared.h"
#include "xi_properties.h"

#include <map>
#include <vector>
#include <cstring>

struct PropertyData {
    Atom type;
    short format;
    unsigned long len; // element count
    std::vector<uint8_t> data;
    Bool deletable;
};

static std::map<Atom, PropertyData> property_store;

static int (*registered_set_property)(DeviceIntPtr, Atom, XIPropertyValuePtr, BOOL) = nullptr;

static size_t data_size_bytes(int format, unsigned long len) {
    if (format == 0)
        return len * sizeof(float);
    return len * (format / 8);
}

extern "C" {

int XIChangeDeviceProperty(DeviceIntPtr dev, Atom property, Atom type,
                           int format, int mode, unsigned long len,
                           const void *value, Bool sendevent)
{
    (void)dev;
    (void)sendevent;

    if (mode != PropModeReplace)
        return Success; // only PropModeReplace is used by the driver

    size_t nbytes = data_size_bytes(format, len);

    PropertyData &prop = property_store[property];
    prop.type = type;
    prop.format = (short)format;
    prop.len = len;
    prop.data.resize(nbytes);
    if (nbytes > 0 && value)
        std::memcpy(prop.data.data(), value, nbytes);

    return Success;
}

void XISetDevicePropertyDeletable(DeviceIntPtr dev, Atom property,
                                  Bool deletable)
{
    (void)dev;
    auto it = property_store.find(property);
    if (it != property_store.end())
        it->second.deletable = deletable;
}

void XIDeleteDeviceProperty(DeviceIntPtr dev, Atom property, Bool sendevent)
{
    (void)dev;
    (void)sendevent;
    property_store.erase(property);
}

int XIRegisterPropertyHandler(DeviceIntPtr dev,
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

XIPropertyValuePtr waynaptics_get_property_value(Atom property)
{
    auto it = property_store.find(property);
    if (it == property_store.end())
        return nullptr;

    PropertyData &pd = it->second;
    // Build a static XIPropertyValueRec to return; valid until next call
    static XIPropertyValueRec val;
    val.type = pd.type;
    val.format = pd.format;
    val.size = (long)pd.len;
    val.data = pd.data.data();
    return &val;
}

int waynaptics_call_set_property_handler(DeviceIntPtr dev, Atom property,
                                         XIPropertyValuePtr val,
                                         BOOL checkonly)
{
    if (!registered_set_property)
        return BadValue;
    return registered_set_property(dev, property, val, checkonly);
}

} /* extern "C" */
