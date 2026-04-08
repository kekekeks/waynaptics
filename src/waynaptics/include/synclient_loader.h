#pragma once

#include "synshared.h"

#include <string>
#include <optional>
#include <functional>

bool waynaptics_load_synclient_config(const std::string &path, DeviceIntPtr dev);

// Load base config + runtime config overlay (if different and exists).
// Restores to snapshot first, then applies both layers.
void waynaptics_reload_config(const std::string &config_path,
                              const std::string &runtime_config_path,
                              DeviceIntPtr dev);

// Apply a single "Name=value" option to the running driver.
// Returns std::nullopt on success, or an error message on failure.
std::optional<std::string> waynaptics_apply_option(const std::string &name,
                                                   const std::string &value,
                                                   DeviceIntPtr dev);

// Dump current config in synclient format via callback.
void waynaptics_dump_config(std::function<void(const std::string &)> emit_line);

// Snapshot all current parameter values (call after DEVICE_INIT, before config load).
void waynaptics_snapshot_initial_config();

// Restore all parameters to the initial snapshot values.
void waynaptics_restore_initial_config(DeviceIntPtr dev);
