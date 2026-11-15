#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check capture-based latency against Wayland presentation feedback.

A run of wayland_reference_app.c measured with mutter_capture.py holds the
capture files and the app log (reference.log, REFERENCE_LOG). The app log
gives, for every displayed frame, the pointer event it drew and the time the
compositor reported the frame as presented (wp_presentation, from the page
flip). For every captured frame this script finds the presented frame that
drew the same injected input and reports:

- injected input to app receive (input delivery),
- app receive to presented (the application and display path),
- injected input to presented (the true input-to-display time), and
- captured frame time minus presented time (the error of the capture clock).

Usage: presentation_check.py RUN_DIR [RUN_DIR ...]
"""

import bisect
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import analyze  # noqa: E402
import capture_analyze  # noqa: E402

MATCH_NS = 3_000_000


def read_log(path):
    inputs, commits, presented, discarded = [], {}, {}, 0
    with open(path) as handle:
        for line in handle:
            f = line.strip().split(",")
            if f[0] == "input":
                inputs.append(int(f[1]))
            elif f[0] == "commit":
                commits[int(f[2])] = (int(f[1]), int(f[3]))
            elif f[0] == "presented":
                presented[int(f[1])] = int(f[2])
            elif f[0] == "discarded":
                discarded += 1
    return inputs, commits, presented, discarded


def check(directory):
    injected = sorted(t for t, *_ in capture_analyze.read_inject(os.path.join(directory, "inject.csv")))
    inputs, commits, presented, discarded = read_log(os.path.join(directory, "reference.log"))

    def injection_before(t):
        i = bisect.bisect_right(injected, t) - 1
        return injected[i] if i >= 0 else None

    stats = {"inject_to_receive": [], "receive_to_presented": [], "inject_to_presented": [],
             "commit_to_presented": [], "capture_minus_presented": []}
    for t in inputs:
        source = injection_before(t)
        if source is not None and t - source < 50_000_000:
            stats["inject_to_receive"].append((t - source) / 1e6)

    # Presented frames keyed by the injection their content came from.
    shown = []
    for frame, when in presented.items():
        commit_ns, input_ns = commits.get(frame, (None, None))
        if input_ns is None or input_ns == 0:
            continue
        source = injection_before(input_ns)
        if source is None:
            continue
        stats["receive_to_presented"].append((when - input_ns) / 1e6)
        stats["inject_to_presented"].append((when - source) / 1e6)
        stats["commit_to_presented"].append((when - commit_ns) / 1e6)
        shown.append((source, when))
    shown.sort()
    sources = [s for s, _ in shown]

    _, _, _, samples = capture_analyze.frame_samples(directory)
    for sample in samples:
        kind, frame_ns, crossed = sample[0], sample[1], sample[2]
        # The captured frame shows the input injected at `crossed` (interpolated);
        # the matching presented frame drew the newest injection at or before it.
        i = bisect.bisect_right(sources, crossed + MATCH_NS) - 1
        if i < 0 or crossed - sources[i] > MATCH_NS:
            continue
        stats["capture_minus_presented"].append((frame_ns - shown[i][1]) / 1e6)

    print("== %s  presented=%d discarded=%d" % (os.path.basename(os.path.normpath(directory)), len(presented), discarded))
    for key, values in stats.items():
        print("   %-26s %s" % (key, analyze.format_stat(analyze.describe(values))))


if __name__ == "__main__":
    for directory in sys.argv[1:]:
        check(directory)
