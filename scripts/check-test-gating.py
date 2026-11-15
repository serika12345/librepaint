#!/usr/bin/env python3
"""Check that tests and benchmarks are reachable only when BUILD_TESTING is on.

The Nix build sources exclude every directory named tests, autotests or
benchmarks (flake.nix, mkLibrepaintBuildSource), and every product build sets
BUILD_TESTING=OFF.  The product CMake configuration therefore must not read
those directories when BUILD_TESTING is off.  The rules, applied to every
CMakeLists.txt and *.cmake file outside cmake/:

  gate     add_subdirectory() of a path with a tests, autotests or benchmarks
           component sits in an if() branch whose condition requires
           BUILD_TESTING.
  source   no other command names a file below a tests, autotests or
           benchmarks directory unless the command sits in such a branch.
  macro    kis_add_test*(), krita_add_*test*() and KRITA_ADD_BENCHMARK() are
           called only below a test directory or in such a branch.

The files below a test directory are exempt: they are reached only through a
gated add_subdirectory().

--configure builds the Linux source that the Nix build uses (the flake's
legacyPackages.x86_64-linux.buildSources.linux output) and runs the CMake configuration with
BUILD_TESTING=OFF on it, which is the end-to-end proof of the rules.
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
from typing import Iterator, List, Optional, Tuple

TEST_COMPONENTS = ("tests", "autotests", "benchmarks")
TEST_COMMAND = re.compile(
    r"^(kis_add_tests?(_maybe_broken)?|krita_add_(broken_)?unit_tests?|"
    r"krita_add_benchmark|ecm_add_tests?|add_test)$"
)
TEST_PATH = re.compile(r"(^|[/\s\"'(;])(tests|autotests|benchmarks)(/|$)")

Command = Tuple[int, str, str]


def strip_comments(text: str) -> str:
    """Return text with CMake comments blanked, keeping line numbers."""
    result: List[str] = []
    index = 0
    length = len(text)
    in_quote = False
    while index < length:
        char = text[index]
        if in_quote:
            result.append(char)
            if char == "\\" and index + 1 < length:
                result.append(text[index + 1])
                index += 2
                continue
            if char == '"':
                in_quote = False
            index += 1
            continue
        if text.startswith("#[[", index):
            end = text.find("]]", index + 3)
            end = length if end < 0 else end + 2
            result.append("".join("\n" if c == "\n" else " " for c in text[index:end]))
            index = end
            continue
        if char == "#":
            end = text.find("\n", index)
            end = length if end < 0 else end
            result.append(" " * (end - index))
            index = end
            continue
        if char == '"':
            in_quote = True
        result.append(char)
        index += 1
    return "".join(result)


def commands(text: str) -> Iterator[Command]:
    """Yield (line, lowercase command name, argument text) for each command."""
    text = strip_comments(text)
    pattern = re.compile(r"([A-Za-z_][A-Za-z0-9_]*)\s*\(")
    position = 0
    while True:
        match = pattern.search(text, position)
        if match is None:
            return
        depth = 1
        index = match.end()
        in_quote = False
        while index < len(text) and depth > 0:
            char = text[index]
            if in_quote:
                if char == "\\":
                    index += 1
                elif char == '"':
                    in_quote = False
            elif char == '"':
                in_quote = True
            elif char == "(":
                depth += 1
            elif char == ")":
                depth -= 1
            index += 1
        arguments = text[match.end() : index - 1]
        line = text.count("\n", 0, match.start()) + 1
        yield line, match.group(1).lower(), arguments
        position = index


def requires_build_testing(condition: str) -> bool:
    """True when a true condition implies BUILD_TESTING is on."""
    words = re.findall(r"[A-Za-z_0-9${}]+|\(|\)", condition)
    if any(word.upper() == "OR" for word in words):
        return False
    for index, word in enumerate(words):
        if word == "BUILD_TESTING":
            if index > 0 and words[index - 1].upper() == "NOT":
                return False
            return True
    return False


def is_test_path(path: str) -> bool:
    return any(part in TEST_COMPONENTS for part in path.replace("\\", "/").split("/"))


def first_argument(arguments: str) -> str:
    words = arguments.split()
    return words[0].strip('"') if words else ""


def check_file(relative_path: str, text: str) -> List[Tuple[int, str, str]]:
    """Return (line, rule, message) violations of one CMake file."""
    violations: List[Tuple[int, str, str]] = []
    # One frame per open if(): whether the current branch requires BUILD_TESTING.
    frames: List[bool] = []
    for line, name, arguments in commands(text):
        if name == "if":
            frames.append(requires_build_testing(arguments))
            continue
        if name == "elseif":
            if frames:
                frames[-1] = requires_build_testing(arguments)
            continue
        if name == "else":
            if frames:
                frames[-1] = False
            continue
        if name == "endif":
            if frames:
                frames.pop()
            continue
        gated = any(frames)
        if name == "add_subdirectory":
            target = first_argument(arguments)
            if is_test_path(target) and not gated:
                violations.append(
                    (line, "gate", f"add_subdirectory({target}) is not inside if(BUILD_TESTING)")
                )
            continue
        if TEST_COMMAND.match(name):
            if not gated:
                violations.append(
                    (line, "macro", f"{name}() outside a test directory and outside if(BUILD_TESTING)")
                )
            continue
        if not gated:
            for offset, argument_line in enumerate(arguments.split("\n")):
                if TEST_PATH.search(argument_line):
                    violations.append(
                        (
                            line + offset,
                            "source",
                            f"{name}() reads a test path outside if(BUILD_TESTING): {argument_line.strip()}",
                        )
                    )
    return violations


def cmake_files(root: str) -> Iterator[str]:
    """Yield tracked-style relative paths of non-test CMake files."""
    for base, directories, names in os.walk(root):
        relative_base = os.path.relpath(base, root)
        directories[:] = sorted(
            name
            for name in directories
            if name not in (".git", "build", ".cache", "logs", "build-ios", "nix", "docs")
        )
        parts = [] if relative_base == "." else relative_base.split(os.sep)
        if parts and (parts[0] == "cmake" or is_test_path("/".join(parts))):
            continue
        for name in sorted(names):
            if name == "CMakeLists.txt" or name.endswith(".cmake") or name.endswith(".cmake.in"):
                yield os.path.normpath(os.path.join(relative_base, name))


def check(root: str) -> List[Tuple[str, int, str, str]]:
    result: List[Tuple[str, int, str, str]] = []
    for relative_path in cmake_files(root):
        with open(os.path.join(root, relative_path), encoding="utf-8", errors="replace") as handle:
            text = handle.read()
        for line, rule, message in check_file(relative_path, text):
            result.append((relative_path, line, rule, message))
    return result


def run(command: List[str], **kwargs) -> subprocess.CompletedProcess:
    return subprocess.run(command, check=False, text=True, **kwargs)


def configure(root: str, build_directory: Optional[str]) -> int:
    """Configure the flake's Linux build source with BUILD_TESTING=OFF."""
    evaluated = run(
        ["nix", "eval", "--raw", f"git+file://{root}#legacyPackages.x86_64-linux.buildSources.linux.outPath"],
        capture_output=True,
    )
    if evaluated.returncode != 0:
        sys.stderr.write(evaluated.stderr)
        return 1
    source = evaluated.stdout.strip()
    build_directory = build_directory or os.path.join(
        os.environ.get("LIBREPAINT_BUILD_ROOT", os.path.join(root, "build")), "test-gating-configure"
    )
    os.makedirs(build_directory, exist_ok=True)
    flags = [
        "-DALLOW_UNSTABLE=QT6",
        "-DBUILD_KRITA_QT_DESIGNER_PLUGINS=OFF",
        "-DBUILD_TESTING=OFF",
        "-DBUILD_WITH_QT6=ON",
        "-DENABLE_UPDATERS=OFF",
    ]
    print(f"configure: {source} -> {build_directory}")
    completed = run(["cmake", "-G", "Ninja", "-S", source, "-B", build_directory, *flags])
    return completed.returncode


def main(argv: List[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--root", default=os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    parser.add_argument("--configure", action="store_true", help="also configure the Nix Linux source")
    parser.add_argument("--build-directory", default=None)
    args = parser.parse_args(argv)

    violations = check(args.root)
    for path, line, rule, message in violations:
        print(f"{path}:{line}: [{rule}] {message}", file=sys.stderr)
    if violations:
        print(f"check-test-gating: {len(violations)} violation(s)", file=sys.stderr)
        return 1
    print("check-test-gating: OK")
    if args.configure:
        return configure(os.path.abspath(args.root), args.build_directory)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
