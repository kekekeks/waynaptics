/*
 * waynaptics-dump: Read actual synaptics XI property values from the
 * running X server and dump them in a format compatible with the
 * waynaptics --config loader.
 *
 * Unlike `synclient -l` which can report stale cached values, this tool
 * queries XIGetDeviceProperty directly for every parameter.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xdefs.h>
#include <X11/Xatom.h>
#include <X11/extensions/XI.h>
#include <X11/extensions/XInput.h>
#include "synaptics-properties.h"

#ifndef XATOM_FLOAT
#define XATOM_FLOAT "FLOAT"
#endif

union flong {
    long l;
    float f;
};

enum ParaType { PT_INT, PT_BOOL, PT_DOUBLE };

struct Parameter {
    const char *name;
    enum ParaType type;
    const char *prop_name;
    int prop_format;    /* 8, 32, or 0 for float */
    int prop_offset;
};

/* Same parameter table as synclient_loader.cpp / synclient.c */
static struct Parameter params[] = {
    {"LeftEdge",              PT_INT,    SYNAPTICS_PROP_EDGES,                      32, 0},
    {"RightEdge",             PT_INT,    SYNAPTICS_PROP_EDGES,                      32, 1},
    {"TopEdge",               PT_INT,    SYNAPTICS_PROP_EDGES,                      32, 2},
    {"BottomEdge",            PT_INT,    SYNAPTICS_PROP_EDGES,                      32, 3},
    {"FingerLow",             PT_INT,    SYNAPTICS_PROP_FINGER,                     32, 0},
    {"FingerHigh",            PT_INT,    SYNAPTICS_PROP_FINGER,                     32, 1},
    {"MaxTapTime",            PT_INT,    SYNAPTICS_PROP_TAP_TIME,                   32, 0},
    {"MaxTapMove",            PT_INT,    SYNAPTICS_PROP_TAP_MOVE,                   32, 0},
    {"MaxDoubleTapTime",      PT_INT,    SYNAPTICS_PROP_TAP_DURATIONS,              32, 1},
    {"SingleTapTimeout",      PT_INT,    SYNAPTICS_PROP_TAP_DURATIONS,              32, 0},
    {"ClickTime",             PT_INT,    SYNAPTICS_PROP_TAP_DURATIONS,              32, 2},
    {"EmulateMidButtonTime",  PT_INT,    SYNAPTICS_PROP_MIDDLE_TIMEOUT,             32, 0},
    {"EmulateTwoFingerMinZ",  PT_INT,    SYNAPTICS_PROP_TWOFINGER_PRESSURE,         32, 0},
    {"EmulateTwoFingerMinW",  PT_INT,    SYNAPTICS_PROP_TWOFINGER_WIDTH,            32, 0},
    {"VertScrollDelta",       PT_INT,    SYNAPTICS_PROP_SCROLL_DISTANCE,            32, 0},
    {"HorizScrollDelta",      PT_INT,    SYNAPTICS_PROP_SCROLL_DISTANCE,            32, 1},
    {"VertEdgeScroll",        PT_BOOL,   SYNAPTICS_PROP_SCROLL_EDGE,                 8, 0},
    {"HorizEdgeScroll",       PT_BOOL,   SYNAPTICS_PROP_SCROLL_EDGE,                 8, 1},
    {"CornerCoasting",        PT_BOOL,   SYNAPTICS_PROP_SCROLL_EDGE,                 8, 2},
    {"VertTwoFingerScroll",   PT_BOOL,   SYNAPTICS_PROP_SCROLL_TWOFINGER,            8, 0},
    {"HorizTwoFingerScroll",  PT_BOOL,   SYNAPTICS_PROP_SCROLL_TWOFINGER,            8, 1},
    {"MinSpeed",              PT_DOUBLE, SYNAPTICS_PROP_SPEED,                       0, 0},
    {"MaxSpeed",              PT_DOUBLE, SYNAPTICS_PROP_SPEED,                       0, 1},
    {"AccelFactor",           PT_DOUBLE, SYNAPTICS_PROP_SPEED,                       0, 2},
    {"UpDownScrolling",       PT_BOOL,   SYNAPTICS_PROP_BUTTONSCROLLING,             8, 0},
    {"LeftRightScrolling",    PT_BOOL,   SYNAPTICS_PROP_BUTTONSCROLLING,             8, 1},
    {"UpDownScrollRepeat",    PT_BOOL,   SYNAPTICS_PROP_BUTTONSCROLLING_REPEAT,      8, 0},
    {"LeftRightScrollRepeat", PT_BOOL,   SYNAPTICS_PROP_BUTTONSCROLLING_REPEAT,      8, 1},
    {"ScrollButtonRepeat",    PT_INT,    SYNAPTICS_PROP_BUTTONSCROLLING_TIME,        32, 0},
    {"TouchpadOff",           PT_INT,    SYNAPTICS_PROP_OFF,                          8, 0},
    {"LockedDrags",           PT_BOOL,   SYNAPTICS_PROP_LOCKED_DRAGS,                8, 0},
    {"LockedDragTimeout",     PT_INT,    SYNAPTICS_PROP_LOCKED_DRAGS_TIMEOUT,        32, 0},
    {"RTCornerButton",        PT_INT,    SYNAPTICS_PROP_TAP_ACTION,                  8, 0},
    {"RBCornerButton",        PT_INT,    SYNAPTICS_PROP_TAP_ACTION,                  8, 1},
    {"LTCornerButton",        PT_INT,    SYNAPTICS_PROP_TAP_ACTION,                  8, 2},
    {"LBCornerButton",        PT_INT,    SYNAPTICS_PROP_TAP_ACTION,                  8, 3},
    {"TapButton1",            PT_INT,    SYNAPTICS_PROP_TAP_ACTION,                  8, 4},
    {"TapButton2",            PT_INT,    SYNAPTICS_PROP_TAP_ACTION,                  8, 5},
    {"TapButton3",            PT_INT,    SYNAPTICS_PROP_TAP_ACTION,                  8, 6},
    {"ClickFinger1",          PT_INT,    SYNAPTICS_PROP_CLICK_ACTION,                8, 0},
    {"ClickFinger2",          PT_INT,    SYNAPTICS_PROP_CLICK_ACTION,                8, 1},
    {"ClickFinger3",          PT_INT,    SYNAPTICS_PROP_CLICK_ACTION,                8, 2},
    {"CircularScrolling",     PT_BOOL,   SYNAPTICS_PROP_CIRCULAR_SCROLLING,          8, 0},
    {"CircScrollDelta",       PT_DOUBLE, SYNAPTICS_PROP_CIRCULAR_SCROLLING_DIST,     0, 0},
    {"CircScrollTrigger",     PT_INT,    SYNAPTICS_PROP_CIRCULAR_SCROLLING_TRIGGER,  8, 0},
    {"CircularPad",           PT_BOOL,   SYNAPTICS_PROP_CIRCULAR_PAD,                8, 0},
    {"PalmDetect",            PT_BOOL,   SYNAPTICS_PROP_PALM_DETECT,                 8, 0},
    {"PalmMinWidth",          PT_INT,    SYNAPTICS_PROP_PALM_DIMENSIONS,             32, 0},
    {"PalmMinZ",              PT_INT,    SYNAPTICS_PROP_PALM_DIMENSIONS,             32, 1},
    {"CoastingSpeed",         PT_DOUBLE, SYNAPTICS_PROP_COASTING_SPEED,              0, 0},
    {"CoastingFriction",      PT_DOUBLE, SYNAPTICS_PROP_COASTING_SPEED,              0, 1},
    {"PressureMotionMinZ",    PT_INT,    SYNAPTICS_PROP_PRESSURE_MOTION,             32, 0},
    {"PressureMotionMaxZ",    PT_INT,    SYNAPTICS_PROP_PRESSURE_MOTION,             32, 1},
    {"PressureMotionMinFactor",  PT_DOUBLE, SYNAPTICS_PROP_PRESSURE_MOTION_FACTOR,   0, 0},
    {"PressureMotionMaxFactor",  PT_DOUBLE, SYNAPTICS_PROP_PRESSURE_MOTION_FACTOR,   0, 1},
    {"GrabEventDevice",       PT_BOOL,   SYNAPTICS_PROP_GRAB,                        8, 0},
    {"TapAndDragGesture",     PT_BOOL,   SYNAPTICS_PROP_GESTURES,                    8, 0},
    {"AreaLeftEdge",          PT_INT,    SYNAPTICS_PROP_AREA,                        32, 0},
    {"AreaRightEdge",         PT_INT,    SYNAPTICS_PROP_AREA,                        32, 1},
    {"AreaTopEdge",           PT_INT,    SYNAPTICS_PROP_AREA,                        32, 2},
    {"AreaBottomEdge",        PT_INT,    SYNAPTICS_PROP_AREA,                        32, 3},
    {"HorizHysteresis",       PT_INT,    SYNAPTICS_PROP_NOISE_CANCELLATION,          32, 0},
    {"VertHysteresis",        PT_INT,    SYNAPTICS_PROP_NOISE_CANCELLATION,          32, 1},
    {"ClickPad",              PT_BOOL,   SYNAPTICS_PROP_CLICKPAD,                     8, 0},
    {"RightButtonAreaLeft",   PT_INT,    SYNAPTICS_PROP_SOFTBUTTON_AREAS,            32, 0},
    {"RightButtonAreaRight",  PT_INT,    SYNAPTICS_PROP_SOFTBUTTON_AREAS,            32, 1},
    {"RightButtonAreaTop",    PT_INT,    SYNAPTICS_PROP_SOFTBUTTON_AREAS,            32, 2},
    {"RightButtonAreaBottom", PT_INT,    SYNAPTICS_PROP_SOFTBUTTON_AREAS,            32, 3},
    {"MiddleButtonAreaLeft",  PT_INT,    SYNAPTICS_PROP_SOFTBUTTON_AREAS,            32, 4},
    {"MiddleButtonAreaRight", PT_INT,    SYNAPTICS_PROP_SOFTBUTTON_AREAS,            32, 5},
    {"MiddleButtonAreaTop",   PT_INT,    SYNAPTICS_PROP_SOFTBUTTON_AREAS,            32, 6},
    {"MiddleButtonAreaBottom",PT_INT,    SYNAPTICS_PROP_SOFTBUTTON_AREAS,            32, 7},
    {NULL,                    PT_INT,    NULL,                                        0, 0},
};

int main(int argc, char *argv[])
{
    Display *dpy;
    XDeviceInfo *info;
    int ndevices, i, j;
    Atom touchpad_type, synaptics_prop, float_type;
    XDevice *dev = NULL;
    Atom *properties;
    int nprops;
    int found_synaptics;

    (void)argc;
    (void)argv;

    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "Failed to connect to X server.\n");
        return 1;
    }

    touchpad_type = XInternAtom(dpy, XI_TOUCHPAD, True);
    if (!touchpad_type) {
        fprintf(stderr, "XI_TOUCHPAD not initialized. No touchpad?\n");
        XCloseDisplay(dpy);
        return 1;
    }

    synaptics_prop = XInternAtom(dpy, SYNAPTICS_PROP_EDGES, True);
    if (!synaptics_prop) {
        fprintf(stderr, "Synaptics properties not found. No synaptics driver loaded?\n");
        XCloseDisplay(dpy);
        return 1;
    }

    /* Find the synaptics touchpad device */
    info = XListInputDevices(dpy, &ndevices);
    for (i = 0; i < ndevices; i++) {
        if (info[i].type != touchpad_type)
            continue;

        dev = XOpenDevice(dpy, info[i].id);
        if (!dev)
            continue;

        properties = XListDeviceProperties(dpy, dev, &nprops);
        found_synaptics = 0;
        if (properties) {
            for (j = 0; j < nprops; j++) {
                if (properties[j] == synaptics_prop) {
                    found_synaptics = 1;
                    break;
                }
            }
            XFree(properties);
        }

        if (found_synaptics)
            break;

        XCloseDevice(dpy, dev);
        dev = NULL;
    }
    XFreeDeviceList(info);

    if (!dev) {
        fprintf(stderr, "No synaptics touchpad device found.\n");
        XCloseDisplay(dpy);
        return 1;
    }

    float_type = XInternAtom(dpy, XATOM_FLOAT, True);

    /* Dump all parameters by reading XI properties directly */
    for (j = 0; params[j].name; j++) {
        struct Parameter *par = &params[j];
        Atom a, type;
        int format;
        unsigned long nitems, bytes_after;
        unsigned char *data = NULL;
        int len;

        a = XInternAtom(dpy, par->prop_name, True);
        if (!a)
            continue;

        /* Calculate how many 32-bit units we need to fetch */
        len = 1 + ((par->prop_offset *
                    (par->prop_format ? par->prop_format : 32) / 8)) / 4;

        if (XGetDeviceProperty(dpy, dev, a, 0, len, False,
                               AnyPropertyType, &type, &format,
                               &nitems, &bytes_after, &data) != Success ||
            type == None) {
            if (data) XFree(data);
            continue;
        }

        switch (par->prop_format) {
        case 8:
            if (format != 8 || type != XA_INTEGER) {
                fprintf(stderr, "# %-23s = format mismatch\n", par->name);
                break;
            }
            printf("    %-23s = %d\n", par->name,
                   ((char *)data)[par->prop_offset]);
            break;

        case 32:
            if (format != 32 || (type != XA_INTEGER && type != XA_CARDINAL)) {
                fprintf(stderr, "# %-23s = format mismatch\n", par->name);
                break;
            }
            printf("    %-23s = %ld\n", par->name,
                   ((long *)data)[par->prop_offset]);
            break;

        case 0: /* float */
            if (!float_type || format != 32 || type != float_type) {
                fprintf(stderr, "# %-23s = format mismatch\n", par->name);
                break;
            }
            printf("    %-23s = %g\n", par->name,
                   ((union flong *)data)[par->prop_offset].f);
            break;
        }

        XFree(data);
    }

    XCloseDevice(dpy, dev);
    XCloseDisplay(dpy);
    return 0;
}
