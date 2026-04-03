#pragma once

#include "synshared.h"

#ifdef __cplusplus
extern "C" {
#endif

bool waynaptics_preload_synclient_options(const char *path, XF86OptionPtr opts_ptr);
bool waynaptics_load_synclient_config(const char *path, DeviceIntPtr dev);

// Apply a single "Name=value" option to the running driver.
// Returns NULL on success, or an error message string on failure.
const char *waynaptics_apply_option(const char *name, const char *value, DeviceIntPtr dev);

// Dump current config in synclient format into the provided callback.
// The callback is called once per line (without trailing newline).
void waynaptics_dump_config(void (*emit_line)(const char *line, void *ctx), void *ctx);

// Get touchpad axis dimensions.
// Returns false if dimensions are not available.
bool waynaptics_get_dimensions(DeviceIntPtr dev, int *minx, int *maxx, int *miny, int *maxy);

// Snapshot all current parameter values (call after DEVICE_INIT, before config load).
void waynaptics_snapshot_initial_config(void);

// Restore all parameters to the initial snapshot values.
void waynaptics_restore_initial_config(DeviceIntPtr dev);

#ifdef __cplusplus
}
#endif
