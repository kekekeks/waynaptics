#include "fakeclude/shim.h"
#include "include/touch_state.h"
#include <synapticsstr.h>
#include <stdbool.h>
#include <string.h>
#include <linux/input.h>

#ifndef ABS_MT_TOOL_Y
#define ABS_MT_TOOL_Y 0x3d
#endif

#define ABS_MT_MIN ABS_MT_SLOT
#define ABS_MT_MAX ABS_MT_TOOL_Y
#define ABS_MT_CNT (ABS_MT_MAX - ABS_MT_MIN + 1)

// Mirror of eventcomm_proto_data (defined in eventcomm.c, not in a header)
struct waynaptics_proto_data {
    int need_grab; // BOOL
    int st_to_mt_offset[2];
    double st_to_mt_scale[2];
    int axis_map[ABS_MT_CNT];
    int cur_slot;
    ValuatorMask **last_mt_vals;
    int num_touches;
};

bool waynaptics_get_touch_state(DeviceIntPtr dev, struct WaynapticsTouchState *state) {
    memset(state, 0, sizeof(*state));

    if (!dev || !dev->public.devicePrivate)
        return false;

    InputInfoPtr pInfo = (InputInfoPtr)dev->public.devicePrivate;
    if (!pInfo->private)
        return false;

    SynapticsPrivate *priv = (SynapticsPrivate *)pInfo->private;

    int numFingers = (priv->hwState) ? priv->hwState->numFingers : 0;
    if (numFingers <= 0)
        return false;

    // Always report the single-touch position as contact 0
    state->contacts[0].x = priv->hwState->x;
    state->contacts[0].y = priv->hwState->y;
    state->contacts[0].z = priv->hwState->z;
    state->contacts[0].active = 1;
    state->num_contacts = 1;

    // If multitouch is available, read individual slots (capped to numFingers)
    if (numFingers > 1 && priv->has_touch && priv->proto_data && priv->num_slots > 0) {
        struct waynaptics_proto_data *proto_data =
            (struct waynaptics_proto_data *)priv->proto_data;

        if (proto_data->last_mt_vals) {
            int x_axis = proto_data->axis_map[ABS_MT_POSITION_X - ABS_MT_TOUCH_MAJOR];
            int y_axis = proto_data->axis_map[ABS_MT_POSITION_Y - ABS_MT_TOUCH_MAJOR];

            int count = 0;
            int max_slots = priv->num_slots;
            if (max_slots > WAYNAPTICS_MAX_TOUCHES)
                max_slots = WAYNAPTICS_MAX_TOUCHES;

            for (int i = 0; i < max_slots && count < numFingers; i++) {
                if (!proto_data->last_mt_vals[i])
                    continue;

                int x = 0, y = 0;
                bool has_x = false, has_y = false;

                if (x_axis >= 0)
                    has_x = valuator_mask_fetch(proto_data->last_mt_vals[i], x_axis, &x);
                if (y_axis >= 0)
                    has_y = valuator_mask_fetch(proto_data->last_mt_vals[i], y_axis, &y);

                if (has_x && has_y) {
                    state->contacts[count].x = x;
                    state->contacts[count].y = y;
                    state->contacts[count].z = priv->hwState->z;
                    state->contacts[count].active = 1;
                    count++;
                }
            }

            if (count > 0)
                state->num_contacts = count;
        }
    }

    return true;
}
