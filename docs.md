# Waynaptics Architecture

Waynaptics converts the X11 `xf86-input-synaptics` touchpad driver into a
standalone PS/2 mouse emulator using Linux `uinput`. The original driver code
runs **unmodified** — waynaptics provides a shim layer that emulates the XOrg
server APIs so the driver thinks it is loaded inside XOrg.

## High-Level Architecture

```
┌─────────────────────────────────────────────────────┐
│  main.cpp (orchestrator)                            │
│  CLI args, driver lifecycle, GLib main loop          │
├─────────────────────────────────────────────────────┤
│  XOrg Shim Layer                                     │
│  ┌──────────────┐ ┌─────────────────────────────┐   │
│  │ device_mgmt  │ │ output_backend              │   │
│  │ xf86Open/    │ │ ┌──────────┐ ┌──────────┐  │   │
│  │ CloseSerial  │ │ │  uinput  │ │   dry    │  │   │
│  │ AddEnabled   │ │ │ REL_X/Y  │ │  (log)   │  │   │
│  │  → GLib IO   │ │ │ HI_RES   │ │          │  │   │
│  └──────────────┘ │ └──────────┘ └──────────┘  │   │
│  ┌──────────────┐ │ event_posting               │   │
│  │ xi_properties│ │ xf86PostMotion/Button →     │   │
│  │ in-memory    │ │   ptrveloc → backend        │   │
│  │ prop store   │ └─────────────────────────────┘   │
│  └──────────────┘ ┌─────────────────────────────┐   │
│  ┌──────────────┐ │ device_init                 │   │
│  │ options      │ │ InitPointerDeviceStruct     │   │
│  │ std::map     │ │ InitValuatorAxisStruct      │   │
│  │ key→value    │ │ SetScrollValuator           │   │
│  └──────────────┘ └─────────────────────────────┘   │
├─────────────────────────────────────────────────────┤
│  Original synaptics driver (unmodified C code)       │
│  synaptics.c  eventcomm.c  synproto.c  properties.c │
└─────────────────────────────────────────────────────┘
```

## Driver Lifecycle

The synaptics driver expects a specific XOrg lifecycle. Waynaptics orchestrates
this from `main.cpp`:

1. **SynapticsPreInit** — Discovers device (auto-detect via `/dev/input/event*`),
   reads touchpad dimensions/capabilities, sets default parameters from
   `xf86Set*Option()` calls, then closes the device.

2. **DEVICE_INIT** — Sets up valuators (X, Y, scroll axes), registers XI
   properties, configures acceleration profile. This is where
   `SynapticsAccelerationProfile` is registered and
   `ACCEL_PROP_CONSTANT_DECELERATION` is set.

3. **Config loading** — After properties are initialized, the synclient config
   (if provided) is applied via the XI property system. The driver's
   `SetProperty` callback updates `synpara` fields.

4. **DEVICE_ON** — Reopens the device, grabs it (via `libevdev_grab`), and
   starts reading events. `xf86AddEnabledDevice` registers the fd with GLib
   as an IO watch.

5. **GLib main loop** — Events flow: touchpad → eventcomm → driver logic →
   `xf86PostMotionEvent`/`xf86PostButtonEvent` → acceleration → uinput.

6. **DEVICE_OFF / DEVICE_CLOSE** — Cleanup on SIGINT/SIGTERM.

## Source Files

### Shim Layer (`src/waynaptics/`)

| File | Purpose |
|------|---------|
| `main.cpp` | Entry point, CLI parsing, driver lifecycle, signal handling |
| `options.cpp` | `xf86SetIntOption`, `xf86SetRealOption`, `xf86CheckStrOption`, etc. — backed by `Options` class (`include/options.h`) |
| `device_mgmt.cpp` | `xf86OpenSerial` (→ `open()`), `xf86CloseSerial` (→ `close()`), `xf86AddEnabledDevice` (→ GLib IO watch) |
| `device_init.cpp` | `InitPointerDeviceStruct`, `xf86InitValuatorAxisStruct`, `SetScrollValuator`, `SetDeviceSpecificAccelerationProfile` |
| `event_posting.cpp` | `xf86PostMotionEvent` (with acceleration), `xf86PostButtonEvent`, `xf86PostMotionEventM` (scroll) |
| `ptrveloc.cpp` | Pointer acceleration matching X server's `ptrveloc.c`: velocity tracking, profile evaluation, softening |
| `output_backend.cpp` | `UinputBackend` (creates `/dev/uinput` virtual mouse) and `DryBackend` (logs to stderr) |
| `xi_properties.cpp` | In-memory XI property store: `XIChangeDeviceProperty`, `XIRegisterPropertyHandler`, `SetProperty` callbacks |
| `synclient_loader.cpp` | Parses `synclient -l` dumps, pre-loads into options and applies via XI properties |
| `atoms.cpp` | Atom registry: `XIGetKnownProperty`/`NameForAtom` backed by `std::vector<std::string>` |
| `valuators.c` | `ValuatorMask` bit-manipulation functions (C file) |
| `supportlib.cpp` | Timer system (`TimerSet`/`TimerCancel`), `GetTimeInMillis`, misc stubs |
| `mainloop.cpp` | GLib main loop wrapper |

### Headers (`src/waynaptics/include/`)

| File | Purpose |
|------|---------|
| `synshared.h` | Core types (`InputInfoRec`, `DeviceIntRec`, `ValuatorMask`, etc.) and **all** C function declarations |
| `options.h` | `Options` class definition (single `std::map<string,string>`) |
| `output_backend.h` | `OutputBackend` abstract interface |
| `ptrveloc.h` | Acceleration API (`waynaptics_accel_init`, `waynaptics_accel_apply`, `waynaptics_accel_set_profile`) |
| `device_init.h` | `DeviceInitState`, `ScrollValuatorInfo`, `waynaptics_get_device_init_state()` |
| `xi_properties.h` | Property store API (`waynaptics_get_property_value`, `waynaptics_call_set_property_handler`) |

### Unmodified Driver (`external/xf86-input-synaptics/src/`)

| File | Purpose |
|------|---------|
| `synaptics.c` | Main driver: PreInit, DeviceInit/On/Off/Close, motion processing, `SynapticsAccelerationProfile` |
| `eventcomm.c` | evdev backend: device discovery, event reading, `EventAutoDevProbe` |
| `synproto.c` | Protocol helpers |
| `properties.c` | XI property registration and `SetProperty` callback |

### Header Shims (`src/waynaptics/fakeclude/`)

Replacement headers that redirect standard XOrg includes (e.g. `<xf86.h>`,
`<xf86Xinput.h>`, `<xorgVersion.h>`) to `synshared.h`. This is how the
unmodified driver code compiles against our shim types instead of the real
XOrg headers.

## Key Gotchas

### 1. Missing C declarations = pointer truncation SIGSEGV

The driver is compiled as **C** code. If a C function that returns a pointer
is not declared before use, the C compiler assumes it returns `int` (32 bits).
On x86-64 this truncates the 64-bit pointer, causing a segfault when it's
dereferenced.

**All** `extern "C"` functions callable from the driver must be declared in
`synshared.h`. If you add a new shim function, add its declaration there.

### 2. `private` and `public` are C++ keywords

The original XOrg structs use `private` and `public` as field names. Since our
headers are included from C++ code, these are renamed:

- `pInfo->private` → `pInfo->priv`
- `dev->public` → `dev->pub`

The **C** driver code (compiled as C, where these aren't keywords) uses the
original names. The `synshared.h` struct definitions handle this with the
actual field names matching what each language expects. See the `InputInfoRec`
and `DeviceIntRec` definitions.

### 3. XOrg return conventions

`DeviceControl` (`DEVICE_INIT`, `DEVICE_ON`, etc.) returns `int` where
**0 = Success** and non-zero = error. This is the **opposite** of the usual
boolean convention. Always check `!= 0` for failure.

### 4. The synclient config lies about actual values

Desktop environments with `disable-while-typing=true` constantly set
`TouchpadOff=2` via XI properties. `synclient -l` reads its internal cache
and may not reflect the actual X server state.

**Solution:** The synclient loader ignores `TouchpadOff` and
`GrabEventDevice` from config files — these are not portable user
preferences.

### 5. Acceleration: two-phase config loading

The driver reads `MinSpeed`/`MaxSpeed`/`AccelFactor` from options during
`set_default_parameters()` (in PreInit), then normalizes them during
DeviceInit. If the synclient config is only applied AFTER DeviceInit (via
properties), the acceleration constants are wrong.

**Solution:** Config values are pre-loaded into the Options map BEFORE
PreInit (`waynaptics_preload_synclient_options`), so `set_default_parameters()`
reads the correct values. The config is then applied again via the property
system after DeviceInit for runtime-adjustable parameters.

### 6. Acceleration profile must survive init

`SetDeviceSpecificAccelerationProfile` is called during DeviceInit (before
`waynaptics_accel_init`). The profile function pointer must be preserved
across the `memset` in `waynaptics_accel_init`.

### 7. Softening stores pre-scaling values

The X server's `ApplySoftening` stores `last_dx`/`last_dy` **before**
applying the acceleration multiplier and constant deceleration. If you store
post-scaling values, the softening creates a positive feedback loop that
causes exponential growth in output values.

### 8. Smooth scrolling

The synaptics driver posts scroll as fractional valuator deltas on axes 2
(horizontal) and 3 (vertical). `SetScrollValuator` provides the increment
(e.g., `scroll_dist_vert=29` means 29 touchpad units = 1 scroll click).

The uinput backend converts to `REL_WHEEL_HI_RES` / `REL_HWHEEL_HI_RES`
(120 units = 1 detent) and also emits legacy `REL_WHEEL`/`REL_HWHEEL` by
accumulating fractional values until full detents.

### 9. Edge scroll defaults

Touchpads with `BTN_TOOL_DOUBLETAP` (most modern ones) default to
`vertEdgeScroll=FALSE` (two-finger scroll is preferred). The synclient config
can override this via the `Synaptics Edge Scrolling` property.

### 10. Device grab conflicts

Only one process can grab an evdev device at a time. If the X server's
synaptics driver is running, waynaptics will fail with `EBUSY` (errno 16)
on `libevdev_grab`. Stop or disable the X driver first:

```sh
# Disable X synaptics driver for device id 13:
xinput disable 13
```

## Build System

```sh
cd cmake-build-debug
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

Produces one executable:
- `waynaptics` — the main emulator

Dependencies: `libevdev`, `glib-2.0`.

Key CMake settings:
- `-DBUILD_EVENTCOMM` — enables the evdev backend in the driver
- `-w` — suppresses warnings from the original driver code

## Usage

```sh
# Auto-detect touchpad, apply config:
waynaptics --config synclient.txt

# Specify device by name:
waynaptics -n "Touchpad" --config synclient.txt

# Specify device by path:
waynaptics -d /dev/input/event5 --config synclient.txt

# Safe testing (no grab, no uinput, logs to stderr):
waynaptics --dry --log-output
```
