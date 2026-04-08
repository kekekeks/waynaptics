#include "synshared.h"
#include "device_init.h"
#include "accel.h"
#include "log.h"
#include <cstring>

static DeviceInitState g_device_init_state;

DeviceInitState *waynaptics_get_device_init_state()
{
    return &g_device_init_state;
}

extern "C" Bool
InitPointerDeviceStruct(DevicePtr device, CARD8 *map, int numButtons,
                        Atom *btn_labels, PtrCtrlProcPtr controlProc,
                        int numMotionEvents, int numAxes, Atom *axes_labels)
{
    wlog("init", "InitPointerDeviceStruct: %d buttons, %d axes", numButtons, numAxes);
    return TRUE;
}

extern "C" int
GetMotionHistorySize(void)
{
    return 256;
}

extern "C" Bool
xf86InitValuatorAxisStruct(DeviceIntPtr dev, int axnum, Atom label,
                           int minval, int maxval, int resolution,
                           int min_res, int max_res, int mode)
{
    wlog("init", "xf86InitValuatorAxisStruct: axis %d, range [%d, %d], res %d, mode %d",
         axnum, minval, maxval, resolution, mode);
    return TRUE;
}

extern "C" void
xf86InitValuatorDefaults(DeviceIntPtr dev, int axnum)
{
    /* no-op */
}

extern "C" Bool
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

    wlog("init", "SetScrollValuator: axis %d, type %d, increment %.2f",
         axnum, static_cast<int>(type), increment);
    return TRUE;
}

extern "C" DeviceVelocityPtr
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

extern "C" void
SetDeviceSpecificAccelerationProfile(DeviceVelocityPtr vel,
                                     PointerAccelerationProfileFunc profile)
{
    waynaptics_accel_set_profile(profile);
}
