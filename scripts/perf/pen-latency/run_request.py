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
"""

import json
import sys

ENV_NAME_CHARS = set("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")
KRITARC_KEY_CHARS = set("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-")
SURFACE_STATE_MODES = ("off", "on", "light")


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


def build_request(target, runs, app, inject=None, capture=False, name=None, presentmon="on",
                  presentmon_fields=None, env=None, kritarc=None, display_check=None,
                  surface_state="off"):
    request = {
        "target": target,
        "runs": int(runs),
        "app": app,
        "inject": inject or {},
        "capture": bool(capture),
        "name": name or target,
        "presentmon": presentmon or "on",
        "surface_state": checked_surface_state(surface_state),
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
    return request


def main(argv):
    if len(argv) != 12:
        raise SystemExit(
            "usage: run_request.py TARGET RUNS APP INJECT_JSON CAPTURE NAME PRESENTMON "
            "PRESENTMON_JSON TARGET_ENV_JSON TARGET_KRITARC_JSON TARGET_DISPLAY_CHECK_JSON "
            "SURFACE_STATE")
    (target, runs, app, inject, capture, name, presentmon, presentmon_fields, target_env,
     target_kritarc, target_display_check, surface_state) = argv
    request = build_request(target, runs, app, json.loads(inject), capture == "true", name,
                            presentmon, json.loads(presentmon_fields), json.loads(target_env or "{}"),
                            json.loads(target_kritarc or "{}"),
                            json.loads(target_display_check or "{}"),
                            surface_state)
    print(json.dumps(request))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
