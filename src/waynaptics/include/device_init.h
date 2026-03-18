#ifndef WAYNAPTICS_DEVICE_INIT_H
#define WAYNAPTICS_DEVICE_INIT_H

#include "synshared.h"

#define MAX_SCROLL_AXES 4

typedef struct ScrollValuatorInfo {
    int axis;
    enum ScrollType type;
    double increment;
    int flags;
} ScrollValuatorInfo;

typedef struct DeviceInitState {
    ScrollValuatorInfo scroll_axes[MAX_SCROLL_AXES];
    int num_scroll_axes;
    DeviceVelocityRec velocity;
} DeviceInitState;

#ifdef __cplusplus
extern "C" {
#endif

struct DeviceInitState *waynaptics_get_device_init_state(void);

#ifdef __cplusplus
}
#endif

#endif
