#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""PresentMon 2.x command line for the Windows pen-latency measurement.

The Windows runner in run-surface.ps1 starts PresentMon for every run. This
module owns the command line so that it can be checked on a machine without
Windows PowerShell. run-remote.sh writes the result into request.json, and the
PowerShell runner only replaces the {pid} and {output} placeholders.

The session name is fixed and short. ETW allows about 64 sessions system-wide,
and Microsoft asks controllers to remove an orphaned session instead of
starting a second one (StartTraceW, evntrace.h). Names that change on every
run leave one session behind whenever PresentMon is killed, so the runner
stops this fixed session before and after each measurement.
"""

import json

# Short ASCII name. A dotted or long name makes failures hard to read in
# logman output; 16 characters keeps the command line easy to copy.
SESSION_NAME = "LibrePaintPenLatency"

# The runner substitutes these placeholders after it has started the app.
PID_PLACEHOLDER = "{pid}"
OUTPUT_PLACEHOLDER = "{output}"


def argument_template():
    """Arguments for one run, with placeholders for values known at launch."""
    return [
        "--process_id", PID_PLACEHOLDER,
        "--output_file", OUTPUT_PLACEHOLDER,
        "--qpc_time",
        "--no_console_stats",
        "--terminate_on_proc_exit",
        "--session_name", SESSION_NAME,
    ]


def request_fields():
    """Fields that run-remote.sh adds to request.json."""
    return {
        "presentmon_session": SESSION_NAME,
        "presentmon_arguments": argument_template(),
    }


def render(pid, output):
    """Return the arguments with the runtime placeholders filled in."""
    return [value.replace(PID_PLACEHOLDER, str(pid)).replace(OUTPUT_PLACEHOLDER, output)
            for value in argument_template()]


if __name__ == "__main__":
    print(json.dumps(request_fields()))
