#pragma once

#include "synshared.h"

#ifdef __cplusplus
extern "C" {
#endif

// Start the config socket server on the GLib main loop.
// socket_path: path for the Unix domain socket (deleted and recreated)
// config_path: path to config file for 'save' command (may be NULL)
// dev: device pointer for applying property changes
void waynaptics_config_socket_start(const char *socket_path,
                                    const char *config_path,
                                    DeviceIntPtr dev);

#ifdef __cplusplus
}
#endif
