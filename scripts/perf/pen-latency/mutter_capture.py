#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Capture-based pointer latency measurement for GNOME (mutter) sessions.

Opens a mutter RemoteDesktop session with a linked ScreenCast monitor
stream, finds the white canvas in the first captured frame, replays the
trajectory of trajectory.py with absolute pointer motion, and records for
every captured frame the extent of dark pixels in each trajectory band.

Outputs in --out:
- inject.csv: time_ns,phase,band,x,y,pressed (x, y in captured pixels)
- capture.csv: frame_ns,arrival_ns,band,xmin,xmax,dense_xmin,dense_xmax,ymin,ymax
- capture-meta.json: canvas rectangle, bands, trajectory options, frame size

All times are CLOCK_MONOTONIC nanoseconds. frame_ns is the PipeWire buffer
time converted to CLOCK_MONOTONIC; arrival_ns is when this process received
the frame. Requires PyGObject, GStreamer with the PipeWire plugin, numpy.
"""

import argparse
import csv
import json
import os
import sys
import threading
import time

import gi

gi.require_version("Gst", "1.0")
from gi.repository import Gio, GLib, Gst  # noqa: E402

import numpy as np  # noqa: E402

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import trajectory  # noqa: E402

BTN_LEFT = 0x110
RD = "org.gnome.Mutter.RemoteDesktop"
SC = "org.gnome.Mutter.ScreenCast"


def monotonic_ns():
    return time.clock_gettime_ns(time.CLOCK_MONOTONIC)


class Mutter:
    def __init__(self, connector):
        self.bus = Gio.bus_get_sync(Gio.BusType.SESSION)
        self.rd = self.call(RD, "/org/gnome/Mutter/RemoteDesktop", RD, "CreateSession", None, "(o)")[0]
        session_id = self.call(
            RD, self.rd, "org.freedesktop.DBus.Properties", "Get",
            GLib.Variant("(ss)", (RD + ".Session", "SessionId")), "(v)",
        )[0]
        self.sc = self.call(
            SC, "/org/gnome/Mutter/ScreenCast", SC, "CreateSession",
            GLib.Variant("(a{sv})", ({"remote-desktop-session-id": GLib.Variant("s", session_id)},)), "(o)",
        )[0]
        # cursor-mode 0: the cursor is not part of the captured image.
        self.stream = self.call(
            SC, self.sc, SC + ".Session", "RecordMonitor",
            GLib.Variant("(sa{sv})", (connector, {"cursor-mode": GLib.Variant("u", 0)})), "(o)",
        )[0]
        self.node = None
        loop = GLib.MainLoop()

        def added(_connection, _sender, _path, _iface, _signal, params, _data):
            self.node = params.unpack()[0]
            loop.quit()

        self.bus.signal_subscribe(None, SC + ".Stream", "PipeWireStreamAdded", self.stream, None, 0, added, None)
        self.call(RD, self.rd, RD + ".Session", "Start", None)
        GLib.timeout_add(5000, loop.quit)
        loop.run()
        if self.node is None:
            raise SystemExit("mutter did not announce a PipeWire stream")

    def call(self, dest, path, iface, method, args, reply=None):
        result = self.bus.call_sync(
            dest, path, iface, method, args, GLib.VariantType(reply) if reply else None,
            Gio.DBusCallFlags.NONE, -1, None,
        )
        return result.unpack() if result is not None else None

    def move(self, x, y):
        self.call(RD, self.rd, RD + ".Session", "NotifyPointerMotionAbsolute", GLib.Variant("(sdd)", (self.stream, x, y)))

    def button(self, pressed):
        self.call(RD, self.rd, RD + ".Session", "NotifyPointerButton", GLib.Variant("(ib)", (BTN_LEFT, pressed)))

    def stop(self):
        try:
            self.call(RD, self.rd, RD + ".Session", "Stop", None)
        except GLib.Error:
            pass


def save_png(path, frame):
    """Write a BGRx frame as an RGB PNG (diagnostics)."""
    import struct
    import zlib

    rgb = np.ascontiguousarray(frame[:, :, 2::-1])
    raw = b"".join(b"\x00" + rgb[y].tobytes() for y in range(rgb.shape[0]))

    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)

    header = struct.pack(">IIBBBBB", rgb.shape[1], rgb.shape[0], 8, 2, 0, 0, 0)
    with open(path, "wb") as handle:
        handle.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(b"IDAT", zlib.compress(raw, 6)) + chunk(b"IEND", b""))


def find_canvas(frame, threshold=245):
    """Largest white axis-aligned rectangle around the white pixel nearest the frame center."""
    white = np.all(frame[:, :, :3] >= threshold, axis=2)
    ys, xs = np.nonzero(white)
    if len(xs) == 0:
        raise SystemExit("no white canvas visible in the captured frame")
    h, w = white.shape
    nearest = np.argmin((xs - w / 2) ** 2 + (ys - h / 2) ** 2)
    cx, cy = int(xs[nearest]), int(ys[nearest])
    row = white[cy]
    col = white[:, cx]
    left = cx - int(np.argmin(row[cx::-1])) + 1 if not row[: cx + 1].all() else 0
    right = cx + int(np.argmin(row[cx:])) if not row[cx:].all() else w
    top = cy - int(np.argmin(col[cy::-1])) + 1 if not col[: cy + 1].all() else 0
    bottom = cy + int(np.argmin(col[cy:])) if not col[cy:].all() else h
    inset = 8
    return left + inset, top + inset, right - left - 2 * inset, bottom - top - 2 * inset


class Capture:
    def __init__(self, node):
        self.pipe = Gst.parse_launch(
            "pipewiresrc path=%d always-copy=true ! video/x-raw,format=BGRx ! "
            "appsink name=sink emit-signals=true sync=false max-buffers=4 drop=false" % node
        )
        self.sink = self.pipe.get_by_name("sink")
        self.lock = threading.Lock()
        self.latest = None
        self.bands = None
        self.rows = []
        self.dark = 128
        self.dense = 6
        self.offset = None
        self.sink.connect("new-sample", self.on_sample)

    def start(self):
        self.pipe.set_state(Gst.State.PLAYING)
        self.pipe.get_state(5 * Gst.SECOND)
        self.offset = self.clock_offset()

    def clock_offset(self):
        """CLOCK_MONOTONIC minus pipeline clock, from the tightest of 50 paired reads."""
        best = None
        clock = self.pipe.get_clock()
        for _ in range(50):
            a = monotonic_ns()
            c = clock.get_time()
            b = monotonic_ns()
            if best is None or b - a < best[0]:
                best = (b - a, (a + b) // 2 - c)
        return best[1]

    def on_sample(self, sink):
        sample = sink.emit("pull-sample")
        arrival = monotonic_ns()
        buffer = sample.get_buffer()
        caps = sample.get_caps().get_structure(0)
        width, height = caps.get_value("width"), caps.get_value("height")
        ok, info = buffer.map(Gst.MapFlags.READ)
        if not ok:
            return Gst.FlowReturn.OK
        try:
            stride = len(info.data) // height
            frame = np.frombuffer(info.data, dtype=np.uint8).reshape(height, stride // 4, 4)[:, :width]
            frame_ns = buffer.pts + self.pipe.get_base_time() + (self.offset or 0)
            with self.lock:
                if self.bands is None:
                    self.latest = frame.copy()
                else:
                    self.detect(frame, frame_ns, arrival)
        finally:
            buffer.unmap(info)
        return Gst.FlowReturn.OK

    def detect(self, frame, frame_ns, arrival):
        for name, x0, x1, y0, y1 in self.bands:
            region = frame[y0:y1, x0:x1, :3]
            dark = np.all(region < self.dark, axis=2)
            columns = dark.sum(axis=0)
            any_columns = np.nonzero(columns)[0]
            if len(any_columns) == 0:
                self.rows.append((frame_ns, arrival, name, "", "", "", "", "", ""))
                continue
            dense_columns = np.nonzero(columns >= self.dense)[0]
            any_rows = np.nonzero(dark.any(axis=1))[0]
            self.rows.append((
                frame_ns, arrival, name,
                x0 + int(any_columns[0]), x0 + int(any_columns[-1]),
                x0 + int(dense_columns[0]) if len(dense_columns) else "",
                x0 + int(dense_columns[-1]) if len(dense_columns) else "",
                y0 + int(any_rows[0]), y0 + int(any_rows[-1]),
            ))

    def stop(self):
        self.pipe.set_state(Gst.State.NULL)


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--out", required=True)
    parser.add_argument("--connector", help="monitor connector; default: the first one mutter reports")
    parser.add_argument("--options", default="{}", help="trajectory option overrides as JSON")
    parser.add_argument("--input-scale", type=float, default=1.0,
                        help="pointer coordinate units per captured pixel")
    parser.add_argument("--dark", type=int, default=128, help="channel value below which a pixel is dark")
    parser.add_argument("--dense", type=int, default=6, help="dark pixels per column that count as ink")
    parser.add_argument("--snapshot", action="store_true", help="save first-frame.png and exit")
    args = parser.parse_args(argv)
    os.makedirs(args.out, exist_ok=True)
    Gst.init(None)

    connector = args.connector
    if connector is None:
        bus = Gio.bus_get_sync(Gio.BusType.SESSION)
        state = bus.call_sync(
            "org.gnome.Mutter.DisplayConfig", "/org/gnome/Mutter/DisplayConfig",
            "org.gnome.Mutter.DisplayConfig", "GetCurrentState", None, None, Gio.DBusCallFlags.NONE, -1, None,
        ).unpack()
        connector = state[1][0][0][0]

    mutter = Mutter(connector)
    capture = Capture(mutter.node)
    capture.dark, capture.dense = args.dark, args.dense
    capture.start()
    try:
        # Move the pointer out of the way and force one frame with the canvas.
        mutter.move(1, 1)
        deadline = time.time() + 5
        while time.time() < deadline:
            with capture.lock:
                frame = capture.latest
            if frame is not None:
                break
            time.sleep(0.05)
        if frame is None:
            raise SystemExit("no frame captured")
        save_png(os.path.join(args.out, "first-frame.png"), frame)
        if args.snapshot:
            return 0
        left, top, width, height = find_canvas(frame)
        options = trajectory.options(json.loads(args.options))
        bands = []
        for name, y, half in trajectory.bands(top, height, options):
            bands.append((name, left, left + width, int(y - half), int(y + half) + 1))
        with open(os.path.join(args.out, "capture-meta.json"), "w") as handle:
            json.dump({
                "connector": connector, "frame": [int(frame.shape[1]), int(frame.shape[0])],
                "canvas": [left, top, width, height], "bands": bands, "options": options,
                "input_scale": args.input_scale, "dark": args.dark, "dense": args.dense,
            }, handle, indent=2)
        with capture.lock:
            capture.bands = bands

        injected = []
        pressed = False
        start = monotonic_ns() + 300_000_000
        for t, phase, band, x, y, want in trajectory.samples(left, top, width, height, options):
            target = start + int(t * 1e9)
            while True:
                remaining = target - monotonic_ns()
                if remaining <= 0:
                    break
                time.sleep(remaining / 1e9 if remaining > 2_000_000 else 0)
            now = monotonic_ns()
            mutter.move(x * args.input_scale, y * args.input_scale)
            if want != pressed:
                mutter.button(want)
                pressed = want
            injected.append((now, phase, band, "%.2f" % x, "%.2f" % y, int(want)))
        time.sleep(0.5)
    finally:
        if "pressed" in locals() and pressed:
            mutter.button(False)
        capture.stop()
        mutter.stop()

    with open(os.path.join(args.out, "inject.csv"), "w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["time_ns", "phase", "band", "x", "y", "pressed"])
        writer.writerows(injected)
    with open(os.path.join(args.out, "capture.csv"), "w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["frame_ns", "arrival_ns", "band", "xmin", "xmax", "dense_xmin", "dense_xmax", "ymin", "ymax"])
        writer.writerows(capture.rows)
    print("frames=%d injected=%d canvas=%s" % (len({r[0] for r in capture.rows}), len(injected), (left, top, width, height)))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
