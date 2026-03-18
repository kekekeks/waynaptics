#ifndef WAYNAPTICS_XI_PROPERTIES_H
#define WAYNAPTICS_XI_PROPERTIES_H

#include "synshared.h"

#ifdef __cplusplus
extern "C" {
#endif

int XIChangeDeviceProperty(DeviceIntPtr dev, Atom property, Atom type,
                           int format, int mode, unsigned long len,
                           const void *value, Bool sendevent);

void XISetDevicePropertyDeletable(DeviceIntPtr dev, Atom property,
                                  Bool deletable);

void XIDeleteDeviceProperty(DeviceIntPtr dev, Atom property, Bool sendevent);

int XIRegisterPropertyHandler(DeviceIntPtr dev,
                              int (*SetProperty)(DeviceIntPtr, Atom,
                                                 XIPropertyValuePtr, BOOL),
                              int (*GetProperty)(DeviceIntPtr, Atom),
                              int (*DeleteProperty)(DeviceIntPtr, Atom));

/* Get the XIPropertyValue for a given property atom, or NULL if not found. */
XIPropertyValuePtr waynaptics_get_property_value(Atom property);

/* Invoke the registered SetProperty handler for the given property.
   Returns Success (0) on success, or an X error code. */
int waynaptics_call_set_property_handler(DeviceIntPtr dev, Atom property,
                                         XIPropertyValuePtr val,
                                         BOOL checkonly);

#ifdef __cplusplus
}
#endif

#endif
