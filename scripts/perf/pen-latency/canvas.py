#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Write the white 2048x1536 RGB canvas that every measurement run opens.

The PNG carries a 72 dpi pHYs chunk: without one, applications pick their
own resolution, and Krita 6 then shows the canvas at a tiny zoom.

Usage: canvas.py OUT.png
"""

import struct
import sys
import zlib

WIDTH, HEIGHT = 2048, 1536
PIXELS_PER_METER = 2835  # 72 dpi


def chunk(kind, data):
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)


def write(path):
    raw = b"".join(b"\x00" + b"\xff" * (WIDTH * 3) for _ in range(HEIGHT))
    with open(path, "wb") as handle:
        handle.write(
            b"\x89PNG\r\n\x1a\n"
            + chunk(b"IHDR", struct.pack(">IIBBBBB", WIDTH, HEIGHT, 8, 2, 0, 0, 0))
            + chunk(b"pHYs", struct.pack(">IIB", PIXELS_PER_METER, PIXELS_PER_METER, 1))
            + chunk(b"IDAT", zlib.compress(raw, 9))
            + chunk(b"IEND", b"")
        )


if __name__ == "__main__":
    write(sys.argv[1])
