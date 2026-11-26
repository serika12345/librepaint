#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Save the whole KWin workspace as a PNG through KWin's ScreenShot2 D-Bus
interface (org.kde.KWin.ScreenShot2.CaptureWorkspace).

KWin writes the raw image to a pipe and returns its size, stride and
QImage format. Needs PyGObject; DBUS_SESSION_BUS_ADDRESS names KWin's bus.

Usage: kwin_screenshot.py OUT.png
"""

import os
import struct
import sys
import zlib

import gi

gi.require_version("Gio", "2.0")
from gi.repository import Gio, GLib  # noqa: E402

# QImage::Format_RGB32, Format_ARGB32 and Format_ARGB32_Premultiplied are
# 32-bit words 0xAARRGGBB, stored as B, G, R, A bytes on little-endian hosts.
# Format_RGBX8888, Format_RGBA8888 and Format_RGBA8888_Premultiplied store
# R, G, B, A bytes. The value is the byte offset of red, green and blue.
CHANNELS = {4: (2, 1, 0), 5: (2, 1, 0), 6: (2, 1, 0), 16: (0, 1, 2), 17: (0, 1, 2), 18: (0, 1, 2)}


def capture():
    bus = Gio.bus_get_sync(Gio.BusType.SESSION, None)
    read_fd, write_fd = os.pipe()
    fds = Gio.UnixFDList.new()
    index = fds.append(write_fd)
    os.close(write_fd)
    options = {"native-resolution": GLib.Variant("b", True)}
    reply, _ = bus.call_with_unix_fd_list_sync(
        "org.kde.KWin.ScreenShot2", "/org/kde/KWin/ScreenShot2", "org.kde.KWin.ScreenShot2",
        "CaptureWorkspace", GLib.Variant("(a{sv}h)", (options, index)), GLib.VariantType("(a{sv})"),
        Gio.DBusCallFlags.NONE, 10000, fds, None)
    del fds
    meta = reply.unpack()[0]
    chunks = []
    with os.fdopen(read_fd, "rb") as stream:
        while True:
            chunk = stream.read(1 << 20)
            if not chunk:
                break
            chunks.append(chunk)
    return meta, b"".join(chunks)


def write_png(path, width, height, stride, data, channels):
    red, green, blue = channels
    rows = []
    for y in range(height):
        row = data[y * stride:y * stride + width * 4]
        rgb = bytearray(width * 3)
        rgb[0::3] = row[red::4]
        rgb[1::3] = row[green::4]
        rgb[2::3] = row[blue::4]
        rows.append(b"\0" + bytes(rgb))

    def chunk(kind, body):
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body) & 0xFFFFFFFF)

    with open(path, "wb") as stream:
        stream.write(b"\x89PNG\r\n\x1a\n")
        stream.write(chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)))
        stream.write(chunk(b"IDAT", zlib.compress(b"".join(rows), 6)))
        stream.write(chunk(b"IEND", b""))


def main(argv):
    if len(argv) != 1:
        sys.exit(__doc__)
    meta, data = capture()
    if meta.get("format") not in CHANNELS:
        sys.exit("unexpected image format %s" % meta.get("format"))
    write_png(argv[0], meta["width"], meta["height"], meta["stride"], data, CHANNELS[meta["format"]])
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
