#pragma once

#include "synshared.h"
#include <string>

// Start the config socket server on the GLib main loop.
// socket_path: path for the Unix domain socket (deleted and recreated)
// config_path: path to system config file for 'reload' command (empty if none)
// runtime_config_path: writable path for 'save' command (empty = falls back to config_path)
// dev: device pointer for applying property changes
void waynaptics_config_socket_start(const std::string &socket_path,
                                    const std::string &config_path,
                                    const std::string &runtime_config_path,
                                    DeviceIntPtr dev);

// Broadcast current touch contacts to subscribed socket clients.
void waynaptics_broadcast_touches(DeviceIntPtr dev);
