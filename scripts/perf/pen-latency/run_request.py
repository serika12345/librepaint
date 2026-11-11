#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build request.json for run-surface.ps1 on a Windows measurement host.

The runner passes one measurement target at a time. Target environment
variables from hosts.json reach the measured application through this file;
run-surface.ps1 applies them before it starts the application and writes them
to meta.txt. Values are strings and are never inserted into command lines.

A target can also carry "kritarc" keys. They are the root keys of kritarc (the
KConfig file whose group is the empty group), such as the hidden
useMaximumFrameLatencyOne switch. run-surface.ps1 writes them before the
application starts, backs up the file, and restores it after the run.

A display check opens one image instead of the generated canvas and captures
the whole screen at 100 % and 400 % zoom (see display_check.py).

A target can carry "surface_state": "on" or "light" to record the device state
around the run; the default is "off" (no recording). This is a runner setting,
not an environment variable for the application.

A target can also carry "surface_state_wait": "off" to skip the wait that
otherwise holds each run until the device's % Processor Performance has
recovered (two samples of at least 70 %, one Get-Counter read every five
seconds, at most 120 seconds). The wait is a runner setting as well.

A target can carry "thread_cpu": "on" to have thread_cpu.ps1 sample the
process and thread CPU counters through the run. The default is "off"; the
sampler is for the CPU comparisons, not for every latency run.

A target can carry "power": "on" to have power.ps1 sample the battery once a
second through the run. The default is "off"; the sampler is for the battery
comparisons, where the device runs on battery power.
"""

import json
import sys

ENV_NAME_CHARS = set("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")
KRITARC_KEY_CHARS = set("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-")
SURFACE_STATE_MODES = ("off", "on", "light")
SURFACE_STATE_WAIT_MODES = ("on", "off")
THREAD_CPU_MODES = ("off", "on")
POWER_MODES = ("off", "on")


def checked_env(env):
    """Return a copy of env with string values, or raise ValueError."""
    result = {}
    for name, value in (env or {}).items():
        if not name or name[0].isdigit() or not set(name) <= ENV_NAME_CHARS:
            raise ValueError("invalid environment variable name: %r" % (name,))
        text = str(value)
        if "\x00" in text or "\n" in text or "\r" in text:
            raise ValueError("invalid value for %s" % name)
        result[name] = text
    return result


def checked_kritarc(kritarc):
    """Return root kritarc keys as strings, or raise ValueError.

    Keys go before the first [Group] line, so they must be plain names. Values
    are kept free of the characters that KConfig treats as syntax.
    """
    result = {}
    for name, value in (kritarc or {}).items():
        if not name or not set(name) <= KRITARC_KEY_CHARS:
            raise ValueError("invalid kritarc key: %r" % (name,))
        text = str(value)
        if any(ch in text for ch in "\x00\r\n\\[]="):
            raise ValueError("invalid value for kritarc key %s" % name)
        result[name] = text
    return result


def checked_display_check(display_check):
    """Return the display check object with an image path, or None."""
    if not display_check:
        return None
    if not isinstance(display_check, dict):
        raise ValueError("displayCheck must be an object")
    image = str(display_check.get("image", ""))
    if not image:
        raise ValueError("displayCheck needs an image")
    if "\x00" in image or "\n" in image or "\r" in image:
        raise ValueError("invalid displayCheck image")
    return {"image": image}


def checked_surface_state(mode):
    """Return the device-state recording mode, or raise ValueError.

    The runner only knows these three names. An unknown value is rejected
    before request.json is written instead of silently recording nothing.
    """
    text = str(mode or "off")
    if text not in SURFACE_STATE_MODES:
        raise ValueError("surface_state must be one of %s: %r" % (", ".join(SURFACE_STATE_MODES), mode))
    return text


def checked_surface_state_wait(mode):
    """Return whether the runner waits for the device to recover, or raise.

    The wait is on by default so a throttled device delays a run instead of
    producing a value that cannot be compared. A target turns it off to
    measure how much the wait changes the result.
    """
    text = str(mode if mode is not None else "on")
    if text not in SURFACE_STATE_WAIT_MODES:
        raise ValueError("surface_state_wait must be one of %s: %r" % (
            ", ".join(SURFACE_STATE_WAIT_MODES), mode))
    return text


def checked_thread_cpu(mode):
    """Return whether the runner samples per-thread CPU, or raise.

    The sampler is off by default: it adds one process that reads the thread
    counters every 500 ms. A CPU comparison turns it on for both targets of
    the comparison, so the sampling cost is the same on both sides.
    """
    text = "off" if mode is None else str(mode)
    if text not in THREAD_CPU_MODES:
        raise ValueError("thread_cpu must be one of %s: %r" % (
            ", ".join(THREAD_CPU_MODES), mode))
    return text


def checked_power(mode):
    """Return whether the runner samples battery power, or raise.

    The sampler is off by default: it reads Win32_Battery and root\\wmi every
    second. A power comparison turns it on for both targets of the comparison,
    and the runner leaves AC runs out of the power aggregation.
    """
    text = "off" if mode is None else str(mode)
    if text not in POWER_MODES:
        raise ValueError("power must be one of %s: %r" % (
            ", ".join(POWER_MODES), mode))
    return text


def checked_check(check):
    """Return the automatic-check settings, or None.

    The Q4 automatic check replaces the latency trajectory with a fixed
    sequence of clipboard, window, keyboard, resize/dialog and synthetic-pen
    steps. The runner only needs to know that the mode is on and which
    thresholds the development host applies; every evidence path is chosen
    by run-surface.ps1.
    """
    if not check:
        return None
    if not isinstance(check, dict):
        raise ValueError("check must be an object")
    if not check.get("enabled", True):
        return None
    return dict(check)


def build_request(target, runs, app, inject=None, capture=False, name=None, presentmon="on",
                  presentmon_fields=None, env=None, kritarc=None, display_check=None,
                  surface_state="off", surface_state_wait="on", thread_cpu="off",
                  power="off", check=None):
    request = {
        "target": target,
        "runs": int(runs),
        "app": app,
        "inject": inject or {},
        "capture": bool(capture),
        "name": name or target,
        "presentmon": presentmon or "on",
        "surface_state": checked_surface_state(surface_state),
        "surface_state_wait": checked_surface_state_wait(surface_state_wait),
        "thread_cpu": checked_thread_cpu(thread_cpu),
        "power": checked_power(power),
    }
    request.update(presentmon_fields or {})
    checked = checked_env(env)
    if checked:
        request["env"] = checked
    kritarc_keys = checked_kritarc(kritarc)
    if kritarc_keys:
        request["kritarc"] = kritarc_keys
    display = checked_display_check(display_check)
    if display:
        request["displayCheck"] = display
    automatic = checked_check(check)
    if automatic:
        request["check"] = automatic
    return request


def main(argv):
    if len(argv) not in (15, 16):
        raise SystemExit(
            "usage: run_request.py TARGET RUNS APP INJECT_JSON CAPTURE NAME PRESENTMON "
            "PRESENTMON_JSON TARGET_ENV_JSON TARGET_KRITARC_JSON TARGET_DISPLAY_CHECK_JSON "
            "SURFACE_STATE SURFACE_STATE_WAIT THREAD_CPU POWER [CHECK_JSON]")
    (target, runs, app, inject, capture, name, presentmon, presentmon_fields, target_env,
     target_kritarc, target_display_check, surface_state, surface_state_wait, thread_cpu,
     power) = argv[:15]
    check_json = argv[15] if len(argv) == 16 else "{}"
    request = build_request(target, runs, app, json.loads(inject), capture == "true", name,
                            presentmon, json.loads(presentmon_fields), json.loads(target_env or "{}"),
                            json.loads(target_kritarc or "{}"),
                            json.loads(target_display_check or "{}"),
                            surface_state, surface_state_wait, thread_cpu, power,
                            json.loads(check_json or "{}"))
    print(json.dumps(request))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
