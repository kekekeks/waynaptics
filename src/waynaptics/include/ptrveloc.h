#ifndef WAYNAPTICS_PTRVELOC_H
#define WAYNAPTICS_PTRVELOC_H

#include "synshared.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the acceleration state (call after DeviceInit) */
void waynaptics_accel_init(void);

/* Apply acceleration to raw dx/dy, returns accelerated values */
void waynaptics_accel_apply(DeviceIntPtr dev, double *dx, double *dy);

#ifdef __cplusplus
}
#endif

#endif
