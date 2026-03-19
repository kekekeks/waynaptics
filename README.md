# Waynaptics - synaptics "driver" for Wayland

## Why? 

libinput is way too opinionated to give me the **EXACT** setup I've had with Xorg. Wayland compositors also often don't provide customization UI for touchpads, so users are stuck with what's there.

## How?

Waynaptics implements the minimal set of Xorg APIs to run the original synaptics driver without an actual Xorg. On start it grabs your touchpad device, feeds it to synaptics driver and then feeds its output to an emulated a Lenovo ScrollPoint mouse using uinput. This particular model is used because it has hacks in libinput codebase to enable smooth scrolling with wheel.

## How to use

### 1) Grab config
Grab your existing settings from your old X11 session using `synclient > waynaptics.conf`.

### 2) Compile and install

Compile waynaptics yourself or grab a precompiled binary from releases.

If you are using a package, copy your config to `/etc/waynaptics.conf` and restart the service via `systemctl restart waynaptics.service`.

If you are running manually, specify the config via `-c waynaptics.conf`. 


There are some extra options you can specify from command line:

```
Synaptics touchpad → PS/2 mouse emulator via uinput.

Options:
  -c, --config <file>   Path to synclient parameter dump
  -d, --device <path>   Specific evdev device path (auto-detect if omitted)
  -n, --device-name <name>
                        Match evdev device by name substring (e.g. "Touchpad")
      --mouse-type <type>
                        scroll-point (default) or generic
      --scroll-factor <N>
                        Scroll speed multiplier (default: 5 for scroll-point)
      --dry             Dry mode: don't grab device, don't create uinput
      --no-hires-scroll Disable hi-res scroll events (REL_WHEEL_HI_RES)
      --no-lores-scroll Disable low-res scroll events (REL_WHEEL)
      --log-evdev       Log raw evdev touchpad events to stderr
      --log-output      Log produced mouse/scroll output events to stderr
  -h, --help            Print this help and exit
```

You might need to adjust the unit by specifying device from command line (use --device-name and get the name from evtest or smth).

### 3) Adjust DE settings

DISABLE mouse acceleration for your mouse, otherwise libinput will apply accel profile on top of synaptics accel profile. In KDE you can do this for individual pointer devices, not sure about other DEs.

