#!/usr/bin/env python3
"""Watch /nix/store for leaked copies of the repository.

Every store path whose name ends in ``-source`` and whose NAR size exceeds
1 GiB is recorded once.  Copies that still contain a repository tree are
deleted when no other path refers to them.  The script also warns through
``build/agent-loop/oc.sh`` when /nix runs low.
"""

import argparse
import json
import os
import subprocess
import sys
import time

GIB = 1024**3
STORE = "/nix/store"
MIN_CANDIDATE_BYTES = 1 * GIB
MIN_FREE_GIB = 100
WARNING_INTERVAL_SECONDS = 3600


class StoreError(Exception):
    pass


def repository_root():
    return os.path.realpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))


def state_root():
    return os.path.normpath(
        os.path.expanduser(os.environ.get("LIBREPAINT_STATE_ROOT", "~/.local/state/librepaint"))
    )


def state_file():
    return os.path.join(state_root(), "store-guard.state")


def log_file():
    return os.path.join(state_root(), "store-guard.log")


def nix_calls_log():
    return os.path.join(state_root(), "nix-calls.log")


def oc_sh():
    return os.path.join(repository_root(), "build", "agent-loop", "oc.sh")


def now():
    return time.time()


def run_command(argv):
    return subprocess.run(
        argv,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=False,
    )


def query_path_info(paths):
    result = run_command(["nix", "path-info", "--size", "--json"] + list(paths))
    if result.returncode != 0:
        raise StoreError(result.stderr.strip() or "nix path-info failed")
    data = json.loads(result.stdout or "[]")
    if isinstance(data, dict):
        return data
    return {entry["path"]: entry for entry in data}


def process_table():
    result = run_command(["ps", "-eo", "pid,ppid,etime,args"])
    if result.returncode != 0:
        return ""
    return result.stdout


def delete_store_path(path):
    return run_command(["nix-store", "--delete", path])


def query_referrers(path):
    return run_command(["nix-store", "--query", "--referrers", path])


def send_event(message):
    script = oc_sh()
    if not os.path.isfile(script):
        return False
    result = run_command([script, "event", message])
    return result.returncode == 0


def free_gib(path="/nix"):
    stats = os.statvfs(path)
    return stats.f_bavail * stats.f_frsize / GIB


def source_candidates():
    try:
        names = os.listdir(STORE)
    except OSError as error:
        raise StoreError("cannot list %s: %s" % (STORE, error))
    return [os.path.join(STORE, name) for name in sorted(names) if name.endswith("-source")]


def load_state():
    try:
        with open(state_file(), "r", encoding="utf-8") as handle:
            data = json.load(handle)
    except (OSError, ValueError):
        return {"known": {}, "last_warning": 0, "delete_forbidden": False}
    if not isinstance(data, dict):
        return {"known": {}, "last_warning": 0, "delete_forbidden": False}
    data.setdefault("known", {})
    data.setdefault("last_warning", 0)
    data.setdefault("delete_forbidden", False)
    return data


def save_state(state):
    path = state_file()
    try:
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path + ".tmp", "w", encoding="utf-8") as handle:
            json.dump(state, handle, indent=2, sort_keys=True)
            handle.write("\n")
        os.replace(path + ".tmp", path)
    except OSError:
        pass


def single_line(text):
    return " | ".join(line for line in (text or "").splitlines() if line)


def append_log(record):
    path = log_file()
    try:
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "a", encoding="utf-8") as handle:
            handle.write(json.dumps(record, ensure_ascii=False, sort_keys=True) + "\n")
    except OSError:
        pass


def first_seen_record(path, info):
    registration = info.get("registrationTime") or 0
    calls = []
    try:
        with open(nix_calls_log(), "r", encoding="utf-8", errors="replace") as handle:
            lines = handle.readlines()[-20:]
    except OSError:
        lines = []
    for line in lines:
        epoch = None
        for field in line.split():
            if field.startswith("epoch="):
                try:
                    epoch = int(field[len("epoch=") :])
                except ValueError:
                    epoch = None
                break
        if epoch is not None and registration and abs(epoch - registration) <= 120:
            calls.append(line.rstrip("\n"))

    processes = []
    for line in process_table().splitlines():
        if any(word in line for word in ("nix", "direnv", "opencode", "systemd-run")):
            processes.append(line)
    processes = processes[:30]

    referrers_result = query_referrers(path)
    referrers_text = ""
    if referrers_result.returncode == 0:
        referrers_text = single_line(referrers_result.stdout)

    return {
        "time": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime(now())),
        "path": path,
        "narSize": info.get("narSize"),
        "registrationTime": registration,
        "nix_calls": calls,
        "processes": processes,
        "referrers": referrers_text,
    }


def delete_eligible(path):
    if not os.path.isdir(path):
        return False
    if not os.path.lexists(os.path.join(path, ".git")):
        return False
    if not os.path.lexists(os.path.join(path, "flake.nix")):
        return False
    return any(os.path.lexists(os.path.join(path, name)) for name in ("build", ".cache", "po"))


def run(dry_run=False):
    state = load_state()
    report = {"candidates": [], "new": [], "deleted": [], "refused": [], "warning": ""}

    try:
        candidates = source_candidates()
        info = query_path_info(candidates) if candidates else {}
    except (StoreError, ValueError) as error:
        message = "store-guard: %s" % error
        print(message, file=sys.stderr)
        if not dry_run:
            append_log({"time": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime(now())), "error": str(error)})
        return 1

    known = state["known"]
    large = {
        path: entry
        for path, entry in info.items()
        if (entry.get("narSize") or 0) > MIN_CANDIDATE_BYTES
    }

    for path, entry in sorted(large.items()):
        report["candidates"].append(path)
        if path in known:
            continue
        record = first_seen_record(path, entry)
        report["new"].append(record)
        if dry_run:
            print(json.dumps(record, ensure_ascii=False, sort_keys=True))
        else:
            append_log(record)
            known[path] = {
                "narSize": entry.get("narSize"),
                "registrationTime": entry.get("registrationTime"),
            }

    if not dry_run:
        for path in sorted(large):
            if not delete_eligible(path):
                continue
            if state["delete_forbidden"]:
                continue
            result = delete_store_path(path)
            stamp = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime(now()))
            output = single_line((result.stdout or "") + (result.stderr or ""))
            if result.returncode == 0:
                report["deleted"].append(path)
                append_log({"time": stamp, "deleted": path})
                continue
            permission_denied = "permission denied" in output.lower()
            append_log(
                {
                    "time": stamp,
                    "refused": path,
                    "permission_denied": permission_denied,
                    "output": output,
                }
            )
            report["refused"].append(path)
            if permission_denied:
                state["delete_forbidden"] = True
                append_log(
                    {
                        "time": stamp,
                        "delete_disabled": True,
                        "reason": "deleting store paths requires privileges not held by this user",
                    }
                )
                break

    try:
        available = free_gib()
    except OSError as error:
        report["warning"] = ""
        if not dry_run:
            append_log(
                {
                    "time": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime(now())),
                    "error": "cannot read /nix free space: %s" % error,
                }
            )
        available = None

    if available is not None and available < MIN_FREE_GIB:
        stamp = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime(now()))
        report["warning"] = "%.1f GiB free" % available
        if dry_run:
            print("store-guard: /nix free space is %s" % report["warning"])
        elif now() - float(state.get("last_warning") or 0) >= WARNING_INTERVAL_SECONDS:
            message = "停止: Nix ストアの空きが 100GiB 未満（%.1fGiB）。store-guard のログを確認する" % available
            if send_event(message):
                state["last_warning"] = now()
            else:
                append_log({"time": stamp, "warning": message, "delivery": "failed"})

    if not dry_run:
        save_state(state)

    if dry_run:
        print(
            "store-guard: %d oversized candidates, %d new, %d deleted, %d refused"
            % (
                len(report["candidates"]),
                len(report["new"]),
                len(report["deleted"]),
                len(report["refused"]),
            )
        )
    return 0


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dry-run", action="store_true", help="report only; write no state or log and delete nothing")
    args = parser.parse_args(argv)
    return run(dry_run=args.dry_run)


if __name__ == "__main__":
    sys.exit(main())
