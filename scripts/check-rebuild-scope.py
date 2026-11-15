#!/usr/bin/env python3
"""Check which derivations a representative edit rebuilds.

scripts/rebuild-scope.json lists the derivations of every platform (labels such
as linux/compile) and representative edits (probes) with the labels each probe
must change.  For every probe this script

  1. builds a scratch commit on top of HEAD that applies the edit, using a
     temporary index, so no ref, branch or work tree file changes,
  2. evaluates the drvPath of every listed derivation in the committed tree
     (git+file://<root>?rev=<commit>), and
  3. compares it with the drvPath at HEAD.

A label changes when at least one of its derivations changes.  The check fails
when a label outside the expected set changes or an expected label does not
change.  The work tree is not evaluated: uncommitted edits are ignored, because
the work tree's line-ending conversion alters source files and would hide the
result.  Nothing is built.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import fnmatch
import json
import os
import re
import subprocess
import sys
import tempfile
import urllib.parse
from typing import Dict, List, Optional, Sequence, Set

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MATRIX = os.path.join(REPO_ROOT, "scripts", "rebuild-scope.json")

PLATFORM_NAMES = {
    "linux": "Linux",
    "windows": "Windows",
    "android": "Android",
    "macos": "macOS",
    "ios": "iOS",
}
CATEGORY_NAMES = {
    "compile": "コンパイル",
    "package": "パッケージ化",
    "dependencies": "依存",
    "shell": "開発シェル",
}


def load_matrix(path: str = MATRIX) -> dict:
    with open(path, encoding="utf-8") as handle:
        return json.load(handle)


def label_parts(label: str):
    platform, category = label.split("/")
    return platform, category


def always_labels(matrix: dict) -> Set[str]:
    return {name for name, label in matrix["labels"].items() if label.get("alwaysChanges")}


def expected_changes(matrix: dict, probe: dict) -> Set[str]:
    """Expand a probe's patterns into labels, plus the labels every edit changes."""
    labels = set(matrix["labels"])
    expected = set(always_labels(matrix))
    for pattern in probe["changes"]:
        matched = {label for label in labels if fnmatch.fnmatchcase(label, pattern)}
        if not matched:
            raise ValueError(f"probe {probe['name']}: pattern {pattern} matches no label")
        expected |= matched
    return expected


def validate_matrix(matrix: dict) -> List[str]:
    problems: List[str] = []
    names = set()
    for name, label in matrix["labels"].items():
        if "/" not in name and not label.get("alwaysChanges"):
            problems.append(f"label {name} is neither platform/category nor alwaysChanges")
        if not label.get("attrs"):
            problems.append(f"label {name} lists no flake attribute")
    for probe in matrix["probes"]:
        if probe["name"] in names:
            problems.append(f"duplicate probe {probe['name']}")
        names.add(probe["name"])
        edit = probe["edit"]
        if sum(key in edit for key in ("append", "create", "insert_after")) != 1:
            problems.append(f"probe {probe['name']}: edit needs exactly one of append, create, insert_after")
        if "insert_after" in edit and "text" not in edit:
            problems.append(f"probe {probe['name']}: insert_after needs text")
        try:
            expected_changes(matrix, probe)
        except ValueError as error:
            problems.append(str(error))
    return problems


def run(arguments: Sequence[str], cwd: str, env: Optional[dict] = None, data: Optional[bytes] = None) -> str:
    completed = subprocess.run(
        list(arguments),
        cwd=cwd,
        env=env,
        input=data,
        capture_output=True,
        check=False,
    )
    if completed.returncode != 0:
        raise RuntimeError(
            f"{' '.join(arguments[:4])} failed ({completed.returncode}): "
            f"{completed.stderr.decode(errors='replace').strip()[-2000:]}"
        )
    return completed.stdout.decode(errors="replace")


def git_show(root: str, rev: str, path: str) -> Optional[bytes]:
    completed = subprocess.run(
        ["git", "show", f"{rev}:{path}"], cwd=root, capture_output=True, check=False
    )
    return completed.stdout if completed.returncode == 0 else None


def apply_edit(content: Optional[bytes], edit: dict, path: str) -> bytes:
    """Return the file content after the probe's edit."""
    if "create" in edit:
        if content is not None:
            raise ValueError(f"{path} exists; a create probe needs a new file")
        return edit["create"].encode()
    if content is None:
        raise ValueError(f"{path} does not exist in HEAD")
    text = content.decode()
    if "append" in edit:
        return (text + edit["append"]).encode()
    pattern = re.compile(edit["insert_after"])
    lines = text.split("\n")
    for index, line in enumerate(lines):
        if pattern.search(line):
            indent = line[: len(line) - len(line.lstrip())]
            lines.insert(index + 1, indent + edit["text"])
            return "\n".join(lines).encode()
    raise ValueError(f"{path}: no line matches {edit['insert_after']!r}")


def scratch_commit(root: str, base: str, edit: dict) -> str:
    """Commit the edit on top of base without touching refs or the work tree."""
    path = edit["path"]
    content = apply_edit(git_show(root, base, path), edit, path)
    with tempfile.TemporaryDirectory(prefix="rebuild-scope-") as directory:
        env = dict(os.environ, GIT_INDEX_FILE=os.path.join(directory, "index"))
        env.update(
            GIT_AUTHOR_NAME="rebuild-scope",
            GIT_AUTHOR_EMAIL="rebuild-scope@localhost",
            GIT_COMMITTER_NAME="rebuild-scope",
            GIT_COMMITTER_EMAIL="rebuild-scope@localhost",
        )
        run(["git", "read-tree", base], root, env)
        blob = run(["git", "hash-object", "-w", "--stdin"], root, env, content).strip()
        run(["git", "update-index", "--add", "--cacheinfo", f"100644,{blob},{path}"], root, env)
        tree = run(["git", "write-tree"], root, env).strip()
        return run(["git", "commit-tree", tree, "-p", base, "-m", f"rebuild-scope probe {path}"], root, env).strip()


def nix_string(value: str) -> str:
    if "${" in value:
        raise ValueError("unexpected ${ in the evaluation request")
    return json.dumps(value)


def drv_paths(root: str, rev: str, matrix: dict) -> Dict[str, List[str]]:
    """Evaluate the drvPath of every derivation of every label at rev."""
    request = {
        name: [attr.split(".") for attr in label["attrs"]] for name, label in matrix["labels"].items()
    }
    url = f"git+file://{urllib.parse.quote(root)}?rev={rev}"
    expression = (
        f"let flake = builtins.getFlake {nix_string(url)};"
        " get = path: builtins.foldl' (set: key: set.${key}) flake path;"
        f" request = builtins.fromJSON {nix_string(json.dumps(request))};"
        " in builtins.mapAttrs (name: paths: map (path: (get path).drvPath) paths) request"
    )
    output = run(["nix", "eval", "--json", "--expr", expression], root)
    return json.loads(output)


def changed_labels(base: Dict[str, List[str]], probe: Dict[str, List[str]]) -> Set[str]:
    return {name for name in base if base[name] != probe[name]}


def check_probe(root: str, base_rev: str, base: Dict[str, List[str]], matrix: dict, probe: dict):
    commit = scratch_commit(root, base_rev, {**probe["edit"], "path": probe["path"]})
    changed = changed_labels(base, drv_paths(root, commit, matrix))
    expected = expected_changes(matrix, probe)
    return probe["name"], changed, expected


def describe(labels: Set[str], skip: Set[str]) -> str:
    """Japanese summary of platform/category labels."""
    by_category: Dict[str, List[str]] = {}
    for label in sorted(labels - skip):
        platform, category = label_parts(label)
        by_category.setdefault(category, []).append(platform)
    parts = []
    for category in CATEGORY_NAMES:
        platforms = by_category.get(category)
        if not platforms:
            continue
        if set(platforms) == set(PLATFORM_NAMES):
            parts.append(f"全プラットフォームの{CATEGORY_NAMES[category]}")
        else:
            order = [name for name in PLATFORM_NAMES if name in platforms]
            parts.append("、".join(f"{PLATFORM_NAMES[name]}の{CATEGORY_NAMES[category]}" for name in order))
    return "、".join(parts) if parts else "なし"


def render_table(matrix: dict) -> str:
    skip = always_labels(matrix)
    lines = ["| 編集の種類 | 代表の編集 | 再構築される派生物 |", "| --- | --- | --- |"]
    for probe in matrix["probes"]:
        changes = expected_changes(matrix, probe)
        lines.append(f"| {probe['description']} | `{probe['path']}` | {describe(changes, skip)} |")
    return "\n".join(lines)


def main(argv: Sequence[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--root", default=REPO_ROOT)
    parser.add_argument("--matrix", default=MATRIX)
    parser.add_argument("--probe", action="append", help="check only this probe (repeatable)")
    parser.add_argument("--jobs", type=int, default=3, help="concurrent evaluations")
    parser.add_argument("--render-table", action="store_true", help="print the Markdown table and exit")
    args = parser.parse_args(argv)

    matrix = load_matrix(args.matrix)
    problems = validate_matrix(matrix)
    if problems:
        for problem in problems:
            print(f"check-rebuild-scope: {problem}", file=sys.stderr)
        return 1
    if args.render_table:
        print(render_table(matrix))
        return 0

    root = os.path.abspath(args.root)
    probes = [probe for probe in matrix["probes"] if not args.probe or probe["name"] in args.probe]
    if not probes:
        print("check-rebuild-scope: no probe selected", file=sys.stderr)
        return 1
    base_rev = run(["git", "rev-parse", "HEAD"], root).strip()
    print(f"check-rebuild-scope: base {base_rev[:12]}, {len(probes)} probe(s)", flush=True)
    base = drv_paths(root, base_rev, matrix)

    failures = 0
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        futures = {pool.submit(check_probe, root, base_rev, base, matrix, probe): probe for probe in probes}
        for future in concurrent.futures.as_completed(futures):
            probe = futures[future]
            try:
                name, changed, expected = future.result()
            except Exception as error:  # report each probe, then fail
                failures += 1
                print(f"FAIL {probe['name']}: {error}", file=sys.stderr, flush=True)
                continue
            unexpected = sorted(changed - expected)
            missing = sorted(expected - changed)
            if unexpected or missing:
                failures += 1
                print(f"FAIL {name} ({probe['path']})", file=sys.stderr)
                if unexpected:
                    print(f"  rebuilt but not expected: {', '.join(unexpected)}", file=sys.stderr)
                if missing:
                    print(f"  expected but unchanged: {', '.join(missing)}", file=sys.stderr, flush=True)
            else:
                print(f"ok   {name}: {', '.join(sorted(changed - always_labels(matrix))) or 'nothing rebuilt'}", flush=True)
    if failures:
        print(f"check-rebuild-scope: {failures} probe(s) failed", file=sys.stderr)
        return 1
    print("check-rebuild-scope: OK")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
