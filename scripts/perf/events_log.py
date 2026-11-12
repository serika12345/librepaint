#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Write the measurement tools' events to one log the auditor follows.

`run.py` and `check.py` append one line per event to `build/perf/events.log`
(changed with `--events`). The format is the design in
`build/agent-loop/events-spec.md`: the ISO 8601 time to the second, the kind,
the label, the target, the run number and a detail, separated by tabs. The
file is closed after every line, so `tail -F` shows each event as it happens.

Kinds:

- run-start: the measurement group started;
- target-start: one target's run started;
- target-done: one target's run succeeded (detail: state and medians);
- target-failed: one target's run failed (detail: last line of status.txt);
- target-abandoned: the target's remaining runs were skipped;
- rest: the rest between two runs started (detail: rest_seconds);
- step: the automatic check's step started or ended (detail: step name);
- step-timeout: a step (or the target) passed its limit;
- run-done: the group ended (detail: exit code).
"""

import datetime
import os

KINDS = ("run-start", "target-start", "target-done", "target-failed",
         "target-abandoned", "rest", "step", "step-timeout", "run-done")


def line(kind, label, target, run, detail="", now=None):
    """Return one event line without the newline.

    A tab, carriage return or newline inside a field would split the line, so
    each is replaced by a space; the line always has six tab-separated fields.
    """
    if kind not in KINDS:
        raise ValueError("unknown event kind: %r" % (kind,))
    stamp = (now or datetime.datetime.now()).isoformat(timespec="seconds")
    fields = (stamp, kind, str(label), str(target), str(run), str(detail))
    return "\t".join(field.replace("\t", " ").replace("\r", " ").replace("\n", " ")
                     for field in fields)


def write(path, kind, label, target, run, detail="", now=None):
    """Append one event and close the file."""
    directory = os.path.dirname(os.path.abspath(path))
    os.makedirs(directory, exist_ok=True)
    with open(path, "a", encoding="utf-8") as handle:
        handle.write(line(kind, label, target, run, detail, now) + "\n")
