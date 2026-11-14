#!/usr/bin/env python3
"""Check that a repository cannot inflate the Nix store.

The checks mirror the Nix store hazards of this project: real state
directories at a worktree root, a repository tree large enough to be expensive
when copied as a path: flake, and little free space in /nix.
"""

import argparse
import importlib.util
import os
import sys

MAX_APPARENT_BYTES = 8 * 1024**3
DEFAULT_MIN_FREE_GIB = 100
NIX_STORE = "/nix"
GIB = 1024**3


def load_ensure_state_links():
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "ensure-state-links.py")
    spec = importlib.util.spec_from_file_location("ensure_state_links", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def directory_size(path):
    total = 0
    stack = [path]
    while stack:
        current = stack.pop()
        try:
            with os.scandir(current) as entries:
                for entry in entries:
                    try:
                        if entry.is_symlink():
                            continue
                        if entry.is_dir(follow_symlinks=False):
                            stack.append(entry.path)
                        elif entry.is_file(follow_symlinks=False):
                            total += entry.stat(follow_symlinks=False).st_size
                    except OSError:
                        continue
        except OSError:
            continue
    return total


def apparent_size(root):
    total = 0
    top_directories = []
    try:
        with os.scandir(root) as entries:
            for entry in entries:
                try:
                    if entry.is_symlink():
                        continue
                    if entry.is_dir(follow_symlinks=False):
                        size = directory_size(entry.path)
                        top_directories.append((size, entry.name))
                        total += size
                    elif entry.is_file(follow_symlinks=False):
                        total += entry.stat(follow_symlinks=False).st_size
                except OSError:
                    continue
    except OSError:
        pass
    top_directories.sort(reverse=True)
    return total, top_directories


def free_gib(path=NIX_STORE):
    stats = os.statvfs(path)
    return stats.f_bavail * stats.f_frsize / GIB


def check(root, min_free_gib=DEFAULT_MIN_FREE_GIB, skip_free=False):
    problems = []
    ensure = load_ensure_state_links()
    for path in ensure.check_roots(root):
        problems.append(
            (
                "real state directory: %s" % path,
                "run: python3 scripts/ensure-state-links.py --migrate while no build is running",
            )
        )

    total, top_directories = apparent_size(root)
    if total > MAX_APPARENT_BYTES:
        lines = [
            "repository apparent size is %.1f GiB, over the 8 GiB limit: %s"
            % (total / GIB, root),
            "a path: flake would copy this tree, including .git, into the Nix store; "
            "largest directories:",
        ]
        for size, name in top_directories[:5]:
            lines.append("  %.1f GiB  %s" % (size / GIB, name))
        problems.append(
            (
                "\n".join(lines),
                "keep large artifacts out of the repository root; never use path: references to it",
            )
        )

    if not skip_free and os.path.isdir(NIX_STORE):
        available = free_gib()
        if available < min_free_gib:
            problems.append(
                (
                    "/nix has %.1f GiB free, below the %d GiB limit"
                    % (available, min_free_gib),
                    "delete leaked -source paths or old generations; see docs/architecture/DEVELOPMENT.md",
                )
            )

    return problems


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", default=None, help="repository root to inspect")
    parser.add_argument("--min-free-gib", type=int, default=DEFAULT_MIN_FREE_GIB)
    parser.add_argument("--skip-free", action="store_true", help="do not check free space in /nix")
    args = parser.parse_args(argv)

    root = args.root or os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
    root = os.path.realpath(root)

    problems = check(root, min_free_gib=args.min_free_gib, skip_free=args.skip_free)
    for message, fix in problems:
        print("check-store-hygiene: %s" % message, file=sys.stderr)
        print("check-store-hygiene: %s" % fix, file=sys.stderr)
    if problems:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
