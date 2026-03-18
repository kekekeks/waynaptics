/*
 * Pointer acceleration implementation matching the X server's ptrveloc.c
 * algorithm. Tracks finger velocity over time and applies the
 * device-specific acceleration profile (SynapticsAccelerationProfile).
 */

#include "include/synshared.h"
#include "include/ptrveloc.h"
#include "include/device_init.h"
#include "include/xi_properties.h"

#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sys/time.h>

/* Direction bits for velocity consistency checking */
#define UNDEFINED 0x00
#define N         0x01
#define NE        0x02
#define E         0x04
#define SE        0x08
#define S         0x10
#define SW        0x20
#define W         0x40
#define NW        0x80

#define NUM_TRACKERS 16
#define RESET_TIME_MS 300

struct MotionTracker {
    double dx, dy;
    unsigned long time;  /* milliseconds */
    int dir;
};

/* Full acceleration state matching X server's DeviceVelocityRec */
struct AccelState {
    MotionTracker trackers[NUM_TRACKERS];
    int cur_tracker;

    double velocity;
    double last_velocity;
    double last_dx, last_dy;

    /* Config — set from DeviceVelocityRec passed by driver */
    double const_acceleration;
    double corr_mul;
    double min_acceleration;

    /* Tracker query config */
    double max_rel_diff;
    double max_diff;
    int initial_range;
    int reset_time;
    bool use_softening;
    bool average_accel;

    /* Profile function (set by synaptics driver) */
    double (*profile)(DeviceIntPtr dev, DeviceVelocityPtr vel,
                      double velocity, double threshold, double acc);

    bool initialized;
};

static AccelState g_accel;

static unsigned long get_millis(void)
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return (unsigned long)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

static int get_direction(double dx, double dy)
{
    /* Classify movement into octant direction bits */
    if (dx == 0.0 && dy == 0.0)
        return UNDEFINED;

    int dir = 0;
    /* Threshold for considering diagonal: tan(22.5°) ≈ 0.414 */
    double r = 0.414;

    if (dy < 0) {
        if (dx > -dy * r) dir |= E;
        if (-dx > -dy * r) dir |= W;
        dir |= N;
    }
    if (dy > 0) {
        if (dx > dy * r) dir |= E;
        if (-dx > dy * r) dir |= W;
        dir |= S;
    }
    if (dx > 0) {
        if (dy > dx * r) dir |= S;
        if (-dy > dx * r) dir |= N;
        dir |= E;
    }
    if (dx < 0) {
        if (dy > -dx * r) dir |= S;
        if (-dy > -dx * r) dir |= N;
        dir |= W;
    }
    return dir;
}

static void feed_trackers(double dx, double dy, unsigned long time)
{
    AccelState *s = &g_accel;
    int dir = get_direction(dx, dy);

    /* Accumulate into all existing trackers */
    for (int i = 0; i < NUM_TRACKERS; i++) {
        s->trackers[i].dx += dx;
        s->trackers[i].dy += dy;
    }

    /* Push a new tracker (reset current slot) */
    s->cur_tracker = (s->cur_tracker + 1) % NUM_TRACKERS;
    s->trackers[s->cur_tracker].dx = 0.0;
    s->trackers[s->cur_tracker].dy = 0.0;
    s->trackers[s->cur_tracker].time = time;
    s->trackers[s->cur_tracker].dir = dir;
}

static double query_trackers(unsigned long time)
{
    AccelState *s = &g_accel;
    double velocity = 0.0;
    bool found_initial = false;

    /* Walk trackers from newest to oldest, find consistent velocity */
    for (int offset = 1; offset < NUM_TRACKERS; offset++) {
        int idx = (s->cur_tracker - offset + NUM_TRACKERS) % NUM_TRACKERS;
        MotionTracker *t = &s->trackers[idx];

        unsigned long dt = time - t->time;
        if (dt == 0 || dt > (unsigned long)s->reset_time)
            break;

        /* Direction consistency check */
        int common_dir = s->trackers[s->cur_tracker].dir & t->dir;
        if (common_dir == 0 && offset > 1)
            break;

        double dist = sqrt(t->dx * t->dx + t->dy * t->dy);
        double tracker_vel = (dist / (double)dt) * s->corr_mul * s->const_acceleration;

        if (!found_initial) {
            if (offset >= s->initial_range) {
                velocity = tracker_vel;
                found_initial = true;
            }
        } else {
            /* Check if this older tracker's velocity is consistent */
            double diff = fabs(tracker_vel - velocity);
            if (diff > s->max_diff &&
                diff / fmax(velocity, tracker_vel) > s->max_rel_diff)
                break;
            velocity = tracker_vel;
        }
    }

    return velocity;
}

static void apply_softening(double *dx, double *dy)
{
    AccelState *s = &g_accel;
    if (s->last_dx != 0.0 || s->last_dy != 0.0) {
        if (*dx != 0.0) {
            if ((*dx > 0) == (s->last_dx > 0))
                *dx = (*dx + s->last_dx) * 0.5;
        }
        if (*dy != 0.0) {
            if ((*dy > 0) == (s->last_dy > 0))
                *dy = (*dy + s->last_dy) * 0.5;
        }
    }
    /* Store BEFORE scaling — matches X server's ApplySoftening */
    s->last_dx = *dx;
    s->last_dy = *dy;
}

extern "C" {

void waynaptics_accel_init(void)
{
    AccelState *s = &g_accel;

    /* Save profile set during DEVICE_INIT (before memset wipes it) */
    auto saved_profile = s->profile;
    memset(s, 0, sizeof(*s));
    s->profile = saved_profile;

    /* Copy config from device init state */
    DeviceInitState *dis = waynaptics_get_device_init_state();
    s->corr_mul = dis->velocity.corr_mul;  /* Set to 12.5 by driver */

    /*
     * The driver sets ACCEL_PROP_CONSTANT_DECELERATION to 1/min_speed
     * via XIChangeDeviceProperty. In the real X server, this triggers
     * vel->const_acceleration = 1/decel = min_speed.
     * We read that property value directly.
     */
    Atom decel_prop = XIGetKnownProperty(ACCEL_PROP_CONSTANT_DECELERATION);
    XIPropertyValuePtr val = waynaptics_get_property_value(decel_prop);
    if (val && val->data && val->format == 32 && val->size >= 1) {
        float decel = ((float *)val->data)[0];
        if (decel > 0.0f)
            s->const_acceleration = 1.0 / decel;
        else
            s->const_acceleration = 1.0;
    } else {
        s->const_acceleration = dis->velocity.const_acceleration;
    }

    fprintf(stderr, "(accel) const_acceleration=%.3f corr_mul=%.1f\n",
            s->const_acceleration, s->corr_mul);

    /* Defaults matching X server */
    s->min_acceleration = 1.0;
    s->max_rel_diff = 0.2;
    s->max_diff = 1.0;
    s->initial_range = 2;
    s->reset_time = RESET_TIME_MS;
    s->use_softening = true;
    s->average_accel = true;

    s->initialized = true;

    /* Initialize tracker times */
    unsigned long now = get_millis();
    for (int i = 0; i < NUM_TRACKERS; i++)
        s->trackers[i].time = now;
}

void waynaptics_accel_set_profile(
    double (*profile)(DeviceIntPtr, DeviceVelocityPtr, double, double, double))
{
    g_accel.profile = profile;
}

void waynaptics_accel_apply(DeviceIntPtr dev, double *dx, double *dy)
{
    AccelState *s = &g_accel;
    if (!s->initialized || !s->profile)
        return;

    if (*dx == 0.0 && *dy == 0.0)
        return;

    unsigned long now = get_millis();

    /* 1. Track velocity */
    s->last_velocity = s->velocity;
    feed_trackers(*dx, *dy, now);
    s->velocity = query_trackers(now);
    bool reset = (s->velocity == 0.0);

    /* 2. Compute acceleration multiplier from profile */
    /* The synaptics profile ignores threshold/acc in a meaningful way,
     * but we pass 0/1.0 which matches how X server sets up the PtrCtrl
     * for device-specific profiles (threshold=0, num=1, den=1 → acc=1.0) */
    DeviceVelocityRec vel_for_profile;
    vel_for_profile.const_acceleration = s->const_acceleration;
    vel_for_profile.corr_mul = s->corr_mul;

    double mult;
    if (s->average_accel && s->velocity != s->last_velocity) {
        /* Simpson's rule averaging */
        double f1 = s->profile(dev, &vel_for_profile, s->velocity, 0, 1.0);
        double f2 = s->profile(dev, &vel_for_profile, s->last_velocity, 0, 1.0);
        double f3 = s->profile(dev, &vel_for_profile,
                              (s->velocity + s->last_velocity) / 2.0, 0, 1.0);
        mult = (f1 + f2 + 4.0 * f3) / 6.0;
    } else {
        mult = s->profile(dev, &vel_for_profile, s->velocity, 0, 1.0);
    }

    if (mult < s->min_acceleration)
        mult = s->min_acceleration;

    /* 3. Softening (smooth jitter) */
    if (mult > 1.0 && s->use_softening && !reset)
        apply_softening(dx, dy);

    /* 4. Apply constant deceleration */
    *dx *= s->const_acceleration;
    *dy *= s->const_acceleration;

    /* 5. Scale by acceleration multiplier */
    *dx *= mult;
    *dy *= mult;
}

} /* extern "C" */
