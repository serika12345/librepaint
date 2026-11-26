#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Judge the states.jsonl of run-ui-check.sh.

Checks, for the expected Qt platform:
- start: the main window is visible.
- tablet_result: the virtual pen's stroke arrived as tablet events with
  pressure from at most 0.3 to at least 0.8 and x tilt from at most -20 to at
  least +20 degrees (virtual_tablet.py sends 0.1 to 1.0 and -30 to +30).
- popup: the brush preset popup is visible, its top-left corner is at the
  bottom-left corner of its button within TOLERANCE_PX.
- docker: the Layers docker is found, floating and visible.
- window: two main windows exist and both are visible.
--no-pen skips the two pen checks for a session that has no pen device
(KWin's virtual output). Prints one line per check and exits 1 when one
fails. The screenshots next to
states.jsonl show the compositor's placement, which Qt's coordinates cannot
show on Wayland.
"""

import argparse
import json
import sys

TOLERANCE_PX = 4


def judge(states, platform, pen=True):
    """Return [(check, ok, detail)]."""
    by_state = {entry["state"]: entry for entry in states}
    results = []

    def add(name, ok, detail):
        results.append((name, bool(ok), detail))

    platforms = sorted({entry.get("platform") for entry in states})
    add("platform", platforms == [platform], "recorded %s" % ",".join(map(str, platforms)))
    for name in ("start", "tablet", "tablet_result", "popup", "docker", "window"):
        if "error" in by_state.get(name, {}):
            add(name, False, by_state[name]["error"])
    start = by_state.get("start", {})
    add("main window", start.get("visible"), "rect %s" % start.get("main"))
    tablet = by_state.get("tablet_result", {})
    pressure, tilt = tablet.get("pressure"), tablet.get("x_tilt")
    if pen:
        add("pen pressure", pressure and pressure[0] <= 0.3 and pressure[1] >= 0.8,
            "%s events, %s pressed, pressure %s" % (tablet.get("events"), tablet.get("pressed"), pressure))
        add("pen tilt", tilt and tilt[0] <= -20 and tilt[1] >= 20, "x tilt %s, y tilt %s" % (tilt, tablet.get("y_tilt")))
    popup = by_state.get("popup", {})
    button = popup.get("button")
    popups = popup.get("popups") or []
    placed = False
    if button and popups:
        x, y = popups[0]["rect"][:2]
        placed = abs(x - button[0]) <= TOLERANCE_PX and abs(y - (button[1] + button[3])) <= TOLERANCE_PX
    add("popup", placed, "button %s, popups %s" % (button, [entry["rect"] for entry in popups]))
    docker = by_state.get("docker", {})
    add("floating docker", docker.get("found") and docker.get("floating") and docker.get("visible"),
        "rect %s" % docker.get("rect"))
    window = by_state.get("window", {})
    add("second window", window.get("count") == 2 and all(window.get("visible") or [False]),
        "visible %s" % window.get("visible"))
    add("finished", "done" in by_state, "")
    return results


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("states")
    parser.add_argument("--platform", required=True, help="expected Qt platform name, e.g. wayland or xcb")
    parser.add_argument("--no-pen", action="store_true", help="skip the pen checks, for a session without a pen device")
    args = parser.parse_args(argv)
    with open(args.states) as stream:
        states = [json.loads(line) for line in stream if line.strip()]
    results = judge(states, args.platform, pen=not args.no_pen)
    for name, ok, detail in results:
        print("%s %s %s" % ("ok  " if ok else "FAIL", name, detail))
    return 0 if all(ok for _, ok, _ in results) else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
