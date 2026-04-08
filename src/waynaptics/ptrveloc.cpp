/*
 * Pointer acceleration implementation matching the X server's ptrveloc.c
 * algorithm. Tracks finger velocity over time and applies the
 * device-specific acceleration profile (SynapticsAccelerationProfile).
 */

#include "synshared.h"
#include "accel.h"
#include "device_init.h"
#include "xi_properties.h"
#include "log.h"

#include <cmath>

// Direction bits for velocity consistency checking (used as bitmask)
namespace Dir {
    constexpr int Undefined = 0x00;
    constexpr int N         = 0x01;
    constexpr int NE        = 0x02;
    constexpr int E         = 0x04;
    constexpr int SE        = 0x08;
    constexpr int S         = 0x10;
    constexpr int SW        = 0x20;
    constexpr int W         = 0x40;
    constexpr int NW        = 0x80;
}

static constexpr int NUM_TRACKERS = 16;
static constexpr int RESET_TIME_MS = 300;

struct MotionTracker {
    double dx = 0.0, dy = 0.0;
    unsigned long time = 0;
    int dir = 0;
};

/* Full acceleration state matching X server's DeviceVelocityRec */
struct AccelState {
    MotionTracker trackers[NUM_TRACKERS]{};
    int cur_tracker = 0;

    double velocity = 0.0;
    double last_velocity = 0.0;
    double last_dx = 0.0, last_dy = 0.0;

    /* Config — set from DeviceVelocityRec passed by driver */
    double const_acceleration = 0.0;
    double corr_mul = 0.0;
    double min_acceleration = 0.0;

    /* Tracker query config */
    double max_rel_diff = 0.0;
    double max_diff = 0.0;
    int initial_range = 0;
    int reset_time = 0;
    bool use_softening = false;
    bool average_accel = false;

    /* Profile function (set by synaptics driver) */
    double (*profile)(DeviceIntPtr dev, DeviceVelocityPtr vel,
                      double velocity, double threshold, double acc) = nullptr;

    bool initialized = false;
};

static AccelState g_accel;

static int get_direction(double dx, double dy)
{
    if (dx == 0.0 && dy == 0.0)
        return Dir::Undefined;

    int dir = 0;
    /* Threshold for considering diagonal: tan(22.5°) ≈ 0.414 */
    double r = 0.414;

    if (dy < 0) {
        if (dx > -dy * r) dir |= Dir::E;
        if (-dx > -dy * r) dir |= Dir::W;
        dir |= Dir::N;
    }
    if (dy > 0) {
        if (dx > dy * r) dir |= Dir::E;
        if (-dx > dy * r) dir |= Dir::W;
        dir |= Dir::S;
    }
    if (dx > 0) {
        if (dy > dx * r) dir |= Dir::S;
        if (-dy > dx * r) dir |= Dir::N;
        dir |= Dir::E;
    }
    if (dx < 0) {
        if (dy > -dx * r) dir |= Dir::S;
        if (-dy > -dx * r) dir |= Dir::N;
        dir |= Dir::W;
    }
    return dir;
}

static void feed_trackers(double dx, double dy, unsigned long time)
{
    auto &s = g_accel;
    int dir = get_direction(dx, dy);

    for (int i = 0; i < NUM_TRACKERS; i++) {
        s.trackers[i].dx += dx;
        s.trackers[i].dy += dy;
    }

    s.cur_tracker = (s.cur_tracker + 1) % NUM_TRACKERS;
    s.trackers[s.cur_tracker].dx = 0.0;
    s.trackers[s.cur_tracker].dy = 0.0;
    s.trackers[s.cur_tracker].time = time;
    s.trackers[s.cur_tracker].dir = dir;
}

static double query_trackers(unsigned long time)
{
    auto &s = g_accel;
    double velocity = 0.0;
    bool found_initial = false;

    for (int offset = 1; offset < NUM_TRACKERS; offset++) {
        int idx = (s.cur_tracker - offset + NUM_TRACKERS) % NUM_TRACKERS;
        auto &t = s.trackers[idx];

        unsigned long dt = time - t.time;
        if (dt == 0 || dt > static_cast<unsigned long>(s.reset_time))
            break;

        int common_dir = s.trackers[s.cur_tracker].dir & t.dir;
        if (common_dir == 0 && offset > 1)
            break;

        double dist = sqrt(t.dx * t.dx + t.dy * t.dy);
        double tracker_vel = (dist / static_cast<double>(dt)) * s.corr_mul * s.const_acceleration;

        if (!found_initial) {
            if (offset >= s.initial_range) {
                velocity = tracker_vel;
                found_initial = true;
            }
        } else {
            double diff = fabs(tracker_vel - velocity);
            if (diff > s.max_diff &&
                diff / fmax(velocity, tracker_vel) > s.max_rel_diff)
                break;
            velocity = tracker_vel;
        }
    }

    return velocity;
}

static void apply_softening(double &dx, double &dy)
{
    auto &s = g_accel;
    if (s.last_dx != 0.0 || s.last_dy != 0.0) {
        if (dx != 0.0) {
            if ((dx > 0) == (s.last_dx > 0))
                dx = (dx + s.last_dx) * 0.5;
        }
        if (dy != 0.0) {
            if ((dy > 0) == (s.last_dy > 0))
                dy = (dy + s.last_dy) * 0.5;
        }
    }
    /* Store BEFORE scaling — matches X server's ApplySoftening */
    s.last_dx = dx;
    s.last_dy = dy;
}

void waynaptics_accel_init()
{
    auto saved_profile = g_accel.profile;
    g_accel = AccelState{};
    g_accel.profile = saved_profile;

    auto &s = g_accel;

    DeviceInitState *dis = waynaptics_get_device_init_state();
    s.corr_mul = dis->velocity.corr_mul;

    /*
     * The driver sets ACCEL_PROP_CONSTANT_DECELERATION to 1/min_speed
     * via XIChangeDeviceProperty. In the real X server, this triggers
     * vel->const_acceleration = 1/decel = min_speed.
     * We read that property value directly.
     */
    Atom decel_prop = XIGetKnownProperty(ACCEL_PROP_CONSTANT_DECELERATION);
    auto val = waynaptics_get_property_value(decel_prop);
    if (val && val->format == 32 && val->size >= 1) {
        float decel = val->read<float>(0);
        if (decel > 0.0f)
            s.const_acceleration = 1.0 / decel;
        else
            s.const_acceleration = 1.0;
    } else {
        s.const_acceleration = dis->velocity.const_acceleration;
    }

    wlog("accel", "const_acceleration=%.3f corr_mul=%.1f",
         s.const_acceleration, s.corr_mul);

    s.min_acceleration = 1.0;
    s.max_rel_diff = 0.2;
    s.max_diff = 1.0;
    s.initial_range = 2;
    s.reset_time = RESET_TIME_MS;
    s.use_softening = true;
    s.average_accel = true;

    s.initialized = true;

    unsigned long now = GetTimeInMillis();
    for (int i = 0; i < NUM_TRACKERS; i++)
        s.trackers[i].time = now;
}

void waynaptics_accel_set_profile(
    double (*profile)(DeviceIntPtr, DeviceVelocityPtr, double, double, double))
{
    g_accel.profile = profile;
}

void waynaptics_accel_apply(DeviceIntPtr dev, double &dx, double &dy)
{
    auto &s = g_accel;
    if (!s.initialized || !s.profile)
        return;

    if (dx == 0.0 && dy == 0.0)
        return;

    unsigned long now = GetTimeInMillis();

    /* 1. Track velocity */
    s.last_velocity = s.velocity;
    feed_trackers(dx, dy, now);
    s.velocity = query_trackers(now);
    bool reset = (s.velocity == 0.0);

    /* 2. Compute acceleration multiplier from profile */
    DeviceVelocityRec vel_for_profile;
    vel_for_profile.const_acceleration = s.const_acceleration;
    vel_for_profile.corr_mul = s.corr_mul;

    double mult;
    if (s.average_accel && s.velocity != s.last_velocity) {
        /* Simpson's rule averaging */
        double f1 = s.profile(dev, &vel_for_profile, s.velocity, 0, 1.0);
        double f2 = s.profile(dev, &vel_for_profile, s.last_velocity, 0, 1.0);
        double f3 = s.profile(dev, &vel_for_profile,
                              (s.velocity + s.last_velocity) / 2.0, 0, 1.0);
        mult = (f1 + f2 + 4.0 * f3) / 6.0;
    } else {
        mult = s.profile(dev, &vel_for_profile, s.velocity, 0, 1.0);
    }

    if (mult < s.min_acceleration)
        mult = s.min_acceleration;

    /* 3. Softening (smooth jitter) */
    if (mult > 1.0 && s.use_softening && !reset)
        apply_softening(dx, dy);

    /* 4. Apply constant deceleration */
    dx *= s.const_acceleration;
    dy *= s.const_acceleration;

    /* 5. Scale by acceleration multiplier */
    dx *= mult;
    dy *= mult;
}
