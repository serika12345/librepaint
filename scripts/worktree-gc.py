#!/usr/bin/env python3
"""List and remove unused git worktrees.

The script compares ``git worktree list --porcelain`` with the ledger written
by ``new-worktree.py`` and classifies every worktree except the main one.  The
default is a listing only.  ``--apply`` prunes missing worktrees and removes
the worktrees that are safe to delete, together with their ledger entry and
their state directory.
"""

import argparse
import importlib.util
import os
import shutil
import subprocess
import sys
import time

DEFAULT_DAYS = 7
CATEGORIES = ("prunable", "unregistered", "inside-repo", "eligible", "needs-review")
ACTIONABLE = ("prunable", "unregistered", "eligible")
DAY_SECONDS = 24 * 60 * 60


def load_module(module_name, filename):
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), filename)
    spec = importlib.util.spec_from_file_location(module_name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


new_worktree = load_module("new_worktree", "new-worktree.py")
ensure_links = load_module("ensure_state_links", "ensure-state-links.py")


def repository_root():
    return os.path.realpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))


def run_git(root, arguments):
    try:
        return subprocess.run(
            ["git", "-C", root] + list(arguments),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=False,
        )
    except OSError as error:
        return subprocess.CompletedProcess([], 1, "", str(error))


def git_worktrees(root):
    result = run_git(root, ["worktree", "list", "--porcelain"])
    if result.returncode != 0:
        return []
    entries = []
    current = None
    for line in result.stdout.splitlines():
        if line.startswith("worktree "):
            if current is not None:
                entries.append(current)
            current = {"path": line[len("worktree ") :]}
            continue
        if current is None:
            continue
        if line.startswith("HEAD "):
            current["head"] = line[len("HEAD ") :]
        elif line.startswith("branch "):
            current["branch"] = line[len("branch ") :]
        elif line == "detached":
            current["detached"] = True
        elif line.startswith("prunable"):
            current["prunable"] = True
        elif line.startswith("locked"):
            current["locked"] = True
    if current is not None:
        entries.append(current)
    return entries


def branch_name(entry):
    ref = entry.get("branch")
    if ref and ref.startswith("refs/heads/"):
        return ref[len("refs/heads/") :]
    return None


def working_tree_state(path):
    result = run_git(path, ["status", "--porcelain"])
    if result.returncode != 0:
        return True, False
    lines = [line for line in result.stdout.splitlines() if line.strip()]
    modified = any(not line.startswith("??") for line in lines)
    untracked = any(line.startswith("??") for line in lines)
    return modified, untracked


def head_contains_pushed_commit(path, entry):
    head = entry.get("head")
    branch = branch_name(entry)
    if branch:
        result = run_git(path, ["rev-parse", "--verify", "refs/remotes/origin/%s" % branch])
        return result.returncode == 0 and result.stdout.strip() == head
    result = run_git(path, ["branch", "-r", "--contains", head or "HEAD"])
    return result.returncode == 0 and bool(result.stdout.strip())


def last_commit_time(path):
    result = run_git(path, ["log", "-1", "--format=%ct"])
    if result.returncode != 0:
        return None
    try:
        return int(result.stdout.strip())
    except ValueError:
        return None


def stash_related(path, branch):
    result = run_git(path, ["stash", "list"])
    if result.returncode != 0:
        return False
    messages = [line for line in result.stdout.splitlines() if line.strip()]
    if not messages:
        return False
    if branch:
        return any(branch in line for line in messages)
    return True


def processes_below(path, proc_entries):
    return [(pid, cwd) for pid, cwd in proc_entries() if ensure_links._is_within(cwd, path)]


def ledger_keys_for(ledger, worktrees):
    real_paths = {os.path.realpath(entry["path"]) for entry in worktrees}
    return [key for key in ledger if os.path.realpath(key) in real_paths]


def classify(root=None, days=DEFAULT_DAYS, environ=None, proc_entries=None, now=None):
    root = repository_root() if root is None else os.path.realpath(root)
    proc_entries = ensure_links.proc_cwd_entries if proc_entries is None else proc_entries
    now = time.time() if now is None else now

    try:
        ledger = new_worktree.load_ledger(environ)
    except new_worktree.ConfigurationError:
        ledger = {}

    worktrees = git_worktrees(root)
    registered = {os.path.realpath(key) for key in ledger}
    report = {
        "repository": root,
        "main": worktrees[0]["path"] if worktrees else None,
        "entries": [],
        "ledger": ledger_keys_for(ledger, worktrees),
    }

    for entry in worktrees[1:]:
        path = entry["path"]
        result = {
            "path": path,
            "branch": branch_name(entry),
            "category": None,
            "reasons": [],
            "hint": "",
        }

        if not os.path.isdir(path):
            result["category"] = "prunable"
        elif os.path.realpath(path) not in registered:
            result["category"] = "unregistered"
            result["hint"] = "new-worktree.py --adopt で登録するか、利用者に確認"
        elif ensure_links._is_within(path, root):
            result["category"] = "inside-repo"
            result["hint"] = "check-store-hygiene.py が違反にする"
        else:
            reasons = result["reasons"]
            modified, untracked = working_tree_state(path)
            if modified:
                reasons.append("変更あり")
            if untracked:
                reasons.append("未追跡のファイルあり")
            if not head_contains_pushed_commit(path, entry):
                reasons.append("push されていない")
            committed = last_commit_time(path)
            if committed is None:
                reasons.append("最終コミットが不明")
            elif now - committed <= days * DAY_SECONDS:
                reasons.append("最終コミットが %d 日以内" % days)
            blockers = processes_below(path, proc_entries)
            if blockers:
                reasons.append("プロセスあり")
            if stash_related(path, result["branch"]):
                reasons.append("スタッシュあり")
            result["category"] = "needs-review" if reasons else "eligible"

        report["entries"].append(result)

    return report


def by_category(report):
    grouped = {category: [] for category in CATEGORIES}
    for entry in report["entries"]:
        grouped[entry["category"]].append(entry)
    return grouped


def format_report(report):
    lines = []
    grouped = by_category(report)
    for category in CATEGORIES:
        for entry in grouped[category]:
            line = "%s: %s" % (category, entry["path"])
            if entry["reasons"]:
                line += " (%s)" % ", ".join(entry["reasons"])
            lines.append(line)
            if entry.get("hint"):
                lines.append("  %s" % entry["hint"])
    if not lines:
        lines.append("worktree-gc: no worktrees other than the main one")
    return lines


def remove_ledger_entry(path, environ):
    target = os.path.realpath(path)

    def mutate(data):
        for key in [key for key in data if os.path.realpath(key) == target]:
            del data[key]

    new_worktree.update_ledger(environ, mutate)


def state_dir_in_use(state_dir, path, report):
    target = os.path.realpath(state_dir)
    others = [entry["path"] for entry in report["entries"] if entry["path"] != path]
    if report.get("main"):
        others.append(report["main"])
    for other in others:
        if not os.path.isdir(other):
            continue
        for name in ensure_links.STATE_NAMES:
            link = os.path.join(other, name)
            if not os.path.islink(link):
                continue
            if os.path.realpath(ensure_links._link_target(link)) == target:
                return other
    return None


def remove_state_dir(path, report, environ):
    state_dir = ensure_links.worktree_state_dir(path, environ)
    if not os.path.isdir(state_dir):
        return ""
    root = ensure_links.state_root(environ)
    if not ensure_links._is_within(state_dir, root) or os.path.realpath(state_dir) == os.path.realpath(root):
        return "kept state directory outside the state root: %s" % state_dir
    in_use = state_dir_in_use(state_dir, path, report)
    if in_use:
        return "kept state directory used by %s: %s" % (in_use, state_dir)
    try:
        shutil.rmtree(state_dir)
    except OSError as error:
        return "cannot remove state directory %s: %s" % (state_dir, error)
    return "removed state: %s" % state_dir


def remove_worktree(path, report, environ):
    result = run_git(report["repository"], ["worktree", "remove", path])
    if result.returncode != 0:
        links = [os.path.join(path, name) for name in ensure_links.STATE_NAMES]
        if not all(os.path.islink(link) for link in links):
            return ["failed: %s (%s)" % (path, (result.stderr or "").strip())]
        for link in links:
            os.unlink(link)
        result = run_git(report["repository"], ["worktree", "remove", path])
        if result.returncode != 0:
            return ["failed: %s (%s)" % (path, (result.stderr or "").strip())]

    lines = ["removed: %s" % path]
    remove_ledger_entry(path, environ)
    message = remove_state_dir(path, report, environ)
    if message:
        lines.append(message)
    return lines


def apply_report(report, environ=None):
    lines = []
    failures = 0

    prunable = [entry for entry in report["entries"] if entry["category"] == "prunable"]
    if prunable:
        result = run_git(report["repository"], ["worktree", "prune"])
        if result.returncode != 0:
            failures += 1
            lines.append("failed to prune: %s" % (result.stderr or "").strip())
        else:
            lines.extend("pruned: %s" % entry["path"] for entry in prunable)

    for entry in [entry for entry in report["entries"] if entry["category"] == "eligible"]:
        result = remove_worktree(entry["path"], report, environ)
        lines.extend(result)
        if any(line.startswith("failed:") for line in result):
            failures += 1

    return lines, failures


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--apply", action="store_true", help="prune missing worktrees and remove eligible ones")
    parser.add_argument("--days", type=int, default=DEFAULT_DAYS, help="minimum age of the last commit, in days")
    args = parser.parse_args(argv)

    try:
        report = classify(days=args.days)
    except new_worktree.ConfigurationError as error:
        print("worktree-gc: %s" % error, file=sys.stderr)
        return 2

    if args.apply:
        lines, failures = apply_report(report)
        for line in lines:
            print(line)
        return 1 if failures else 0

    for line in format_report(report):
        print(line)
    return 0


if __name__ == "__main__":
    sys.exit(main())
