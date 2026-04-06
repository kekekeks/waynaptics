#pragma once

#include "synshared.h"

#ifdef __cplusplus
extern "C" {
#endif

// Start the config socket server on the GLib main loop.
// socket_path: path for the Unix domain socket (deleted and recreated)
// config_path: path to system config file for 'reload' command (may be NULL)
// runtime_config_path: writable path for 'save' command (may be NULL; falls back to config_path)
// dev: device pointer for applying property changes
void waynaptics_config_socket_start(const char *socket_path,
                                    const char *config_path,
                                    const char *runtime_config_path,
                                    DeviceIntPtr dev);

#ifdef __cplusplus
}
#endif
