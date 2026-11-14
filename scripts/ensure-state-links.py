#!/usr/bin/env python3
"""Keep mutable build state outside the repository.

The repository root's ``build``, ``.cache`` and ``logs`` entries are symbolic
links into ``$LIBREPAINT_STATE_ROOT/<worktree>-<hash>/``.  A ``path:`` flake
copies the whole repository root into the Nix store, so the entries must not
be real directories.

Without arguments the script creates missing links and refuses to touch real
directories.  ``--migrate`` moves real directories into the state root and
replaces them with links.  ``--check`` reports real directories without
changing anything.
"""

import argparse
import contextlib
import fcntl
import hashlib
import os
import subprocess
import sys

STATE_NAMES = ("build", ".cache", "logs")
DEFAULT_STATE_ROOT = "~/.local/state/librepaint"


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


def worktree_state_dir(root, environ=None):
    real = os.path.realpath(root)
    digest = hashlib.sha256(real.encode("utf-8")).hexdigest()[:8]
    return os.path.join(state_root(environ), "%s-%s" % (os.path.basename(real), digest))


def state_path(root, name, environ=None):
    return os.path.join(worktree_state_dir(root, environ), name)


def _is_within(path, base):
    try:
        path = os.path.realpath(path)
        base = os.path.realpath(base)
        return os.path.commonpath([path, base]) == base
    except (OSError, ValueError):
        return False


def git_worktrees(root):
    try:
        result = subprocess.run(
            ["git", "-C", root, "worktree", "list", "--porcelain"],
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            check=False,
        )
    except OSError:
        return []
    if result.returncode != 0:
        return []
    return [
        line[len("worktree ") :]
        for line in result.stdout.splitlines()
        if line.startswith("worktree ")
    ]


def checked_roots(root):
    roots = [root]
    claude_worktrees = os.path.join(root, ".claude", "worktrees")
    if os.path.isdir(claude_worktrees):
        for name in sorted(os.listdir(claude_worktrees)):
            roots.append(os.path.join(claude_worktrees, name))
    roots.extend(git_worktrees(root))
    unique = []
    seen = set()
    for candidate in roots:
        real = os.path.realpath(candidate)
        if real in seen:
            continue
        seen.add(real)
        unique.append(candidate)
    return unique


def check_roots(root):
    violations = []
    for worktree in checked_roots(root):
        for name in STATE_NAMES:
            path = os.path.join(worktree, name)
            if os.path.isdir(path) and not os.path.islink(path):
                violations.append(path)
    return violations


def proc_cwd_entries(proc_root="/proc"):
    entries = []
    try:
        names = os.listdir(proc_root)
    except OSError:
        return entries
    for name in names:
        if not name.isdigit():
            continue
        link = os.path.join(proc_root, name, "cwd")
        try:
            cwd = os.readlink(link)
        except OSError:
            continue
        entries.append((int(name), cwd))
    return entries


@contextlib.contextmanager
def exclusive_lock(path):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "a", encoding="utf-8") as handle:
        fcntl.flock(handle, fcntl.LOCK_EX)
        try:
            yield
        finally:
            fcntl.flock(handle, fcntl.LOCK_UN)


def nearest_existing(path):
    current = os.path.abspath(path)
    while not os.path.exists(current):
        parent = os.path.dirname(current)
        if parent == current:
            break
        current = parent
    return current


def _fail(message):
    print(message, file=sys.stderr)
    raise SystemExit(1)


def _link_target(link):
    destination = os.readlink(link)
    if not os.path.isabs(destination):
        destination = os.path.normpath(os.path.join(os.path.dirname(link), destination))
    return destination


def migrate_name(root, name, proc_entries, quiet=False, environ=None):
    link = os.path.join(root, name)
    target = state_path(root, name, environ)
    root_state = state_root(environ)
    with exclusive_lock(os.path.join(root_state, ".lock")):
        if sys.platform.startswith("linux"):
            blockers = [(pid, cwd) for pid, cwd in proc_entries() if _is_within(cwd, link)]
            if blockers:
                lines = ["processes have a working directory under %s:" % link]
                lines.extend("  %d %s" % (pid, cwd) for pid, cwd in blockers)
                _fail("\n".join(lines))
        if os.path.isdir(target) and os.listdir(target):
            _fail("state directory already exists and is not empty: %s" % target)
        source_device = os.stat(link).st_dev
        ancestor = nearest_existing(target)
        if os.stat(ancestor).st_dev != source_device:
            _fail(
                "refusing to migrate across filesystems: %s -> %s" % (link, target)
            )
        os.makedirs(os.path.dirname(target), exist_ok=True)
        if os.path.lexists(target):
            os.rmdir(target)
        os.rename(link, target)
        try:
            os.symlink(target, link)
        except OSError as error:
            os.rename(target, link)
            _fail("cannot create symbolic link %s: %s" % (link, error))
    if not quiet:
        print("migrated %s -> %s" % (link, target))


def ensure_name(root, name, migrate=False, proc_entries=proc_cwd_entries, quiet=False, environ=None):
    link = os.path.join(root, name)
    target = state_path(root, name, environ)

    if os.path.islink(link):
        destination = _link_target(link)
        if os.path.isdir(destination):
            return
        if os.path.isfile(destination):
            _fail("%s points to a regular file: %s" % (link, destination))
        os.makedirs(destination, exist_ok=True)
        if not quiet:
            print("created %s for the dangling link %s" % (destination, link))
        return

    if os.path.isdir(link):
        if not migrate:
            _fail(
                "state directory is a real directory: %s\n"
                "run: python3 scripts/ensure-state-links.py --migrate" % link
            )
        migrate_name(root, name, proc_entries, quiet=quiet, environ=environ)
        return

    if os.path.lexists(link):
        _fail("%s exists and is not a directory" % link)

    os.makedirs(target, exist_ok=True)
    os.symlink(target, link)
    if not quiet:
        print("linked %s -> %s" % (link, target))


def ensure_repository(root=None, migrate=False, quiet=False, proc_entries=proc_cwd_entries, environ=None):
    root = repository_root() if root is None else root
    for name in STATE_NAMES:
        ensure_name(
            root,
            name,
            migrate=migrate,
            proc_entries=proc_entries,
            quiet=quiet,
            environ=environ,
        )


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--migrate", action="store_true", help="move real directories into the state root")
    mode.add_argument("--check", action="store_true", help="report real directories without changing anything")
    parser.add_argument("--quiet", action="store_true", help="only print errors")
    args = parser.parse_args(argv)

    try:
        state_root()
    except ConfigurationError as error:
        print("error: %s" % error, file=sys.stderr)
        return 2

    root = repository_root()

    if args.check:
        violations = check_roots(root)
        for path in violations:
            print("real state directory: %s" % path, file=sys.stderr)
        if violations:
            print(
                "run: python3 scripts/ensure-state-links.py --migrate "
                "while no build is running",
                file=sys.stderr,
            )
            return 1
        return 0

    ensure_repository(root, migrate=args.migrate, quiet=args.quiet)
    return 0


if __name__ == "__main__":
    sys.exit(main())
