#ifndef WAYNAPTICS_ROOT_SHIM_H
#define WAYNAPTICS_ROOT_SHIM_H

#include "synshared.h"

#define GET_ABI_MAJOR(x) x
// No idea what threaded input is, but it's controlled by checking if this is >23
#define ABI_XINPUT_VERSION 24


#define MODULEVENDORSTRING ""
#define MODINFOSTRING1 0
#define MODINFOSTRING2 0
#define XORG_VERSION_CURRENT 0
#define PACKAGE_VERSION_MAJOR 0
#define PACKAGE_VERSION_MINOR 0
#define PACKAGE_VERSION_PATCHLEVEL 0
#define ABI_CLASS_XINPUT 0
#define MOD_CLASS_XINPUT 0



#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif




/* This structure is expected to be returned by the initfunc */
typedef struct {
    const char *modname;        /* name of module, e.g. "foo" */
    const char *vendor;         /* vendor specific string */
    CARD32 _modinfo1_;          /* constant MODINFOSTRING1/2 to find */
    CARD32 _modinfo2_;          /* infoarea with a binary editor or sign tool */
    CARD32 xf86version;         /* contains XF86_VERSION_CURRENT */
    CARD8 majorversion;         /* module-specific major version */
    CARD8 minorversion;         /* module-specific minor version */
    CARD16 patchlevel;          /* module-specific patch level */
    const char *abiclass;       /* ABI class that the module uses */
    CARD32 abiversion;          /* ABI version */
    const char *moduleclass;    /* module class description */
    CARD32 checksum[4];         /* contains a digital signature of the */
    /* version info structure */
} XF86ModuleVersionInfo;


#endif
