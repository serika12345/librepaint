#!/usr/bin/env python3
"""Create git worktrees under one root and record them in a ledger.

Worktrees are created below ``LIBREPAINT_WORKTREE_ROOT`` (default
``~/Documents/librepaint-wt``), outside the repository, so a ``path:`` flake
cannot copy them.  Each new worktree gets the ``build``, ``.cache`` and
``logs`` symbolic links from ``ensure-state-links.py`` and one ledger entry
under ``$LIBREPAINT_STATE_ROOT/worktrees.json``.  ``--adopt`` records an
existing worktree without moving it.
"""

import argparse
import contextlib
import datetime
import fcntl
import json
import os
import re
import subprocess
import sys

NAME_PATTERN = re.compile(r"^[A-Za-z0-9._-]+$")
DEFAULT_WORKTREE_ROOT = "~/Documents/librepaint-wt"
DEFAULT_STATE_ROOT = "~/.local/state/librepaint"
LEDGER_FILE = "worktrees.json"
OWNERS = ("claude", "opencode", "user")


class ConfigurationError(Exception):
    """Invalid configuration, reported with exit code 2."""


def repository_root():
    return os.path.realpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))


def state_root(environ=None):
    environ = os.environ if environ is None else environ
    expanded = os.path.expanduser(environ.get("LIBREPAINT_STATE_ROOT", DEFAULT_STATE_ROOT))
    if not os.path.isabs(expanded):
        raise ConfigurationError(
            "LIBREPAINT_STATE_ROOT must be an absolute path: %s" % expanded
        )
    return os.path.normpath(expanded)


def worktree_root(environ=None):
    environ = os.environ if environ is None else environ
    expanded = os.path.expanduser(environ.get("LIBREPAINT_WORKTREE_ROOT", DEFAULT_WORKTREE_ROOT))
    if not os.path.isabs(expanded):
        raise ConfigurationError(
            "LIBREPAINT_WORKTREE_ROOT must be an absolute path: %s" % expanded
        )
    return os.path.normpath(expanded)


def ledger_path(environ=None):
    return os.path.join(state_root(environ), LEDGER_FILE)


@contextlib.contextmanager
def ledger_lock(environ=None, exclusive=False):
    path = ledger_path(environ)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "a+", encoding="utf-8") as handle:
        fcntl.flock(handle, fcntl.LOCK_EX if exclusive else fcntl.LOCK_SH)
        try:
            yield handle
        finally:
            fcntl.flock(handle, fcntl.LOCK_UN)


def read_ledger(handle):
    handle.seek(0)
    text = handle.read()
    if not text.strip():
        return {}
    try:
        data = json.loads(text)
    except ValueError as error:
        raise ConfigurationError("worktree ledger is not valid JSON: %s" % error)
    if not isinstance(data, dict):
        raise ConfigurationError("worktree ledger is not a JSON object")
    return data


def load_ledger(environ=None):
    if not os.path.exists(ledger_path(environ)):
        return {}
    with ledger_lock(environ) as handle:
        return read_ledger(handle)


def update_ledger(environ, mutate):
    with ledger_lock(environ, exclusive=True) as handle:
        data = read_ledger(handle)
        mutate(data)
        handle.seek(0)
        handle.truncate()
        json.dump(data, handle, ensure_ascii=False, indent=2, sort_keys=True)
        handle.write("\n")
        handle.flush()
        os.fsync(handle.fileno())
        return data


def timestamp():
    return datetime.datetime.now(datetime.timezone.utc).replace(microsecond=0).isoformat()


def is_within(path, base):
    try:
        path = os.path.realpath(path)
        base = os.path.realpath(base)
        return os.path.commonpath([path, base]) == base
    except (OSError, ValueError):
        return False


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


def git_worktree_paths(root):
    result = run_git(root, ["worktree", "list", "--porcelain"])
    if result.returncode != 0:
        return []
    return [
        line[len("worktree ") :]
        for line in result.stdout.splitlines()
        if line.startswith("worktree ")
    ]


def validate_name(name):
    if not name or not NAME_PATTERN.match(name) or ".." in name:
        raise ConfigurationError(
            "NAME must match [A-Za-z0-9._-]+ and must not contain '..': %r" % name
        )


def validate_purpose(purpose):
    if not purpose or not purpose.strip():
        raise ConfigurationError("--purpose must not be empty")


def ensure_links(worktree):
    script = os.path.join(worktree, "scripts", "ensure-state-links.py")
    if not os.path.isfile(script):
        print("missing script in new worktree: %s" % script, file=sys.stderr)
        return False
    result = subprocess.run(
        [sys.executable, script],
        cwd=worktree,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        print(result.stderr.strip(), file=sys.stderr)
        return False
    return True


def branch_or_ref(branch, new_branch, detach, name):
    if new_branch:
        return new_branch
    if branch:
        return branch
    if detach:
        return detach
    return name


def create_worktree(name, purpose, owner, branch, new_branch, from_ref, detach, environ=None):
    validate_name(name)
    validate_purpose(purpose)

    repository = repository_root()
    root = worktree_root(environ)
    path = os.path.abspath(os.path.join(root, name))

    if is_within(path, repository):
        raise ConfigurationError(
            "worktree path must be outside the repository: %s" % path
        )
    if os.path.lexists(path):
        print("worktree path already exists: %s" % path, file=sys.stderr)
        return 1

    os.makedirs(root, exist_ok=True)
    command = ["worktree", "add"]
    if new_branch:
        command += ["-b", new_branch, path]
        if from_ref:
            command.append(from_ref)
    elif branch:
        command += [path, branch]
    elif detach:
        command += ["--detach", path, detach]
    else:
        command.append(path)

    result = run_git(repository, command)
    if result.returncode != 0:
        print(result.stderr.strip() or "git worktree add failed", file=sys.stderr)
        return 1

    if not ensure_links(path):
        print(
            "created the worktree but could not create its state links: %s" % path,
            file=sys.stderr,
        )
        return 1

    record = {
        "name": name,
        "purpose": purpose,
        "owner": owner,
        "branch_or_ref": branch_or_ref(branch, new_branch, detach, name),
        "created": timestamp(),
    }
    update_ledger(environ, lambda data: data.__setitem__(path, record))
    print(path)
    return 0


def adopt_worktree(path, purpose, owner, environ=None):
    validate_purpose(purpose)

    repository = repository_root()
    target = os.path.realpath(path)
    if not os.path.isdir(target):
        raise ConfigurationError("not a directory: %s" % path)

    match = None
    for candidate in git_worktree_paths(repository):
        if os.path.realpath(candidate) == target:
            match = candidate
            break
    if match is None:
        raise ConfigurationError("not a worktree of this repository: %s" % path)

    name = os.path.basename(target)
    validate_name(name)

    branch = None
    result = run_git(repository, ["worktree", "list", "--porcelain"])
    if result.returncode == 0:
        for block in result.stdout.split("\n\n"):
            if ("worktree %s" % match) not in block.splitlines():
                continue
            for line in block.splitlines():
                if line.startswith("branch refs/heads/"):
                    branch = line[len("branch refs/heads/") :]
                elif line.startswith("HEAD "):
                    branch = branch or line[len("HEAD ") :]
            break

    record = {
        "name": name,
        "purpose": purpose,
        "owner": owner,
        "branch_or_ref": branch or "",
        "created": timestamp(),
    }
    update_ledger(environ, lambda data: data.__setitem__(target, record))
    print(target)
    return 0


def parse_arguments(argv):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("name", nargs="?", help="worktree name below LIBREPAINT_WORKTREE_ROOT")
    parser.add_argument("--purpose", required=True, help="why the worktree exists")
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--branch", help="check out an existing branch")
    group.add_argument("--new-branch", help="create a branch and check it out")
    group.add_argument("--detach", help="check out a commit without a branch")
    parser.add_argument("--from", dest="from_ref", help="start point for --new-branch")
    parser.add_argument("--owner", choices=OWNERS, default="user")
    parser.add_argument("--adopt", help="register an existing worktree without moving it")
    return parser.parse_args(argv)


def main(argv=None):
    args = parse_arguments(argv)
    try:
        if args.adopt:
            if args.name or args.branch or args.new_branch or args.detach or args.from_ref:
                raise ConfigurationError(
                    "--adopt cannot be combined with NAME or branch options"
                )
            return adopt_worktree(args.adopt, args.purpose, args.owner)
        if not args.name:
            raise ConfigurationError("NAME is required")
        if args.from_ref and not args.new_branch:
            raise ConfigurationError("--from requires --new-branch")
        return create_worktree(
            args.name,
            args.purpose,
            args.owner,
            args.branch,
            args.new_branch,
            args.from_ref,
            args.detach,
        )
    except ConfigurationError as error:
        print("new-worktree: %s" % error, file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
