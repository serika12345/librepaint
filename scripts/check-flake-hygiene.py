#!/usr/bin/env python3
"""Check flake.nix and nix/ for diagnostic-only build options and markers.

The flake and the Nix expressions must not carry experimental build
options, temporary relaxations, or experiment markers: diagnostics run
through evaluation-time overrides and --keep-failed instead.  Lines that
exist today and explain why they exist on the same line or the line before
are exempt from the build-option rules.
"""

import argparse
import os
import re
import sys

FLAKE = "flake.nix"
NIX_DIR = "nix"

REASON_MARKERS = ("nixpkgs", "Issue #", "remove when", "drop when", "removal")
MARKERS = ("EXPERIMENT", "XXX", "TEMP", "DEBUG ONLY")

FLAG_ATTR = re.compile(r"\b(?:ninjaFlags|makeFlags|buildFlags)\s*=")
K_FLAG = re.compile(r"(?<![A-Za-z0-9_-])-k(?![A-Za-z0-9_-])")
BUILD_OPTION_PATTERNS = (
    ("NIX_DEBUG", re.compile(r"\bNIX_DEBUG\b")),
    ("enableDebugging", re.compile(r"\benableDebugging\b")),
    ("dontStrip = true", re.compile(r"\bdontStrip\s*=\s*true\b")),
    ("separateDebugInfo", re.compile(r"\bseparateDebugInfo\b")),
)

RULES = (
    "buildOptions: -k in ninjaFlags, makeFlags, or buildFlags",
    "buildOptions: NIX_DEBUG",
    "buildOptions: enableDebugging",
    "buildOptions: dontStrip = true",
    "buildOptions: separateDebugInfo",
    "markers: EXPERIMENT",
    "markers: XXX",
    "markers: TEMP",
    "markers: DEBUG ONLY",
)


def nix_files(root):
    """Return flake.nix plus nix/**/*.nix, in a stable order."""
    paths = []
    flake = os.path.join(root, FLAKE)
    if os.path.isfile(flake):
        paths.append(flake)
    for base, _directories, names in os.walk(os.path.join(root, NIX_DIR)):
        for name in names:
            if name.endswith(".nix"):
                paths.append(os.path.join(base, name))
    return sorted(paths)


def split_comments(lines):
    """Return the comment text of each line, or None when it has none.

    Tracks double-quoted and indented Nix strings so that a '#' inside a
    string is not taken for a comment.
    """
    comments = []
    in_string = False
    in_indented = False
    for line in lines:
        comment = None
        index = 0
        length = len(line)
        while index < length:
            char = line[index]
            if in_indented:
                if line.startswith("''", index):
                    following = line[index + 2 : index + 3]
                    if following in ("'", "$"):
                        index += 3
                        continue
                    in_indented = False
                    index += 2
                    continue
                index += 1
                continue
            if in_string:
                if char == "\\":
                    index += 2
                    continue
                if char == '"':
                    in_string = False
                index += 1
                continue
            if line.startswith("''", index):
                in_indented = True
                index += 2
                continue
            if char == '"':
                in_string = True
            elif char == "#":
                comment = line[index + 1 :]
                break
            index += 1
        comments.append(comment)
    return comments


def code_part(line):
    """Return the part of a line before its comment."""
    comments = split_comments([line])
    if comments[0] is None:
        return line
    return line[: len(line) - len(comments[0]) - 1]


def bracket_delta(text):
    depth = 0
    for char in text:
        if char in "[({":
            depth += 1
        elif char in "])}":
            depth -= 1
    return depth


def has_reason_comment(comments, index):
    for candidate in (index, index - 1):
        if candidate < 0:
            continue
        comment = comments[candidate]
        if comment is None:
            continue
        if any(marker in comment for marker in REASON_MARKERS):
            return True
    return False


def flag_region_end(lines, start):
    """Return the last line index of the value that starts on start."""
    depth = 0
    opened = False
    index = start
    while index < len(lines):
        delta = bracket_delta(code_part(lines[index]))
        if delta > 0:
            opened = True
        depth += delta
        if opened and depth <= 0:
            return index
        index += 1
    return len(lines) - 1


def find_build_option_violations(lines, comments):
    violations = []
    for index, line in enumerate(lines):
        code = code_part(line)
        for label, pattern in BUILD_OPTION_PATTERNS:
            if pattern.search(code) and not has_reason_comment(comments, index):
                violations.append((index, label))
    index = 0
    while index < len(lines):
        code = code_part(lines[index])
        if not FLAG_ATTR.search(code):
            index += 1
            continue
        end = flag_region_end(lines, index)
        for candidate in range(index, end + 1):
            if not K_FLAG.search(code_part(lines[candidate])):
                continue
            if has_reason_comment(comments, candidate):
                continue
            violations.append((candidate, "buildOptions: -k flag"))
        index = end + 1
    return violations


def find_marker_violations(comments):
    violations = []
    for index, comment in enumerate(comments):
        if comment is None:
            continue
        for marker in MARKERS:
            if marker in comment:
                violations.append((index, "markers: %s" % marker))
    return violations


def check_file(path):
    with open(path, encoding="utf-8", errors="replace") as source:
        lines = source.read().splitlines()
    comments = split_comments(lines)
    violations = find_build_option_violations(lines, comments)
    violations.extend(find_marker_violations(comments))
    return sorted(set(violations))


def check(root):
    """Return (files, violations) for root."""
    files = nix_files(root)
    violations = []
    for path in files:
        for index, rule in check_file(path):
            violations.append((path, index + 1, rule))
    return files, violations


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", default=None, help="repository root to inspect")
    parser.add_argument("--list", action="store_true", help="list the inspected files and rules")
    args = parser.parse_args(argv)

    root = args.root or os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
    root = os.path.realpath(root)

    files, violations = check(root)
    if args.list:
        print("check-flake-hygiene: %d files: %s, %s/**/*.nix" % (len(files), FLAKE, NIX_DIR))
        for rule in RULES:
            print("check-flake-hygiene: rule: %s" % rule)
        print("check-flake-hygiene: reason markers: %s" % ", ".join(REASON_MARKERS))

    for path, line, rule in violations:
        print(
            "check-flake-hygiene: %s:%d: %s" % (os.path.relpath(path, root), line, rule),
            file=sys.stderr,
        )
    if violations:
        print(
            "check-flake-hygiene: %d violation(s); move diagnostics to evaluation-time overrides"
            % len(violations),
            file=sys.stderr,
        )
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
