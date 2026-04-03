#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WAYNAPTICS_MAX_TOUCHES 10

struct WaynapticsTouchContact {
    int x, y, z;
    int active;
};

struct WaynapticsTouchState {
    int num_contacts;
    struct WaynapticsTouchContact contacts[WAYNAPTICS_MAX_TOUCHES];
};

typedef struct _DeviceIntRec *DeviceIntPtr;

// Read current touch state from the device.
// Returns true if state was successfully read.
bool waynaptics_get_touch_state(DeviceIntPtr dev, struct WaynapticsTouchState *state);

// Broadcast touch state to subscribed config sockets.
// Called from io_watch_callback after read_input.
void waynaptics_broadcast_touches(DeviceIntPtr dev);

#ifdef __cplusplus
}
#endif
