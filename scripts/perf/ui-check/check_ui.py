#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Judge the states.jsonl of run-ui-check.sh.

Checks, for the expected Qt platform:
- start: the main window is visible.
- popup: the brush preset popup is visible, its top-left corner is at the
  bottom-left corner of its button within TOLERANCE_PX.
- docker: the Layers docker is found, floating and visible.
- window: two main windows exist and both are visible.
Prints one line per check and exits 1 when one fails. The screenshots next to
states.jsonl show the compositor's placement, which Qt's coordinates cannot
show on Wayland.
"""

import argparse
import json
import sys

TOLERANCE_PX = 4


def judge(states, platform):
    """Return [(check, ok, detail)]."""
    by_state = {entry["state"]: entry for entry in states}
    results = []

    def add(name, ok, detail):
        results.append((name, bool(ok), detail))

    platforms = sorted({entry.get("platform") for entry in states})
    add("platform", platforms == [platform], "recorded %s" % ",".join(map(str, platforms)))
    for name in ("start", "popup", "docker", "window"):
        if "error" in by_state.get(name, {}):
            add(name, False, by_state[name]["error"])
    start = by_state.get("start", {})
    add("main window", start.get("visible"), "rect %s" % start.get("main"))
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
    args = parser.parse_args(argv)
    with open(args.states) as stream:
        states = [json.loads(line) for line in stream if line.strip()]
    results = judge(states, args.platform)
    for name, ok, detail in results:
        print("%s %s %s" % ("ok  " if ok else "FAIL", name, detail))
    return 0 if all(ok for _, ok, _ in results) else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
