#include "output_backend.h"
#include "unique_fd.h"
#include "synshared.h"
#include "log.h"

#include <cstring>
#include <cmath>
#include <cerrno>
#include <fcntl.h>
#include <linux/uinput.h>

#ifndef REL_WHEEL_HI_RES
#define REL_WHEEL_HI_RES 0x0b
#endif
#ifndef REL_HWHEEL_HI_RES
#define REL_HWHEEL_HI_RES 0x0c
#endif

std::unique_ptr<OutputBackend> g_output_backend;

// ---------------------------------------------------------------------------
// UinputBackend
// ---------------------------------------------------------------------------

class UinputBackend : public OutputBackend {
public:
    bool hires_scroll = true;
    bool lores_scroll = true;
    bool emulate_scrollpoint = false;
    double scroll_factor = 1.0;

    ~UinputBackend() override {
        if (fd_) {
            ioctl(fd_.get(), UI_DEV_DESTROY);
        }
    }

    bool init() override;
    void post_motion(int dx, int dy) override;
    void post_button(int button, bool is_down) override;
    void post_scroll(int scroll_type, double value, double scroll_increment) override;
    void sync() override;

private:
    void emit(uint16_t type, uint16_t code, int32_t value);

    UniqueFd fd_;
    double accum_vert_ = 0.0;
    double accum_horiz_ = 0.0;
};

bool UinputBackend::init() {
    fd_.reset(open("/dev/uinput", O_WRONLY | O_NONBLOCK));
    if (!fd_) {
        wlog_errno("uinput", "failed to open /dev/uinput");
        return false;
    }

    if (ioctl(fd_.get(), UI_SET_EVBIT, EV_REL) < 0 ||
        ioctl(fd_.get(), UI_SET_EVBIT, EV_KEY) < 0 ||
        ioctl(fd_.get(), UI_SET_EVBIT, EV_SYN) < 0) {
        wlog_errno("uinput", "failed to set event type capabilities");
        fd_.reset();
        return false;
    }

    if (ioctl(fd_.get(), UI_SET_RELBIT, REL_X) < 0 ||
        ioctl(fd_.get(), UI_SET_RELBIT, REL_Y) < 0) {
        wlog_errno("uinput", "failed to set REL_X/REL_Y capabilities");
        fd_.reset();
        return false;
    }
    if (lores_scroll) {
        if (ioctl(fd_.get(), UI_SET_RELBIT, REL_WHEEL) < 0 ||
            ioctl(fd_.get(), UI_SET_RELBIT, REL_HWHEEL) < 0)
            wlog_errno("uinput", "warning: failed to set lo-res scroll capabilities");
    }
    if (hires_scroll) {
        if (ioctl(fd_.get(), UI_SET_RELBIT, REL_WHEEL_HI_RES) < 0 ||
            ioctl(fd_.get(), UI_SET_RELBIT, REL_HWHEEL_HI_RES) < 0)
            wlog_errno("uinput", "warning: failed to set hi-res scroll capabilities");

        struct uinput_abs_setup abs_setup{};
        abs_setup.absinfo.minimum = -32767;
        abs_setup.absinfo.maximum = 32767;
        abs_setup.absinfo.resolution = 120;

        abs_setup.code = REL_WHEEL_HI_RES;
        if (ioctl(fd_.get(), UI_ABS_SETUP, &abs_setup) < 0)
            wlog_errno("uinput", "warning: UI_ABS_SETUP REL_WHEEL_HI_RES failed");
        abs_setup.code = REL_HWHEEL_HI_RES;
        if (ioctl(fd_.get(), UI_ABS_SETUP, &abs_setup) < 0)
            wlog_errno("uinput", "warning: UI_ABS_SETUP REL_HWHEEL_HI_RES failed");
    }

    if (ioctl(fd_.get(), UI_SET_KEYBIT, BTN_LEFT) < 0 ||
        ioctl(fd_.get(), UI_SET_KEYBIT, BTN_RIGHT) < 0 ||
        ioctl(fd_.get(), UI_SET_KEYBIT, BTN_MIDDLE) < 0) {
        wlog_errno("uinput", "failed to set button capabilities");
        fd_.reset();
        return false;
    }

    struct uinput_setup setup{};
    snprintf(setup.name, UINPUT_MAX_NAME_SIZE, "waynaptics virtual pointer");
    if (emulate_scrollpoint) {
        setup.id.bustype = BUS_USB;
        setup.id.vendor = 0x17EF;
        setup.id.product = 0x6049;
    } else {
        setup.id.bustype = BUS_VIRTUAL;
        setup.id.vendor = 0x1234;
        setup.id.product = 0x5678;
    }
    setup.id.version = 1;

    if (ioctl(fd_.get(), UI_DEV_SETUP, &setup) < 0) {
        wlog_errno("uinput", "UI_DEV_SETUP failed");
        fd_.reset();
        return false;
    }

    if (ioctl(fd_.get(), UI_DEV_CREATE) < 0) {
        wlog_errno("uinput", "UI_DEV_CREATE failed");
        fd_.reset();
        return false;
    }

    wlog("uinput", "device created");
    return true;
}

void UinputBackend::emit(uint16_t type, uint16_t code, int32_t value) {
    struct input_event ev{};
    ev.type = type;
    ev.code = code;
    ev.value = value;
    write(fd_.get(), &ev, sizeof(ev));
}

void UinputBackend::post_motion(int dx, int dy) {
    if (dx != 0)
        emit(EV_REL, REL_X, dx);
    if (dy != 0)
        emit(EV_REL, REL_Y, dy);
}

void UinputBackend::post_button(int button, bool is_down) {
    uint16_t code;
    switch (button) {
        case 1: code = BTN_LEFT;   break;
        case 2: code = BTN_MIDDLE; break;
        case 3: code = BTN_RIGHT;  break;
        default:
            wlog("uinput", "unknown button %d", button);
            return;
    }
    emit(EV_KEY, code, is_down ? 1 : 0);
}

void UinputBackend::post_scroll(int scroll_type, double value, double scroll_increment) {
    if (scroll_increment == 0.0)
        return;

    // Convert driver valuator delta to hi-res units (120 per detent), apply scroll factor
    double hi_res = (value / scroll_increment) * 120.0 * scroll_factor;

    bool vertical = (scroll_type == SCROLL_TYPE_VERTICAL);
    uint16_t hi_res_code = vertical ? REL_WHEEL_HI_RES : REL_HWHEEL_HI_RES;
    uint16_t legacy_code = vertical ? REL_WHEEL : REL_HWHEEL;
    double &accum = vertical ? accum_vert_ : accum_horiz_;

    // Vertical scroll is inverted in Linux: negative = scroll down
    if (vertical)
        hi_res = -hi_res;

    int hi_res_int = static_cast<int>(hi_res);
    if (hires_scroll && hi_res_int != 0)
        emit(EV_REL, hi_res_code, hi_res_int);

    // Accumulate for legacy discrete events (one click per 120 hi-res units)
    accum += hi_res;
    if (lores_scroll) {
        int clicks = static_cast<int>(accum / 120.0);
        if (clicks != 0) {
            emit(EV_REL, legacy_code, clicks);
            accum -= clicks * 120.0;
        }
    }
}

void UinputBackend::sync() {
    emit(EV_SYN, SYN_REPORT, 0);
}

// ---------------------------------------------------------------------------
// DryBackend
// ---------------------------------------------------------------------------

class DryBackend : public OutputBackend {
public:
    bool init() override;
    void post_motion(int dx, int dy) override;
    void post_button(int button, bool is_down) override;
    void post_scroll(int scroll_type, double value, double scroll_increment) override;
    void sync() override;
};

bool DryBackend::init() {
    wlog("dry", "no uinput device created");
    return true;
}

void DryBackend::post_motion(int dx, int dy) {
    wlog("dry", "motion dx=%d dy=%d", dx, dy);
}

void DryBackend::post_button(int button, bool is_down) {
    wlog("dry", "button %d %s", button, is_down ? "down" : "up");
}

void DryBackend::post_scroll(int scroll_type, double value, double scroll_increment) {
    const char *axis = (scroll_type == SCROLL_TYPE_VERTICAL) ? "vert" : "horiz";
    wlog("dry", "scroll %s value=%.1f (increment=%.0f)", axis, value, scroll_increment);
}

void DryBackend::sync() {}

// Factory functions for main.cpp
std::unique_ptr<OutputBackend> waynaptics_create_uinput_backend(
    bool hires_scroll, bool lores_scroll, bool emulate_scrollpoint, double scroll_factor)
{
    auto b = std::make_unique<UinputBackend>();
    b->hires_scroll = hires_scroll;
    b->lores_scroll = lores_scroll;
    b->emulate_scrollpoint = emulate_scrollpoint;
    b->scroll_factor = scroll_factor;
    return b;
}

std::unique_ptr<OutputBackend> waynaptics_create_dry_backend() {
    return std::make_unique<DryBackend>();
}
