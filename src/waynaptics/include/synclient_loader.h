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

#ifdef __cplusplus
}
#endif
