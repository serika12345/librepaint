#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Minimal pointer drawing window, the reference for capture-based latency.

On every pointer event the window immediately redraws: a black 21 px square
at the hover position, and a 20 px black line while the primary button is
held, on a white background. The capture tools detect these marks like the
brush outline and the ink, so measuring this window gives the latency that
the window system and the capture path add without a painting application.

With REFERENCE_LOG set, every received event is written there as
"time_ns,kind,x,y" (CLOCK_MONOTONIC), the clock of mutter_capture.py, so the
input delivery time can be separated from the display and capture time.

Usage: reference_app.py [CANVAS_PATH]   (the argument is ignored)
"""

import os
import time
import tkinter

LOG = open(os.environ["REFERENCE_LOG"], "w", buffering=1) if os.environ.get("REFERENCE_LOG") else None


def record(kind, event):
    if LOG is not None:
        LOG.write("%d,%s,%d,%d\n" % (time.clock_gettime_ns(time.CLOCK_MONOTONIC), kind, event.x_root, event.y_root))


def main():
    root = tkinter.Tk()
    root.title("pen-latency reference")
    width, height = root.winfo_screenwidth(), root.winfo_screenheight()
    root.geometry("%dx%d+%d+%d" % (int(width * 0.8), int(height * 0.8), int(width * 0.1), int(height * 0.1)))
    canvas = tkinter.Canvas(root, background="white", highlightthickness=0, cursor="none")
    canvas.pack(fill="both", expand=True)
    state = {"mark": None, "last": None}

    def hover(event):
        record("hover", event)
        if state["mark"] is not None:
            canvas.delete(state["mark"])
        state["mark"] = canvas.create_rectangle(event.x - 10, event.y - 10, event.x + 10, event.y + 10,
                                                fill="black", outline="black")
        state["last"] = None
        canvas.update_idletasks()

    def press(event):
        record("press", event)
        if state["mark"] is not None:
            canvas.delete(state["mark"])
            state["mark"] = None
        state["last"] = (event.x, event.y)
        canvas.create_oval(event.x - 10, event.y - 10, event.x + 10, event.y + 10, fill="black", outline="black")
        canvas.update_idletasks()

    def drag(event):
        record("drag", event)
        last = state["last"] or (event.x, event.y)
        canvas.create_line(last[0], last[1], event.x, event.y, width=20, fill="black", capstyle="round")
        state["last"] = (event.x, event.y)
        canvas.update_idletasks()

    canvas.bind("<Motion>", hover)
    canvas.bind("<ButtonPress-1>", press)
    canvas.bind("<B1-Motion>", drag)
    canvas.bind("<ButtonRelease-1>", hover)
    try:
        root.mainloop()
    finally:
        if LOG is not None:
            LOG.close()


if __name__ == "__main__":
    main()
