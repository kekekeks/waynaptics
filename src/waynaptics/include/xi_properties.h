#ifndef WAYNAPTICS_XI_PROPERTIES_H
#define WAYNAPTICS_XI_PROPERTIES_H

#include "synshared.h"
#include <vector>
#include <optional>
#include <cstring>

// Owned copy of a property's value, safe to hold on the stack.
// Uses memcpy-based accessors to avoid strict-aliasing violations.
struct PropertyValue {
    Atom type = 0;
    short format = 0;
    long size = 0;  // element count
    std::vector<uint8_t> data;

    explicit operator bool() const { return !data.empty(); }

    template<typename T>
    T read(int offset) const {
        size_t byte_offset = offset * sizeof(T);
        if (byte_offset + sizeof(T) > data.size())
            return T{};
        T result{};
        std::memcpy(&result, data.data() + byte_offset, sizeof(T));
        return result;
    }

    template<typename T>
    bool write(int offset, T value) {
        size_t byte_offset = offset * sizeof(T);
        if (byte_offset + sizeof(T) > data.size())
            return false;
        std::memcpy(data.data() + byte_offset, &value, sizeof(T));
        return true;
    }
};

// Boundary functions — called by synaptics driver C code
extern "C" int XIChangeDeviceProperty(DeviceIntPtr dev, Atom property, Atom type,
                                      int format, int mode, unsigned long len,
                                      const void *value, Bool sendevent);

extern "C" void XISetDevicePropertyDeletable(DeviceIntPtr dev, Atom property,
                                             Bool deletable);

extern "C" void XIDeleteDeviceProperty(DeviceIntPtr dev, Atom property, Bool sendevent);

extern "C" int XIRegisterPropertyHandler(DeviceIntPtr dev,
                                         int (*SetProperty)(DeviceIntPtr, Atom,
                                                            XIPropertyValuePtr, BOOL),
                                         int (*GetProperty)(DeviceIntPtr, Atom),
                                         int (*DeleteProperty)(DeviceIntPtr, Atom));

// Waynaptics-internal — C++ linkage
std::optional<PropertyValue> waynaptics_get_property_value(Atom property);

// Write property data back to the store and notify the driver.
// Only accepts the data bytes — format/type/size are immutable.
int waynaptics_update_property(DeviceIntPtr dev, Atom property,
                               const std::vector<uint8_t> &data);

#endif
