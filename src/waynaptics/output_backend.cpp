#include "include/output_backend.h"
#include "include/synshared.h"

#include <cstdio>
#include <cstring>
#include <cmath>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <linux/uinput.h>

#ifndef REL_WHEEL_HI_RES
#define REL_WHEEL_HI_RES 0x0b
#endif
#ifndef REL_HWHEEL_HI_RES
#define REL_HWHEEL_HI_RES 0x0c
#endif

OutputBackend *g_output_backend = nullptr;

// ---------------------------------------------------------------------------
// UinputBackend
// ---------------------------------------------------------------------------

class UinputBackend : public OutputBackend {
public:
    bool hires_scroll = true;
    bool lores_scroll = true;
    bool init() override;
    void destroy() override;
    void post_motion(int dx, int dy) override;
    void post_button(int button, bool is_down) override;
    void post_scroll(int scroll_type, double value, double scroll_increment) override;
    void sync() override;

private:
    void emit(uint16_t type, uint16_t code, int32_t value);

    int fd_ = -1;
    double accum_vert_ = 0.0;
    double accum_horiz_ = 0.0;
};

bool UinputBackend::init() {
    fd_ = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd_ < 0) {
        fprintf(stderr, "UinputBackend: failed to open /dev/uinput: %s\n", strerror(errno));
        return false;
    }

    ioctl(fd_, UI_SET_EVBIT, EV_REL);
    ioctl(fd_, UI_SET_EVBIT, EV_KEY);
    ioctl(fd_, UI_SET_EVBIT, EV_SYN);

    ioctl(fd_, UI_SET_RELBIT, REL_X);
    ioctl(fd_, UI_SET_RELBIT, REL_Y);
    if (lores_scroll) {
        ioctl(fd_, UI_SET_RELBIT, REL_WHEEL);
        ioctl(fd_, UI_SET_RELBIT, REL_HWHEEL);
    }
    if (hires_scroll) {
        ioctl(fd_, UI_SET_RELBIT, REL_WHEEL_HI_RES);
        ioctl(fd_, UI_SET_RELBIT, REL_HWHEEL_HI_RES);
    }

    ioctl(fd_, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(fd_, UI_SET_KEYBIT, BTN_RIGHT);
    ioctl(fd_, UI_SET_KEYBIT, BTN_MIDDLE);

    struct uinput_setup setup{};
    snprintf(setup.name, UINPUT_MAX_NAME_SIZE, "waynaptics virtual pointer");
    setup.id.bustype = BUS_VIRTUAL;
    setup.id.vendor = 0x1234;
    setup.id.product = 0x5678;
    setup.id.version = 1;

    if (ioctl(fd_, UI_DEV_SETUP, &setup) < 0) {
        fprintf(stderr, "UinputBackend: UI_DEV_SETUP failed: %s\n", strerror(errno));
        close(fd_);
        fd_ = -1;
        return false;
    }

    if (ioctl(fd_, UI_DEV_CREATE) < 0) {
        fprintf(stderr, "UinputBackend: UI_DEV_CREATE failed: %s\n", strerror(errno));
        close(fd_);
        fd_ = -1;
        return false;
    }

    fprintf(stderr, "UinputBackend: device created\n");
    return true;
}

void UinputBackend::destroy() {
    if (fd_ >= 0) {
        ioctl(fd_, UI_DEV_DESTROY);
        close(fd_);
        fd_ = -1;
    }
}

void UinputBackend::emit(uint16_t type, uint16_t code, int32_t value) {
    struct input_event ev{};
    ev.type = type;
    ev.code = code;
    ev.value = value;
    write(fd_, &ev, sizeof(ev));
}

void UinputBackend::post_motion(int dx, int dy) {
    if (dx != 0)
        emit(EV_REL, REL_X, dx);
    if (dy != 0)
        emit(EV_REL, REL_Y, dy);
    sync();
}

void UinputBackend::post_button(int button, bool is_down) {
    uint16_t code;
    switch (button) {
        case 1: code = BTN_LEFT;   break;
        case 2: code = BTN_MIDDLE; break;
        case 3: code = BTN_RIGHT;  break;
        default:
            fprintf(stderr, "UinputBackend: unknown button %d\n", button);
            return;
    }
    emit(EV_KEY, code, is_down ? 1 : 0);
    sync();
}

void UinputBackend::post_scroll(int scroll_type, double value, double scroll_increment) {
    if (scroll_increment == 0.0)
        return;

    // Convert driver valuator delta to hi-res units (120 per detent)
    double hi_res = (value / scroll_increment) * 120.0;

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

    sync();
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
    void destroy() override;
    void post_motion(int dx, int dy) override;
    void post_button(int button, bool is_down) override;
    void post_scroll(int scroll_type, double value, double scroll_increment) override;
    void sync() override;
};

bool DryBackend::init() {
    fprintf(stderr, "Dry mode: no uinput device created\n");
    return true;
}

void DryBackend::destroy() {}

void DryBackend::post_motion(int dx, int dy) {
    fprintf(stderr, "[DRY] motion dx=%d dy=%d\n", dx, dy);
}

void DryBackend::post_button(int button, bool is_down) {
    fprintf(stderr, "[DRY] button %d %s\n", button, is_down ? "down" : "up");
}

void DryBackend::post_scroll(int scroll_type, double value, double scroll_increment) {
    const char *axis = (scroll_type == SCROLL_TYPE_VERTICAL) ? "vert" : "horiz";
    fprintf(stderr, "[DRY] scroll %s value=%.1f (increment=%.0f)\n", axis, value, scroll_increment);
}

void DryBackend::sync() {}

// Factory functions for main.cpp
OutputBackend *waynaptics_create_uinput_backend(bool hires_scroll, bool lores_scroll) {
    auto *b = new UinputBackend();
    b->hires_scroll = hires_scroll;
    b->lores_scroll = lores_scroll;
    return b;
}
OutputBackend *waynaptics_create_dry_backend() { return new DryBackend(); }
