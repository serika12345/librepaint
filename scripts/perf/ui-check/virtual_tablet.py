#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Draw one stroke with a virtual pen tablet made through /dev/uinput.

The device reports absolute position, pressure and tilt like a pen tablet,
so libinput and the compositor hand it to applications as a tablet tool
(tablet-v2 on Wayland, XInput2 through XWayland). The stroke goes left to
right across the given part of the screen: the pressure rises from 10 % to
100 % and the tilt from -30 to +30 degrees on the x axis, with a constant
+15 degrees on the y axis. The tablet covers the whole screen.

Needs write access to /dev/uinput (the uinput group on the Linux host) and
only the standard library.

Usage: virtual_tablet.py [--from X,Y] [--to X,Y] [--seconds S] [--wait-for FILE]
                         [--device-file FILE]
  X and Y are fractions of the screen; defaults 0.30,0.55 to 0.60,0.55.
  --wait-for creates the device at once and draws when FILE exists, so the
  tablet is present before the application starts, as a plugged-in tablet is.
  --device-file writes the device's /dev/input/eventN path to FILE once the
  device exists, for an X server that is given the device explicitly.
"""

import argparse
import fcntl
import glob
import os
import struct
import sys
import time

# linux/input-event-codes.h
EV_SYN, EV_KEY, EV_ABS = 0x00, 0x01, 0x03
SYN_REPORT = 0
BTN_TOOL_PEN, BTN_TOUCH, BTN_STYLUS = 0x140, 0x14A, 0x14B
ABS_X, ABS_Y, ABS_PRESSURE, ABS_TILT_X, ABS_TILT_Y = 0x00, 0x01, 0x18, 0x1A, 0x1B
INPUT_PROP_POINTER = 0x00
BUS_USB = 0x03

# linux/uinput.h
UI_DEV_CREATE = 0x5501
UI_DEV_DESTROY = 0x5502
UI_SET_EVBIT = 0x40045564
UI_SET_KEYBIT = 0x40045565
UI_SET_ABSBIT = 0x40045567
UI_SET_PROPBIT = 0x4004556E
UI_DEV_SETUP = 0x405C5503
UI_ABS_SETUP = 0x401C5504
SYSNAME_LENGTH = 64
UI_GET_SYSNAME = 0x80000000 | (SYSNAME_LENGTH << 16) | (ord("U") << 8) | 44

MAX_XY = 32767
MAX_PRESSURE = 4095
UNITS_PER_MM = 100


def abs_setup(fd, code, minimum, maximum, resolution=0):
    # struct uinput_abs_setup { __u16 code; struct input_absinfo absinfo; }
    # input_absinfo: value, minimum, maximum, fuzz, flat, resolution (s32 each)
    fcntl.ioctl(fd, UI_ABS_SETUP, struct.pack("<H2x6i", code, 0, minimum, maximum, 0, 0, resolution))


def create():
    fd = os.open("/dev/uinput", os.O_WRONLY | os.O_NONBLOCK)
    fcntl.ioctl(fd, UI_SET_EVBIT, EV_KEY)
    fcntl.ioctl(fd, UI_SET_EVBIT, EV_ABS)
    for key in (BTN_TOOL_PEN, BTN_TOUCH, BTN_STYLUS):
        fcntl.ioctl(fd, UI_SET_KEYBIT, key)
    for axis in (ABS_X, ABS_Y, ABS_PRESSURE, ABS_TILT_X, ABS_TILT_Y):
        fcntl.ioctl(fd, UI_SET_ABSBIT, axis)
    fcntl.ioctl(fd, UI_SET_PROPBIT, INPUT_PROP_POINTER)
    abs_setup(fd, ABS_X, 0, MAX_XY, UNITS_PER_MM)
    abs_setup(fd, ABS_Y, 0, MAX_XY, UNITS_PER_MM)
    abs_setup(fd, ABS_PRESSURE, 0, MAX_PRESSURE)
    abs_setup(fd, ABS_TILT_X, -90, 90, 57)  # resolution in units per radian
    abs_setup(fd, ABS_TILT_Y, -90, 90, 57)
    # struct uinput_setup { struct input_id id; char name[80]; __u32 ff_effects_max; }
    name = b"LibrePaint virtual pen tablet"
    fcntl.ioctl(fd, UI_DEV_SETUP, struct.pack("<4H80sI", BUS_USB, 0x56A, 0x0357, 1, name, 0))
    fcntl.ioctl(fd, UI_DEV_CREATE)
    return fd


def event_node(fd):
    """Return the /dev/input/eventN path of the created device."""
    sysname = fcntl.ioctl(fd, UI_GET_SYSNAME, bytes(SYSNAME_LENGTH)).split(b"\0")[0].decode()
    nodes = glob.glob("/sys/devices/virtual/input/%s/event*" % sysname)
    if len(nodes) != 1:
        raise SystemExit("no single event node for %s: %s" % (sysname, nodes))
    return "/dev/input/" + os.path.basename(nodes[0])


def emit(fd, kind, code, value):
    seconds = time.time()
    os.write(fd, struct.pack("<qqHHi", int(seconds), int((seconds % 1) * 1e6), kind, code, value))


def report(fd, x, y, pressure, tilt_x, tilt_y, touch):
    emit(fd, EV_ABS, ABS_X, int(x * MAX_XY))
    emit(fd, EV_ABS, ABS_Y, int(y * MAX_XY))
    emit(fd, EV_ABS, ABS_PRESSURE, int(pressure * MAX_PRESSURE))
    emit(fd, EV_ABS, ABS_TILT_X, int(tilt_x))
    emit(fd, EV_ABS, ABS_TILT_Y, int(tilt_y))
    emit(fd, EV_KEY, BTN_TOUCH, 1 if touch else 0)
    emit(fd, EV_SYN, SYN_REPORT, 0)


def stroke(fd, start, end, seconds, rate=200):
    x0, y0 = start
    x1, y1 = end
    emit(fd, EV_KEY, BTN_TOOL_PEN, 1)
    report(fd, x0, y0, 0.0, -30, 15, False)
    time.sleep(0.3)
    steps = max(2, int(seconds * rate))
    for step in range(steps + 1):
        t = step / steps
        report(fd, x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, 0.1 + 0.9 * t, -30 + 60 * t, 15, True)
        time.sleep(1.0 / rate)
    report(fd, x1, y1, 0.0, 30, 15, False)
    time.sleep(0.2)
    emit(fd, EV_KEY, BTN_TOOL_PEN, 0)
    emit(fd, EV_SYN, SYN_REPORT, 0)


def point(text):
    x, y = (float(value) for value in text.split(","))
    return x, y


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--from", dest="start", type=point, default=(0.30, 0.55))
    parser.add_argument("--to", dest="end", type=point, default=(0.60, 0.55))
    parser.add_argument("--seconds", type=float, default=1.5)
    parser.add_argument("--wait-for", metavar="FILE", help="draw when this file exists (at most 300 seconds)")
    parser.add_argument("--device-file", metavar="FILE", help="write the device's /dev/input/eventN path here")
    args = parser.parse_args(argv)
    fd = create()
    try:
        time.sleep(1.5)  # the compositor opens the new device
        if args.device_file:
            with open(args.device_file + ".tmp", "w") as stream:
                stream.write(event_node(fd) + "\n")
            os.replace(args.device_file + ".tmp", args.device_file)
        if args.wait_for:
            deadline = time.time() + 300
            while not os.path.exists(args.wait_for):
                if time.time() > deadline:
                    raise SystemExit("%s did not appear within 300 seconds" % args.wait_for)
                time.sleep(0.1)
        stroke(fd, args.start, args.end, args.seconds)
        time.sleep(0.5)
    finally:
        fcntl.ioctl(fd, UI_DEV_DESTROY)
        os.close(fd)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
