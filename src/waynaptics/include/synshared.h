#ifndef WAYNAPTICS_ROOT_SYNCHARED_H
#define WAYNAPTICS_ROOT_SYNCHARED_H


#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#include "X11/X.h"
#include "X11/Xproto.h"
#include "X11/Xlib.h"
#include "X11/extensions/XI.h"
#define MAX_VALUATORS 36

struct _ValuatorMask {
    int8_t last_bit;            /* highest bit set in mask */
    int8_t has_unaccelerated;
    uint8_t mask[(MAX_VALUATORS + 7) / 8];
    double valuators[MAX_VALUATORS];    /* valuator data */
    double unaccelerated[MAX_VALUATORS];    /* valuator data */
};

typedef struct _ValuatorMask ValuatorMask;

typedef void* OsTimerPtr;
typedef void* ClientPtr;

typedef void XISBuffer;
typedef void xDeviceCtl;
typedef void* XF86OptionPtr;
typedef void InputAttributes;
typedef void* pointer;
typedef void PtrCtrl;


typedef struct _XIPropertyValue {
    Atom type;                  /* ignored by server */
    short format;               /* format of data for swapping - 8,16,32 */
    long size;                  /* size of data in (format/8) bytes */
    void *data;                 /* private to client */
} XIPropertyValueRec, *XIPropertyValuePtr;


typedef struct _DeviceVelocityRec
{
    double const_acceleration;
    double corr_mul;
} DeviceVelocityRec, *DeviceVelocityPtr;;

typedef struct _DeviceRec
{
    int on;
    void* devicePrivate;
} DeviceRec, *DevicePtr;

typedef struct _DeviceIntRec {

#ifdef __cplusplus
    DeviceRec pub;
#else
    DeviceRec public;
#endif
} DeviceIntRec, *DeviceIntPtr;

/* This holds the input driver entry and module information. */
typedef struct _InputDriverRec {
    int driverVersion;
    const char *driverName;
    void (*Identify) (int flags);
    int (*PreInit) (struct _InputDriverRec * drv,
                    struct _InputInfoRec * pInfo, int flags);
    void (*UnInit) (struct _InputDriverRec * drv,
                    struct _InputInfoRec * pInfo, int flags);
    void *module;
    const char **default_options;
    int capabilities;
} InputDriverRec, *InputDriverPtr;

typedef struct _InputInfoRec {
    struct _InputInfoRec *next;
    char *name;
    char *driver;

    int flags;

    Bool (*device_control) (DeviceIntPtr device, int what);
    void (*read_input) (struct _InputInfoRec * local);
    int (*control_proc) (struct _InputInfoRec * local, xDeviceCtl * control);
    int (*switch_mode) (ClientPtr client, DeviceIntPtr dev, int mode);
    int (*set_device_valuators)
            (struct _InputInfoRec * local,
             int *valuators, int first_valuator, int num_valuators);

    int fd;
    int major;
    int minor;
    DeviceIntPtr dev;
#ifdef __cplusplus
    void *priv;
#else
    void* private;
#endif
    const char *type_name;
    InputDriverPtr drv;
    void *module;
    XF86OptionPtr options;
    InputAttributes *attrs;
} InputInfoRec, *InputInfoPtr;



typedef void *(*ModuleSetupProc) (void *, void *, int *, int *);
typedef void (*ModuleTearDownProc) (void *);

typedef struct {
    void *vers;
    ModuleSetupProc setup;
    ModuleTearDownProc teardown;
} XF86ModuleData;

/* Flags for log messages. */
typedef enum {
    X_PROBED,                   /* Value was probed */
    X_CONFIG,                   /* Value was given in the config file */
    X_DEFAULT,                  /* Value is a default */
    X_CMDLINE,                  /* Value was given on the command line */
    X_NOTICE,                   /* Notice */
    X_ERROR,                    /* Error message */
    X_WARNING,                  /* Warning message */
    X_INFO,                     /* Informational message */
    X_NONE,                     /* No prefix */
    X_NOT_IMPLEMENTED,          /* Not implemented */
    X_DEBUG,                    /* Debug message */
    X_UNKNOWN = -1              /* unknown -- this must always be last */
} MessageType;


#define DEVICE_INIT	0
#define DEVICE_ON	1
#define DEVICE_OFF	2
#define DEVICE_CLOSE	3
#define DEVICE_ABORT	4


/* Type for a 4 byte float. Storage format IEEE 754 in client's default
 * byte-ordering. */
#define XATOM_FLOAT "FLOAT"

/* Pointer acceleration properties */
/* INTEGER of any format */
#define ACCEL_PROP_PROFILE_NUMBER "Device Accel Profile"
/* FLOAT, format 32 */
#define ACCEL_PROP_CONSTANT_DECELERATION "Device Accel Constant Deceleration"
/* FLOAT, format 32 */
#define ACCEL_PROP_ADAPTIVE_DECELERATION "Device Accel Adaptive Deceleration"
/* FLOAT, format 32 */
#define ACCEL_PROP_VELOCITY_SCALING "Device Accel Velocity Scaling"

#define AXIS_LABEL_PROP_REL_X           "Rel X"
#define AXIS_LABEL_PROP_REL_Y           "Rel Y"
#define AXIS_LABEL_PROP_REL_Z           "Rel Z"
#define AXIS_LABEL_PROP_REL_RX          "Rel Rotary X"
#define AXIS_LABEL_PROP_REL_RY          "Rel Rotary Y"
#define AXIS_LABEL_PROP_REL_RZ          "Rel Rotary Z"
#define AXIS_LABEL_PROP_REL_HWHEEL      "Rel Horiz Wheel"
#define AXIS_LABEL_PROP_REL_DIAL        "Rel Dial"
#define AXIS_LABEL_PROP_REL_WHEEL       "Rel Vert Wheel"
#define AXIS_LABEL_PROP_REL_MISC        "Rel Misc"
#define AXIS_LABEL_PROP_REL_VSCROLL     "Rel Vert Scroll"
#define AXIS_LABEL_PROP_REL_HSCROLL     "Rel Horiz Scroll"

/* Default label */
#define BTN_LABEL_PROP_BTN_UNKNOWN      "Button Unknown"
/* Wheel buttons */
#define BTN_LABEL_PROP_BTN_WHEEL_UP     "Button Wheel Up"
#define BTN_LABEL_PROP_BTN_WHEEL_DOWN   "Button Wheel Down"
#define BTN_LABEL_PROP_BTN_HWHEEL_LEFT  "Button Horiz Wheel Left"
#define BTN_LABEL_PROP_BTN_HWHEEL_RIGHT "Button Horiz Wheel Right"


#define BTN_LABEL_PROP_BTN_LEFT         "Button Left"
#define BTN_LABEL_PROP_BTN_RIGHT        "Button Right"
#define BTN_LABEL_PROP_BTN_MIDDLE       "Button Middle"
#define BTN_LABEL_PROP_BTN_SIDE         "Button Side"
#define BTN_LABEL_PROP_BTN_EXTRA        "Button Extra"
#define BTN_LABEL_PROP_BTN_FORWARD      "Button Forward"
#define BTN_LABEL_PROP_BTN_BACK         "Button Back"
#define BTN_LABEL_PROP_BTN_TASK         "Button Task"

/*
 * Absolute axes
 */

#define AXIS_LABEL_PROP_ABS_X           "Abs X"
#define AXIS_LABEL_PROP_ABS_Y           "Abs Y"
#define AXIS_LABEL_PROP_ABS_Z           "Abs Z"
#define AXIS_LABEL_PROP_ABS_RX          "Abs Rotary X"
#define AXIS_LABEL_PROP_ABS_RY          "Abs Rotary Y"
#define AXIS_LABEL_PROP_ABS_RZ          "Abs Rotary Z"
#define AXIS_LABEL_PROP_ABS_THROTTLE    "Abs Throttle"
#define AXIS_LABEL_PROP_ABS_RUDDER      "Abs Rudder"
#define AXIS_LABEL_PROP_ABS_WHEEL       "Abs Wheel"
#define AXIS_LABEL_PROP_ABS_GAS         "Abs Gas"
#define AXIS_LABEL_PROP_ABS_BRAKE       "Abs Brake"
#define AXIS_LABEL_PROP_ABS_HAT0X       "Abs Hat 0 X"
#define AXIS_LABEL_PROP_ABS_HAT0Y       "Abs Hat 0 Y"
#define AXIS_LABEL_PROP_ABS_HAT1X       "Abs Hat 1 X"
#define AXIS_LABEL_PROP_ABS_HAT1Y       "Abs Hat 1 Y"
#define AXIS_LABEL_PROP_ABS_HAT2X       "Abs Hat 2 X"
#define AXIS_LABEL_PROP_ABS_HAT2Y       "Abs Hat 2 Y"
#define AXIS_LABEL_PROP_ABS_HAT3X       "Abs Hat 3 X"
#define AXIS_LABEL_PROP_ABS_HAT3Y       "Abs Hat 3 Y"
#define AXIS_LABEL_PROP_ABS_PRESSURE    "Abs Pressure"
#define AXIS_LABEL_PROP_ABS_DISTANCE    "Abs Distance"
#define AXIS_LABEL_PROP_ABS_TILT_X      "Abs Tilt X"
#define AXIS_LABEL_PROP_ABS_TILT_Y      "Abs Tilt Y"
#define AXIS_LABEL_PROP_ABS_TOOL_WIDTH  "Abs Tool Width"
#define AXIS_LABEL_PROP_ABS_VOLUME      "Abs Volume"
#define AXIS_LABEL_PROP_ABS_MT_TOUCH_MAJOR "Abs MT Touch Major"
#define AXIS_LABEL_PROP_ABS_MT_TOUCH_MINOR "Abs MT Touch Minor"
#define AXIS_LABEL_PROP_ABS_MT_WIDTH_MAJOR "Abs MT Width Major"
#define AXIS_LABEL_PROP_ABS_MT_WIDTH_MINOR "Abs MT Width Minor"
#define AXIS_LABEL_PROP_ABS_MT_ORIENTATION "Abs MT Orientation"
#define AXIS_LABEL_PROP_ABS_MT_POSITION_X  "Abs MT Position X"
#define AXIS_LABEL_PROP_ABS_MT_POSITION_Y  "Abs MT Position Y"
#define AXIS_LABEL_PROP_ABS_MT_TOOL_TYPE   "Abs MT Tool Type"
#define AXIS_LABEL_PROP_ABS_MT_BLOB_ID     "Abs MT Blob ID"
#define AXIS_LABEL_PROP_ABS_MT_TRACKING_ID "Abs MT Tracking ID"
#define AXIS_LABEL_PROP_ABS_MT_PRESSURE    "Abs MT Pressure"
#define AXIS_LABEL_PROP_ABS_MT_DISTANCE    "Abs MT Distance"
#define AXIS_LABEL_PROP_ABS_MT_TOOL_X      "Abs MT Tool X"
#define AXIS_LABEL_PROP_ABS_MT_TOOL_Y      "Abs MT Tool Y"
#define AXIS_LABEL_PROP_ABS_MISC        "Abs Misc"


enum ScrollType {
    SCROLL_TYPE_NONE = 0,           /**< Not a scrolling valuator */
    SCROLL_TYPE_VERTICAL = 8,
    SCROLL_TYPE_HORIZONTAL = 9,
};

#define AccelProfileNone -1
#define AccelProfileClassic  0
#define AccelProfileDeviceSpecific 1
#define AccelProfilePolynomial 2
#define AccelProfileSmoothLinear 3
#define AccelProfileSimple 4
#define AccelProfilePower 5
#define AccelProfileLinear 6
#define AccelProfileSmoothLimited 7
#define AccelProfileLAST AccelProfileSmoothLimited

#ifdef __cplusplus
extern "C" {
#endif


typedef CARD32 (*OsTimerCallback) (OsTimerPtr timer,
                                   CARD32 time,
                                   void *arg);

extern CARD32 GetTimeInMillis(void);
extern CARD64 GetTimeInMicros(void);
extern _X_EXPORT OsTimerPtr TimerSet(OsTimerPtr timer,
                                     int flags,
                                     CARD32 millis,
                                     OsTimerCallback func,
                                     void *arg);

extern _X_EXPORT void TimerCancel(OsTimerPtr /* pTimer */ );
extern _X_EXPORT void TimerFree(OsTimerPtr /* pTimer */ );

int xf86FlushInput(int fd);

Atom XIGetKnownProperty(const char* name);
#define MakeAtom(name, _, __) XIGetKnownProperty(name);
const char* NameForAtom(Atom atom);
int xf86SetIntOption(XF86OptionPtr options, const char* name, int def);
double xf86SetRealOption(XF86OptionPtr options, const char* name, double def);
const char* xf86CheckStrOption(XF86OptionPtr options, const char* name, const char* def);
#define xf86FindOptionValue(options, name) xf86CheckStrOption(options, name, NULL)
// No-op
#define input_lock()
#define input_unlock()

// STUB?
#define XisbNew(fd, size) (XISBuffer *)1
#define XisbFree(x)
#define xf86ProcessCommonOptions(a,b)

//STUB
#define xorg_backtrace()

// Redirect to printf
#define xf86IDrvMsg(dev, type, format, ...) printf(format, ##__VA_ARGS__)
#define xf86ErrorFVerb(what, format, ...) printf(format, ##__VA_ARGS__)

// We disable signal handlers, so redirect to printf
#define LogMessageVerbSigSafe(type, wat, format, ...) printf(format, ##__VA_ARGS__)
#define ErrorFSigSafe(format, ...) printf(format, ##__VA_ARGS__)

#define __BUG_WARN_MSG(cond, with_msg, ...)                                \
          do { if (cond) {                                                \
              ErrorFSigSafe("BUG: triggered 'if (" #cond ")'\n");          \
              ErrorFSigSafe("BUG: %s:%u in %s()\n",                        \
                           __FILE__, __LINE__, __func__);                 \
              if (with_msg) ErrorFSigSafe(__VA_ARGS__);                    \
              xorg_backtrace();                                           \
          } } while(0)

#define BUG_WARN_MSG(cond, ...)                                           \
          __BUG_WARN_MSG(cond, 1, __VA_ARGS__)


// Valuator masks
extern _X_EXPORT ValuatorMask *valuator_mask_new(int num_valuators);
extern _X_EXPORT void valuator_mask_free(ValuatorMask **mask);
extern _X_EXPORT void valuator_mask_set_range(ValuatorMask *mask,
                                              int first_valuator,
                                              int num_valuators,
                                              const int *valuators);
extern _X_EXPORT void valuator_mask_set(ValuatorMask *mask, int valuator,
                                        int data);
extern _X_EXPORT void valuator_mask_set_double(ValuatorMask *mask, int valuator,
                                               double data);
extern _X_EXPORT void valuator_mask_zero(ValuatorMask *mask);
extern _X_EXPORT int valuator_mask_size(const ValuatorMask *mask);
extern _X_EXPORT int valuator_mask_isset(const ValuatorMask *mask, int bit);
extern _X_EXPORT void valuator_mask_unset(ValuatorMask *mask, int bit);
extern _X_EXPORT int valuator_mask_num_valuators(const ValuatorMask *mask);
extern _X_EXPORT void valuator_mask_copy(ValuatorMask *dest,
                                         const ValuatorMask *src);
extern _X_EXPORT int valuator_mask_get(const ValuatorMask *mask, int valnum);
extern _X_EXPORT double valuator_mask_get_double(const ValuatorMask *mask,
                                                 int valnum);
extern _X_EXPORT Bool valuator_mask_fetch(const ValuatorMask *mask,
                                          int valnum, int *val);
extern _X_EXPORT Bool valuator_mask_fetch_double(const ValuatorMask *mask,
                                                 int valnum, double *val);
extern _X_EXPORT Bool valuator_mask_has_unaccelerated(const ValuatorMask *mask);
extern _X_EXPORT void valuator_mask_set_unaccelerated(ValuatorMask *mask,
                                                      int valuator,
                                                      double accel,
                                                      double unaccel);
extern _X_EXPORT void valuator_mask_set_absolute_unaccelerated(ValuatorMask *mask,
                                                               int valuator,
                                                               int absolute,
                                                               double unaccel);
extern _X_EXPORT double valuator_mask_get_accelerated(const ValuatorMask *mask,
                                                      int valuator);
extern _X_EXPORT double valuator_mask_get_unaccelerated(const ValuatorMask *mask,
                                                        int valuator);
extern _X_EXPORT Bool valuator_mask_fetch_unaccelerated(const ValuatorMask *mask,
                                                        int valuator,
                                                        double *accel,
                                                        double *unaccel);
extern _X_HIDDEN void valuator_mask_drop_unaccelerated(ValuatorMask *mask);

#ifdef __cplusplus
}
#endif


#endif