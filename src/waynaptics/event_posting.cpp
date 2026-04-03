#include "include/synshared.h"
#include "include/output_backend.h"
#include "include/device_init.h"
#include "include/ptrveloc.h"

#include <cstdarg>
#include <cstdio>
#include <cmath>

bool g_verbose_mouse_events = false;

// Accumulate fractional remainders so sub-pixel motion isn't lost
static double g_frac_dx = 0.0;
static double g_frac_dy = 0.0;

extern "C" void
xf86PostMotionEvent(DeviceIntPtr dev, int is_absolute,
                    int first_valuator, int num_valuators, ...)
{
    if (!g_output_backend || num_valuators < 2)
        return;

    va_list args;
    va_start(args, num_valuators);
    int raw_dx = va_arg(args, int);
    int raw_dy = va_arg(args, int);
    va_end(args);

    double dx = raw_dx;
    double dy = raw_dy;

    /* Apply pointer acceleration (velocity tracking + profile) */
    waynaptics_accel_apply(dev, &dx, &dy);

    g_frac_dx += dx;
    g_frac_dy += dy;

    int out_dx = (int)trunc(g_frac_dx);
    int out_dy = (int)trunc(g_frac_dy);

    g_frac_dx -= out_dx;
    g_frac_dy -= out_dy;

    if (g_verbose_mouse_events)
        fprintf(stderr, "[MOUSE] motion raw=%d,%d accel=%d,%d\n",
                raw_dx, raw_dy, out_dx, out_dy);

    if (out_dx != 0 || out_dy != 0) {
        g_output_backend->post_motion(out_dx, out_dy);
        g_output_backend->sync();
    }
}

extern "C" void
xf86PostButtonEvent(DeviceIntPtr dev, int is_absolute,
                    int button, int is_down,
                    int first_valuator, int num_valuators, ...)
{
    if (!g_output_backend)
        return;

    if (g_verbose_mouse_events)
        fprintf(stderr, "[MOUSE] button %d %s\n", button, is_down ? "down" : "up");

    g_output_backend->post_button(button, is_down != 0);
    g_output_backend->sync();
}

extern "C" void
xf86PostMotionEventM(DeviceIntPtr dev, int is_absolute,
                     const ValuatorMask *mask)
{
    if (!g_output_backend)
        return;

    DeviceInitState *state = waynaptics_get_device_init_state();
    if (!state)
        return;

    for (int i = 0; i < state->num_scroll_axes; i++) {
        const ScrollValuatorInfo *info = &state->scroll_axes[i];
        if (!valuator_mask_isset(mask, info->axis))
            continue;

        double value = valuator_mask_get_double(mask, info->axis);

        if (g_verbose_mouse_events) {
            const char *axis_name =
                (info->type == SCROLL_TYPE_VERTICAL) ? "vert" : "horiz";
            fprintf(stderr, "[MOUSE] scroll %s value=%.2f increment=%.1f\n",
                    axis_name, value, info->increment);
        }

        g_output_backend->post_scroll(info->type, value, info->increment);
    }

    g_output_backend->sync();
}
