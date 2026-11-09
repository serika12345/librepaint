#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build request.json for run-surface.ps1 on a Windows measurement host.

The runner passes one measurement target at a time. Target environment
variables from hosts.json reach the measured application through this file;
run-surface.ps1 applies them before it starts the application and writes them
to meta.txt. Values are strings and are never inserted into command lines.
"""

import json
import sys

ENV_NAME_CHARS = set("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")


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


def build_request(target, runs, app, inject=None, capture=False, name=None, presentmon="on",
                  presentmon_fields=None, env=None):
    request = {
        "target": target,
        "runs": int(runs),
        "app": app,
        "inject": inject or {},
        "capture": bool(capture),
        "name": name or target,
        "presentmon": presentmon or "on",
    }
    request.update(presentmon_fields or {})
    checked = checked_env(env)
    if checked:
        request["env"] = checked
    return request


def main(argv):
    if len(argv) != 9:
        raise SystemExit(
            "usage: run_request.py TARGET RUNS APP INJECT_JSON CAPTURE NAME PRESENTMON "
            "PRESENTMON_JSON TARGET_ENV_JSON")
    target, runs, app, inject, capture, name, presentmon, presentmon_fields, target_env = argv
    request = build_request(target, runs, app, json.loads(inject), capture == "true", name,
                            presentmon, json.loads(presentmon_fields), json.loads(target_env or "{}"))
    print(json.dumps(request))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
