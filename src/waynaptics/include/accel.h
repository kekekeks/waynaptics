#pragma once

#include "synshared.h"

void waynaptics_accel_init(void);
void waynaptics_accel_set_profile(
    double (*profile)(DeviceIntPtr, DeviceVelocityPtr, double, double, double));
void waynaptics_accel_apply(DeviceIntPtr dev, double &dx, double &dy);
