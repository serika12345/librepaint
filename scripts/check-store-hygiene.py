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
MIN_FREE_GIB_MAX = 100
MIN_FREE_GIB_FLOOR = 30
MIN_FREE_GIB_FRACTION = 0.15
NIX_STORE = "/nix"
GIB = 1024**3


def load_ensure_state_links():
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "ensure-state-links.py")
    spec = importlib.util.spec_from_file_location("ensure_state_links", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def load_worktree_gc():
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "worktree-gc.py")
    spec = importlib.util.spec_from_file_location("worktree_gc", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


WORKTREE_VIOLATIONS = ("prunable", "inside-repo", "unregistered")
WORKTREE_FIXES = {
    "prunable": "run: python3 scripts/worktree-gc.py --apply",
    "inside-repo": "remove it and create worktrees outside the repository",
    "unregistered": "register it with: python3 scripts/new-worktree.py --adopt <path> --purpose <reason>",
}


def worktree_report(root):
    return load_worktree_gc().classify(root)


def worktree_problems(root):
    problems = []
    for entry in worktree_report(root)["entries"]:
        category = entry["category"]
        if category in WORKTREE_VIOLATIONS:
            problems.append(
                (
                    "worktree %s: %s" % (category, entry["path"]),
                    WORKTREE_FIXES[category],
                )
            )
    return problems


def worktree_warnings(root):
    report = worktree_report(root)
    warnings = []
    for entry in report["entries"]:
        if entry["category"] == "eligible":
            warnings.append(
                (
                    "worktree is eligible for removal: %s" % entry["path"],
                    "run: python3 scripts/worktree-gc.py --apply",
                )
            )
    if len(report["ledger"]) > 4:
        warnings.append(
            (
                "the worktree ledger records %d worktrees, over 4"
                % len(report["ledger"]),
                "remove finished worktrees with: python3 scripts/worktree-gc.py --apply",
            )
        )
    return warnings



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


def volume_gib(path=NIX_STORE):
    stats = os.statvfs(path)
    return stats.f_blocks * stats.f_frsize / GIB


def minimum_free_gib(volume):
    """Smallest allowed free space, in GiB, for a volume of ``volume`` GiB."""
    scaled = int(volume * MIN_FREE_GIB_FRACTION)
    return min(MIN_FREE_GIB_MAX, max(MIN_FREE_GIB_FLOOR, scaled))


def free_space_limit(path=NIX_STORE, min_free_gib=None):
    """Return the applied free-space limit in GiB and the text of its basis."""
    if min_free_gib is not None:
        return min_free_gib, "--min-free-gib"
    volume = volume_gib(path)
    return minimum_free_gib(volume), "%d%% of the %.0f GiB volume" % (
        int(MIN_FREE_GIB_FRACTION * 100),
        volume,
    )


def check(root, min_free_gib=None, skip_free=False):
    problems = []
    ensure = load_ensure_state_links()
    for path in ensure.check_roots(root):
        problems.append(
            (
                "real state directory: %s" % path,
                "run: python3 scripts/ensure-state-links.py --migrate while no build is running",
            )
        )

    problems.extend(worktree_problems(root))

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
        limit, basis = free_space_limit(min_free_gib=min_free_gib)
        if available < limit:
            problems.append(
                (
                    "/nix has %.1f GiB free, below the %d GiB limit (%s)"
                    % (available, limit, basis),
                    "delete leaked -source paths or old generations; see docs/architecture/DEVELOPMENT.md",
                )
            )

    return problems


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", default=None, help="repository root to inspect")
    parser.add_argument(
        "--min-free-gib",
        type=int,
        default=None,
        help="lower bound in GiB; defaults to 15%% of the volume, between {0} and {1}".format(
            MIN_FREE_GIB_FLOOR, MIN_FREE_GIB_MAX
        ),
    )
    parser.add_argument("--skip-free", action="store_true", help="do not check free space in /nix")
    args = parser.parse_args(argv)

    root = args.root or os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
    root = os.path.realpath(root)

    problems = check(root, min_free_gib=args.min_free_gib, skip_free=args.skip_free)
    for message, fix in problems:
        print("check-store-hygiene: %s" % message, file=sys.stderr)
        print("check-store-hygiene: %s" % fix, file=sys.stderr)
    for message, fix in worktree_warnings(root):
        print("check-store-hygiene: warning: %s" % message, file=sys.stderr)
        print("check-store-hygiene: warning: %s" % fix, file=sys.stderr)
    if problems:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
