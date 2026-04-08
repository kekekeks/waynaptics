#ifndef WAYNAPTICS_DEVICE_INIT_H
#define WAYNAPTICS_DEVICE_INIT_H

#include "synshared.h"

#define MAX_SCROLL_AXES 4

struct ScrollValuatorInfo {
    int axis;
    enum ScrollType type;
    double increment;
    int flags;
};

struct DeviceInitState {
    ScrollValuatorInfo scroll_axes[MAX_SCROLL_AXES];
    int num_scroll_axes;
    DeviceVelocityRec velocity;
};

DeviceInitState *waynaptics_get_device_init_state();

#endif
