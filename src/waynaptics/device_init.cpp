#include "shim.h"
#include "include/device_init.h"
#include "include/ptrveloc.h"
#include <cstring>
#include <cstdio>

static DeviceInitState g_device_init_state;

extern "C" {

typedef void (*PtrCtrlProcPtr)(DeviceIntPtr device, PtrCtrl *ctrl);
typedef double (*PointerAccelerationProfileFunc)(
    DeviceIntPtr dev, DeviceVelocityPtr vel,
    double velocity, double threshold, double accelCoeff);

/* Defined in ptrveloc.cpp */
extern void waynaptics_accel_set_profile(
    double (*profile)(DeviceIntPtr, DeviceVelocityPtr, double, double, double));

DeviceInitState *waynaptics_get_device_init_state(void)
{
    return &g_device_init_state;
}

Bool
InitPointerDeviceStruct(DevicePtr device, CARD8 *map, int numButtons,
                        Atom *btn_labels, PtrCtrlProcPtr controlProc,
                        int numMotionEvents, int numAxes, Atom *axes_labels)
{
    printf("InitPointerDeviceStruct: %d buttons, %d axes\n", numButtons, numAxes);
    return TRUE;
}

int
GetMotionHistorySize(void)
{
    return 256;
}

Bool
xf86InitValuatorAxisStruct(DeviceIntPtr dev, int axnum, Atom label,
                           int minval, int maxval, int resolution,
                           int min_res, int max_res, int mode)
{
    printf("xf86InitValuatorAxisStruct: axis %d, range [%d, %d], res %d, mode %d\n",
           axnum, minval, maxval, resolution, mode);
    return TRUE;
}

void
xf86InitValuatorDefaults(DeviceIntPtr dev, int axnum)
{
    /* no-op */
}

Bool
SetScrollValuator(DeviceIntPtr dev, int axnum, enum ScrollType type,
                  double increment, int flags)
{
    if (type == SCROLL_TYPE_NONE)
        return TRUE;

    if (increment == 0.0)
        return FALSE;

    DeviceInitState *state = &g_device_init_state;
    if (state->num_scroll_axes >= MAX_SCROLL_AXES)
        return FALSE;

    ScrollValuatorInfo *info = &state->scroll_axes[state->num_scroll_axes++];
    info->axis = axnum;
    info->type = type;
    info->increment = increment;
    info->flags = flags;

    printf("SetScrollValuator: axis %d, type %d, increment %.2f\n",
           axnum, (int)type, increment);
    return TRUE;
}

DeviceVelocityPtr
GetDevicePredictableAccelData(DeviceIntPtr dev)
{
    DeviceVelocityRec *vel = &g_device_init_state.velocity;
    /* Don't reset if already configured by the driver */
    if (vel->const_acceleration == 0.0)
        vel->const_acceleration = 1.0;
    if (vel->corr_mul == 0.0)
        vel->corr_mul = 10.0;
    return vel;
}

void
SetDeviceSpecificAccelerationProfile(DeviceVelocityPtr vel,
                                     PointerAccelerationProfileFunc profile)
{
    waynaptics_accel_set_profile(profile);
}

} /* extern "C" */
