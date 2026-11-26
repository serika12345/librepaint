#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Click once at a point of a KWin session through KWin's fake input
protocol (org_kde_kwin_fake_input).

KWin delivers the click to the window under the point as real pointer input
with an input serial, which a Wayland client needs to open a popup. KWin lets
any client that is not sandboxed bind the protocol. Needs pywayland and the
protocol's pywayland bindings in BINDINGS_DIR (kwin-env.nix builds them).

Usage: kwin_click.py X,Y BINDINGS_DIR
  X and Y are logical pixels of the workspace. WAYLAND_DISPLAY names KWin's
  socket.
"""

import importlib
import sys

BTN_LEFT = 0x110


def bindings(directory):
    sys.path.insert(0, directory)
    return importlib.import_module("fake_input").OrgKdeKwinFakeInput


def main(argv):
    if len(argv) != 2:
        sys.exit(__doc__)
    x, y = (float(value) for value in argv[0].split(","))
    interface = bindings(argv[1])
    from pywayland.client import Display

    display = Display()
    display.connect()
    found = {}

    def on_global(registry, name, iface, version):
        if iface == interface.name:
            found["proxy"] = registry.bind(name, interface, min(version, interface.version))

    registry = display.get_registry()
    registry.dispatcher["global"] = on_global
    display.roundtrip()
    if "proxy" not in found:
        sys.exit("KWin offers no %s" % interface.name)
    fake = found["proxy"]
    fake.authenticate("LibrePaint UI check", "click for the popup check")
    fake.pointer_motion_absolute(x, y)
    display.roundtrip()
    fake.button(BTN_LEFT, 1)
    display.roundtrip()
    fake.button(BTN_LEFT, 0)
    display.roundtrip()
    display.disconnect()
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
