#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

"""Check the Nix dependency policy without building anything.

The check reads the derivation graph of a flake attribute, asks
cache.nixos.org (or a snapshot) which of the needed outputs exist, classifies
the derivations that still have to be built locally, and applies the version
rules of nix/dependency-policy.json. Only main calls nix and the network; the
rest of the module works on plain dictionaries.
"""

from __future__ import annotations

import argparse
from collections import defaultdict
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
import json
from pathlib import Path
import re
import subprocess
import sys
import time
import urllib.error
import urllib.request
from typing import Any, Callable, Iterable, Mapping, Sequence

ROOT = Path(__file__).resolve().parents[2]
POLICY_PATH = ROOT / "nix" / "dependency-policy.json"

PLATFORMS = ("linux", "macos", "windows", "android", "ios")
CACHE_PREFIX = "https://cache.nixos.org"
CACHE_WORKERS = 16
CACHE_TIMEOUT = 30
RETRY_DELAYS = (1, 2, 4)
HASH_PREFIX = re.compile(r"^[0-9a-z]{32}-")
VERSION_PATTERN = re.compile(r"^[0-9]+(?:\.[0-9]+)*")
TARGET_SUFFIX_PATTERN = re.compile(r"^\^\.\+(.*?)\(\-\|\$\)$")
ASCII_LETTERS = frozenset("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")
PYTHON_NAME_PREFIX = re.compile(r"^python3(?:\.[0-9]+)?-")


class GraphError(Exception):
    """The derivation graph is malformed or incomplete."""


class ConfigError(Exception):
    """The policy or the command line is unusable."""


class CacheQueryError(Exception):
    """The cache could not answer after the retries."""


class CommandError(Exception):
    """A nix command failed."""


# ---------------------------------------------------------------------------
# Names and versions
# ---------------------------------------------------------------------------


def base_name(path: str) -> str:
    """Return the last path component."""

    return path.rsplit("/", 1)[-1]


def drv_basename(path: str) -> str:
    """Return the .drv basename of a store path or derivation key."""

    return base_name(path)


def display_name(drv: str) -> str:
    """Return the derivation name without the hash prefix and .drv."""

    name = drv[: -len(".drv")] if drv.endswith(".drv") else drv
    return HASH_PREFIX.sub("", name, count=1)


def matching_name(drv: str) -> str:
    """Return the name used for classification, allowances and rules."""

    return display_name(drv).lower().replace("_", "-")


def parse_drv_name(name: str) -> tuple[str, str]:
    """Split a name like builtins.parseDrvName does.

    The split happens at the first dash whose next character is not an ASCII
    letter.
    """

    for index, char in enumerate(name):
        if char != "-":
            continue
        following = name[index + 1] if index + 1 < len(name) else ""
        if not following or following not in ASCII_LETTERS:
            return name[:index], name[index + 1 :]
    return name, ""


def version_tuple(version: str | None) -> tuple[int, ...] | None:
    """Return the leading dotted number of a version, or None."""

    if not version:
        return None
    match = VERSION_PATTERN.match(version)
    if not match:
        return None
    return tuple(int(part) for part in match.group(0).split("."))


def padded(version: tuple[int, ...], width: int) -> tuple[int, ...]:
    return version + (0,) * (width - len(version))


def compare_versions(left: tuple[int, ...], right: tuple[int, ...]) -> int:
    width = max(len(left), len(right))
    left_padded = padded(left, width)
    right_padded = padded(right, width)
    if left_padded < right_padded:
        return -1
    if left_padded > right_padded:
        return 1
    return 0


def major_minor(version: tuple[int, ...]) -> tuple[int, int]:
    first = version[0] if len(version) > 0 else 0
    second = version[1] if len(version) > 1 else 0
    return first, second


def target_suffixes(patterns: Sequence[str]) -> list[str]:
    """Turn target patterns into the literal suffixes their pname part has."""

    suffixes = []
    for pattern in patterns:
        match = TARGET_SUFFIX_PATTERN.match(pattern)
        if match:
            suffixes.append(match.group(1))
    return suffixes


def strip_target_suffix(pname: str, suffixes: Sequence[str]) -> str:
    for suffix in suffixes:
        match = re.search("(?:" + suffix + ")$", pname)
        if match:
            return pname[: match.start()]
    return pname


def env_pname_version(entry: Mapping[str, Any], name: str) -> tuple[str, str]:
    """Read pname and version from structuredAttrs, env, env.__json, or the name.

    Derivations that use __structuredAttrs keep pname and version in
    structuredAttrs. The top-level version of the JSON is the format version of
    the derivation and is never read. A pname taken from the name loses the
    Python package prefix (python3.14-pyqt6 -> pyqt6).
    """

    pname = ""
    version = ""
    structured = entry.get("structuredAttrs")
    if isinstance(structured, dict):
        pname = structured.get("pname") or ""
        version = structured.get("version") or ""
    env = entry.get("env") or {}
    if not pname:
        pname = env.get("pname") or ""
    if not version:
        version = env.get("version") or ""
    if not pname or not version:
        raw = env.get("__json")
        if isinstance(raw, str):
            try:
                embedded = json.loads(raw)
            except ValueError:
                embedded = None
            if isinstance(embedded, dict):
                if not pname:
                    pname = embedded.get("pname") or ""
                if not version:
                    version = embedded.get("version") or ""
    if not pname or not version:
        parsed_pname, parsed_version = parse_drv_name(name)
        if not pname:
            pname = PYTHON_NAME_PREFIX.sub("", parsed_pname, count=1)
        if not version:
            version = parsed_version
    return str(pname or ""), str(version or "")


# ---------------------------------------------------------------------------
# Graph normalization
# ---------------------------------------------------------------------------


def input_map(entry: Mapping[str, Any]) -> dict[str, set[str]]:
    """Return basename -> requested output names for one derivation."""

    raw = entry.get("inputDrvs")
    if raw is None:
        raw = (entry.get("inputs") or {}).get("drvs") or {}
    if not isinstance(raw, dict):
        raise GraphError("派生物の入力が辞書ではありません")
    inputs: dict[str, set[str]] = {}
    for path, value in raw.items():
        if isinstance(value, dict):
            outputs = value.get("outputs") or []
        elif isinstance(value, (list, tuple)):
            outputs = value
        elif value is None:
            outputs = []
        else:
            raise GraphError(f"派生物の入力 {path} の出力の形が不正です")
        inputs[drv_basename(path)] = {str(output) for output in outputs}
    return inputs


def normalize_graph(data: Any) -> dict[str, dict[str, Any]]:
    """Normalize the three accepted graph shapes to a basename-keyed dict."""

    if not isinstance(data, dict):
        raise GraphError("グラフの JSON が辞書ではありません")
    raw_derivations = data.get("derivations", data)
    if not isinstance(raw_derivations, dict):
        raise GraphError("グラフの derivations が辞書ではありません")
    if not raw_derivations:
        raise GraphError("グラフに派生物がありません")

    graph: dict[str, dict[str, Any]] = {}
    for key, entry in raw_derivations.items():
        if not isinstance(entry, dict):
            raise GraphError(f"派生物 {key} が辞書ではありません")
        outputs = entry.get("outputs") or {}
        if not isinstance(outputs, dict):
            raise GraphError(f"派生物 {key} の outputs が辞書ではありません")
        drv = drv_basename(str(key))
        graph[drv] = {
            "outputs": outputs,
            "inputs": input_map(entry),
            "system": str(entry.get("system") or ""),
            "env": entry.get("env") or {},
        }

    missing = []
    for drv, entry in graph.items():
        for source in entry["inputs"]:
            if source not in graph:
                missing.append(f"{drv} の入力 {source}")
    if missing:
        raise GraphError(
            "グラフに入力の参照先がありません: " + "、".join(sorted(missing))
        )
    return graph


def graph_roots(graph: Mapping[str, Mapping[str, Any]]) -> list[str]:
    """Return the derivations that no other derivation in the graph inputs."""

    referenced: set[str] = set()
    for entry in graph.values():
        referenced.update(entry["inputs"])
    return sorted(drv for drv in graph if drv not in referenced)


def is_fixed_output(entry: Mapping[str, Any]) -> bool:
    outputs = entry.get("outputs") or {}
    return any(
        isinstance(output, dict) and ("hash" in output or "hashAlgo" in output)
        for output in outputs.values()
    )


# ---------------------------------------------------------------------------
# Policy helpers
# ---------------------------------------------------------------------------


def compile_patterns(patterns: Iterable[str], label: str) -> list[re.Pattern[str]]:
    compiled = []
    for pattern in patterns:
        try:
            compiled.append(re.compile(pattern))
        except re.error as error:
            raise ConfigError(f"{label} の正規表現 {pattern!r}: {error}") from error
    return compiled


def validate_policy(policy: Mapping[str, Any]) -> None:
    """Compile every regular expression of the policy."""

    targets = policy.get("targets")
    if not isinstance(targets, dict):
        raise ConfigError("policy の targets が辞書ではありません")
    for name, target in targets.items():
        compile_patterns(target.get("target_patterns") or [], f"targets.{name}")
        if not isinstance(target.get("attrs") or [], list):
            raise ConfigError(f"targets.{name}.attrs が配列ではありません")
    compile_patterns(policy.get("own_patterns") or [], "own_patterns")
    compile_patterns(policy.get("small_patterns") or [], "small_patterns")
    for name, entries in (policy.get("host_local_allow") or {}).items():
        for entry in entries:
            compile_patterns([entry.get("match", "")], f"host_local_allow.{name}")
    rules = policy.get("version_rules") or {}
    for group in rules.get("same_version_groups") or []:
        compile_patterns([group.get("pname_regex", "")], "same_version_groups")
    for rule in rules.get("min_versions") or []:
        if not isinstance(rule.get("require") or [], list):
            raise ConfigError("min_versions.require が配列ではありません")
    for rule in rules.get("pairs") or []:
        if not isinstance(rule.get("allowed") or {}, dict):
            raise ConfigError("pairs.allowed が辞書ではありません")


@dataclass(frozen=True)
class DerivationInfo:
    drv: str
    name: str
    matched: str
    target_name: str
    pname: str
    version: str
    version_tuple: tuple[int, ...] | None
    cls: str


class PlatformView:
    """Derivation names, pnames and classes for one platform."""

    def __init__(
        self,
        graph: Mapping[str, Mapping[str, Any]],
        policy: Mapping[str, Any],
        platform: str,
    ) -> None:
        targets = policy.get("targets") or {}
        if platform not in targets:
            raise ConfigError(f"policy にプラットフォーム {platform} がありません")
        self.graph = graph
        self.targets = compile_patterns(
            targets[platform].get("target_patterns") or [],
            f"targets.{platform}.target_patterns",
        )
        self.suffixes = target_suffixes(
            targets[platform].get("target_patterns") or []
        )
        self._cache: dict[str, DerivationInfo] = {}

    def matches_target(self, name: str) -> bool:
        return any(pattern.search(name) for pattern in self.targets)

    def info(self, drv: str) -> DerivationInfo:
        cached = self._cache.get(drv)
        if cached is not None:
            return cached
        entry = self.graph.get(drv)
        if entry is None:
            raise GraphError(f"グラフに派生物 {drv} がありません")
        name = display_name(drv)
        matched = matching_name(drv)
        pname, version = env_pname_version(entry, name)
        pname = strip_target_suffix(pname, self.suffixes)
        pname = pname.lower().replace("_", "-")
        info = DerivationInfo(
            drv=drv,
            name=name,
            matched=matched,
            target_name=name.lower(),
            pname=pname,
            version=version,
            version_tuple=version_tuple(version),
            cls="target"
            if self.matches_target(name.lower())
            else "host",
        )
        self._cache[drv] = info
        return info


# ---------------------------------------------------------------------------
# Cache lookup
# ---------------------------------------------------------------------------


def snapshot_lookup(snapshot: Mapping[str, Any]) -> Callable[[str], bool]:
    def lookup(path: str) -> bool:
        return bool(snapshot.get(base_name(path)[:32], False))

    return lookup


def network_lookup(path: str) -> bool:
    digest = base_name(path)[:32]
    url = f"{CACHE_PREFIX}/{digest}.narinfo"
    request = urllib.request.Request(url, method="HEAD")
    try:
        with urllib.request.urlopen(request, timeout=CACHE_TIMEOUT) as response:
            if response.status == 200:
                return True
            if response.status == 404:
                return False
            raise CacheQueryError(f"{url}: HTTP {response.status}")
    except urllib.error.HTTPError as error:
        if error.code == 200:
            return True
        if error.code == 404:
            return False
        raise CacheQueryError(f"{url}: HTTP {error.code}") from error
    except urllib.error.URLError as error:
        raise CacheQueryError(f"{url}: {error.reason}") from error
    except TimeoutError as error:
        raise CacheQueryError(f"{url}: タイムアウト") from error


def make_query(
    lookup: Callable[[str], bool],
    sleep: Callable[[float], None],
    memo: dict[str, bool],
    snapshot: dict[str, bool],
) -> Callable[[str], bool]:
    """Wrap a cache lookup with memoization and the retry policy."""

    def query(path: str) -> bool:
        digest = base_name(path)[:32]
        if digest in memo:
            return memo[digest]
        ok = False
        last_error: Exception | None = None
        for attempt in range(len(RETRY_DELAYS) + 1):
            try:
                ok = bool(lookup(path))
                last_error = None
                break
            except Exception as error:  # noqa: BLE001 - anything means "unknown"
                last_error = error
                if attempt == len(RETRY_DELAYS):
                    break
                sleep(RETRY_DELAYS[attempt])
        if last_error is not None:
            raise CacheQueryError(f"{base_name(path)}: {last_error}") from last_error
        memo[digest] = ok
        snapshot[digest] = ok
        return ok

    return query


def requested_outputs(
    graph: Mapping[str, Mapping[str, Any]]
) -> dict[str, set[str]]:
    requested: dict[str, set[str]] = defaultdict(set)
    for entry in graph.values():
        for source, outputs in entry["inputs"].items():
            requested[source].update(outputs)
    return requested


def walk_graph(
    graph: Mapping[str, Mapping[str, Any]],
    roots: Sequence[str],
    query: Callable[[str], bool],
) -> dict[str, Any]:
    """Walk from the roots and record what must be built locally."""

    requested = requested_outputs(graph)
    root_set = set(roots)
    seen = set(roots)
    frontier = list(dict.fromkeys(roots))
    local: list[str] = []
    cached: list[str] = []
    unknown: list[str] = []
    sources = 0

    while frontier:
        plans: dict[str, set[str]] = {}
        paths: list[str] = []
        for drv in frontier:
            entry = graph[drv]
            if is_fixed_output(entry):
                sources += 1
                continue
            names: set[str] = set()
            if drv in root_set:
                names.update(entry["outputs"])
            names.update(requested.get(drv, set()))
            plans[drv] = names
            for name in sorted(names):
                output = entry["outputs"].get(name)
                if isinstance(output, dict) and output.get("path"):
                    paths.append(str(output["path"]))

        results: dict[str, bool] = {}
        if paths:
            with ThreadPoolExecutor(max_workers=CACHE_WORKERS) as pool:
                values = list(pool.map(query, paths))
            results = dict(zip(paths, values))

        following: list[str] = []
        for drv, names in plans.items():
            entry = graph[drv]
            outputs = entry["outputs"]
            usable: list[str] = []
            missing_path = False
            for name in names:
                output = outputs.get(name)
                if isinstance(output, dict) and output.get("path"):
                    usable.append(str(output["path"]))
                else:
                    missing_path = True
            if missing_path:
                unknown.append(drv)
                local.append(drv)
            elif not usable:
                local.append(drv)
            elif all(results.get(path, False) for path in usable):
                cached.append(drv)
                continue
            else:
                local.append(drv)
            for source in entry["inputs"]:
                if source not in seen:
                    seen.add(source)
                    following.append(source)
        frontier = following

    return {
        "local": local,
        "cached": cached,
        "unknown": unknown,
        "sources": sources,
    }


# ---------------------------------------------------------------------------
# Classification and allowances
# ---------------------------------------------------------------------------


def classify(
    view: PlatformView,
    policy: Mapping[str, Any],
    platform: str,
    local: Sequence[str],
) -> dict[str, Any]:
    own_patterns = compile_patterns(policy.get("own_patterns") or [], "own_patterns")
    small_patterns = compile_patterns(
        policy.get("small_patterns") or [], "small_patterns"
    )
    allow_entries = list(
        (policy.get("host_local_allow") or {}).get(platform) or []
    )
    allow_patterns = [
        re.compile(entry.get("match", "")) for entry in allow_entries
    ]
    used = [0] * len(allow_entries)

    counts = {"own": 0, "small": 0, "target": 0, "host": 0}
    violations: list[dict[str, str]] = []
    warnings: list[dict[str, str]] = []

    for drv in sorted(local):
        info = view.info(drv)
        if any(pattern.search(info.matched) for pattern in own_patterns):
            bucket = "own"
        elif any(pattern.search(info.matched) for pattern in small_patterns):
            bucket = "small"
        elif view.matches_target(info.target_name):
            bucket = "target"
        else:
            bucket = "host"
        counts[bucket] += 1
        if bucket != "host":
            continue

        matched = [
            index
            for index, pattern in enumerate(allow_patterns)
            if pattern.search(info.matched)
        ]
        if not matched:
            violations.append(
                {
                    "name": info.name,
                    "drv": drv,
                    "reason": "host_local_allow に該当する項目がありません",
                }
            )
            continue
        for index in matched:
            used[index] += 1
        if all(bool(allow_entries[index].get("provisional")) for index in matched):
            reasons = "、".join(
                str(allow_entries[index].get("reason", "")) for index in matched
            )
            warnings.append(
                {
                    "name": info.name,
                    "drv": drv,
                    "reason": "provisional の許可だけに該当: " + reasons,
                }
            )

    unused = [
        {
            "match": entry.get("match", ""),
            "reason": entry.get("reason", ""),
        }
        for index, entry in enumerate(allow_entries)
        if used[index] == 0
    ]
    return {
        "counts": counts,
        "violations": violations,
        "warnings": warnings,
        "unused": unused,
    }


# ---------------------------------------------------------------------------
# Version rules
# ---------------------------------------------------------------------------


def depth_map(
    graph: Mapping[str, Mapping[str, Any]], start: str, limit: int = 4
) -> dict[str, int]:
    """Breadth-first input depths from start, up to limit edges."""

    depths = {start: 0}
    frontier = [start]
    depth = 0
    while frontier and depth < limit:
        depth += 1
        following = []
        for drv in frontier:
            for source in graph[drv]["inputs"]:
                if source not in depths:
                    depths[source] = depth
                    following.append(source)
        frontier = following
    return depths


def at_minimum_depth(
    depths: Mapping[str, int],
    keep: Callable[[str], bool],
) -> list[str]:
    found = {drv: depth for drv, depth in depths.items() if keep(drv)}
    if not found:
        return []
    minimum = min(found.values())
    return sorted(drv for drv, depth in found.items() if depth == minimum)


def version_rules(
    view: PlatformView,
    policy: Mapping[str, Any],
    inspected: Sequence[str],
) -> tuple[list[dict[str, str]], list[dict[str, str]]]:
    graph = view.graph
    rules = policy.get("version_rules") or {}
    infos = {drv: view.info(drv) for drv in inspected}
    items = [infos[drv] for drv in inspected]
    violations: list[dict[str, str]] = []
    warnings: list[dict[str, str]] = []

    def violation(rule: str, message: str) -> None:
        violations.append({"rule": rule, "message": message})

    def warn(rule: str, message: str) -> None:
        warnings.append({"rule": rule, "message": message})

    def describe(selected: Iterable[DerivationInfo]) -> str:
        return "、".join(f"{info.name} {info.version}" for info in selected)

    # 1. same_version_groups
    for group in rules.get("same_version_groups") or []:
        name = str(group.get("name", "same_version_groups"))
        pattern = re.compile(group.get("pname_regex", ""))
        prefix = str(group.get("version_prefix", ""))
        by_class: dict[str, list[DerivationInfo]] = {"host": [], "target": []}
        for info in items:
            if not pattern.search(info.pname):
                continue
            if info.version_tuple is None:
                warn(name, f"{info.name} の版 {info.version!r} を判定できません")
                continue
            if info.version.startswith(prefix):
                by_class[info.cls].append(info)
        for cls, selected in by_class.items():
            if len(selected) < 2:
                continue
            width = max(len(info.version_tuple or ()) for info in selected)
            keys = {
                padded(info.version_tuple or (), width) for info in selected
            }
            if len(keys) > 1:
                violation(name, f"{cls}: {describe(selected)} の版がそろっていません")

    # 2. cross_qt_minor_match
    if rules.get("cross_qt_minor_match"):
        qtbases = [
            info
            for info in items
            if info.pname == "qtbase"
            and info.version
            and info.version.startswith("6.")
            and info.version_tuple is not None
        ]
        minors = {major_minor(info.version_tuple) for info in qtbases}  # type: ignore[arg-type]
        if len(minors) > 1:
            formatted = "、".join(
                f"{major}.{minor}" for major, minor in sorted(minors)
            )
            violation(
                "cross_qt_minor_match",
                f"qtbase の major.minor が複数あります: {formatted}（{describe(qtbases)}）",
            )

    # 3. pyqt_matches_qt
    if rules.get("pyqt_matches_qt"):
        for info in items:
            if info.pname != "pyqt6":
                continue
            if info.version_tuple is None:
                warn("pyqt_matches_qt", f"{info.name} の版を判定できません")
                continue
            depths = depth_map(graph, info.drv)
            wanted = major_minor(info.version_tuple)

            def is_qtbase(drv: str, cls: str | None = None) -> bool:
                other = infos.get(drv) or view.info(drv)
                if other.pname != "qtbase":
                    return False
                if not other.version or not other.version.startswith("6."):
                    return False
                return cls is None or other.cls == cls

            candidates = at_minimum_depth(
                depths, lambda drv: is_qtbase(drv, info.cls)
            )
            if not candidates:
                candidates = at_minimum_depth(depths, is_qtbase)
                if not candidates:
                    warn(
                        "pyqt_matches_qt",
                        f"{info.name} の入力に qtbase 6 が見つかりません",
                    )
                    continue
            for drv in candidates:
                other = infos.get(drv) or view.info(drv)
                if other.version_tuple is None:
                    warn(
                        "pyqt_matches_qt",
                        f"{other.name} の版を判定できません",
                    )
                    continue
                if major_minor(other.version_tuple) != wanted:
                    violation(
                        "pyqt_matches_qt",
                        f"{info.name} {info.version} の入力 "
                        f"{other.name} {other.version} の major.minor が違います",
                    )

    # 4. min_versions
    for rule in rules.get("min_versions") or []:
        when = rule.get("when") or {}
        pname = str(when.get("pname", ""))
        prefix = str(when.get("version_prefix", ""))
        triggers = [info for info in items if info.pname == pname]
        relevant = [
            info
            for info in triggers
            if info.version_tuple is not None
            and info.version.startswith(prefix)
        ]
        for info in triggers:
            if info.version_tuple is None:
                warn(
                    "min_versions",
                    f"{info.name} の版 {info.version!r} を判定できません",
                )
        if not relevant:
            continue
        for requirement in rule.get("require") or []:
            required_pname = str(requirement.get("pname", ""))
            minimum = version_tuple(str(requirement.get("min", "")))
            if minimum is None:
                raise ConfigError(f"min_versions の min {requirement.get('min')!r} が不正です")
            present = [info for info in items if info.pname == required_pname]
            if not present:
                violation(
                    "min_versions",
                    f"{required_pname} が検査の対象にありません"
                    f"（{pname} {describe(relevant)} が要求）",
                )
                continue
            for info in present:
                if info.version_tuple is None:
                    warn(
                        "min_versions",
                        f"{info.name} の版 {info.version!r} を判定できません",
                    )
                elif compare_versions(info.version_tuple, minimum) < 0:
                    violation(
                        "min_versions",
                        f"{info.name} {info.version} が "
                        f"{requirement.get('min')} 未満です",
                    )

    # 5. pairs
    for rule in rules.get("pairs") or []:
        name = str(rule.get("name", "pairs"))
        pname = str(rule.get("pname", ""))
        dependency = str(rule.get("depends_on_pname", ""))
        allowed = rule.get("allowed") or {}
        for info in items:
            if info.pname != pname:
                continue
            if info.version_tuple is None:
                warn(name, f"{info.name} の版を判定できません")
                continue
            major, minor = major_minor(info.version_tuple)
            key = f"{major}.{minor}"
            if key not in allowed:
                warn(name, f"{info.name} {info.version} の版 {key} が許可の表にありません")
                continue
            permitted = {str(value) for value in allowed[key]}
            depths = depth_map(graph, info.drv)

            def is_dependency(drv: str) -> bool:
                other = infos.get(drv) or view.info(drv)
                return (
                    other.pname == dependency
                    and other.cls == info.cls
                )

            candidates = at_minimum_depth(depths, is_dependency)
            if not candidates:
                warn(
                    name,
                    f"{info.name} {info.version} の入力に "
                    f"{dependency} が見つかりません",
                )
                continue
            for drv in candidates:
                other = infos.get(drv) or view.info(drv)
                if other.version_tuple is None:
                    warn(name, f"{other.name} の版を判定できません")
                    continue
                other_major = other.version_tuple[0]
                if str(other_major) not in permitted:
                    violation(
                        name,
                        f"{info.name} {info.version} の入力 "
                        f"{other.name} {other.version} の major "
                        f"{other_major} が許可 {sorted(permitted)} にありません",
                    )

    return violations, warnings


# ---------------------------------------------------------------------------
# Platform check
# ---------------------------------------------------------------------------


def error_result(platform: str, message: str) -> dict[str, Any]:
    return {
        "platform": platform,
        "counts": {"own": 0, "small": 0, "target": 0, "host": 0, "cached": 0, "sources": 0},
        "host_violations": [],
        "allow_warnings": [],
        "unused_allow": [],
        "unknown": [],
        "version_violations": [],
        "version_warnings": [],
        "passed": False,
        "exit_code": 2,
        "error": message,
    }


def evaluate(
    graph: Mapping[str, Mapping[str, Any]],
    policy: Mapping[str, Any],
    platform: str,
    roots: Sequence[str],
    cache_lookup: Callable[[str], bool],
    sleep: Callable[[float], None] | None = None,
    snapshot: dict[str, bool] | None = None,
) -> dict[str, Any]:
    if sleep is None:
        sleep = time.sleep
    if snapshot is None:
        snapshot = {}
    missing_roots = [root for root in roots if root not in graph]
    if missing_roots:
        return error_result(
            platform,
            "ルートの派生物がグラフにありません: " + "、".join(missing_roots),
        )
    try:
        view = PlatformView(graph, policy, platform)
    except ConfigError as error:
        return error_result(platform, str(error))

    query = make_query(cache_lookup, sleep, {}, snapshot)
    try:
        walked = walk_graph(graph, roots, query)
    except CacheQueryError as error:
        return error_result(platform, f"キャッシュの照会に失敗しました: {error}")
    except GraphError as error:
        return error_result(platform, str(error))

    classification = classify(view, policy, platform, walked["local"])
    inspected = list(walked["local"]) + list(walked["cached"])
    inspection_set = set(inspected)
    version_violations, version_warnings = version_rules(
        view, policy, inspected
    )

    target_count = sum(1 for drv in inspection_set if view.info(drv).cls == "target")
    target_patterns = (policy.get("targets") or {}).get(platform, {}).get(
        "target_patterns"
    ) or []
    if target_patterns and target_count == 0:
        result = error_result(
            platform,
            "target_patterns があるのに検査の対象に target が 1 件もありません",
        )
        result["counts"].update(classification["counts"])
        result["counts"]["cached"] = len(walked["cached"])
        result["counts"]["sources"] = walked["sources"]
        return result

    host_violations = classification["violations"]
    counts = dict(classification["counts"])
    counts["cached"] = len(walked["cached"])
    counts["sources"] = walked["sources"]
    passed = not host_violations and not version_violations
    return {
        "platform": platform,
        "counts": counts,
        "host_violations": host_violations,
        "allow_warnings": classification["warnings"],
        "unused_allow": classification["unused"],
        "unknown": [
            {"name": view.info(drv).name, "drv": drv}
            for drv in sorted(walked["unknown"])
        ],
        "version_violations": version_violations,
        "version_warnings": version_warnings,
        "passed": passed,
        "exit_code": 0 if passed else 1,
        "error": None,
    }


def check_platform(
    data: Any,
    policy: Mapping[str, Any],
    platform: str,
    roots: Sequence[str],
    cache_lookup: Callable[[str], bool],
    sleep: Callable[[float], None] | None = None,
    snapshot: dict[str, bool] | None = None,
) -> dict[str, Any]:
    """Normalize a raw graph and check it, mapping graph errors to exit 2."""

    try:
        graph = normalize_graph(data)
    except GraphError as error:
        return error_result(platform, str(error))
    return evaluate(
        graph, policy, platform, roots, cache_lookup, sleep=sleep, snapshot=snapshot
    )


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------


def result_lines(result: Mapping[str, Any]) -> list[str]:
    platform = result["platform"]
    lines = [f"=== {platform} ==="]
    if result.get("error"):
        lines.append(f"エラー: {result['error']}")
        lines.append("結果: 検査不能")
        return lines

    counts = result["counts"]
    lines.append(
        "手元構築: {total} 件（own {own}、small {small}、target {target}、host {host}）".format(
            total=counts["own"] + counts["small"] + counts["target"] + counts["host"],
            own=counts["own"],
            small=counts["small"],
            target=counts["target"],
            host=counts["host"],
        )
    )
    lines.append(f"キャッシュ: {counts['cached']} 件")
    lines.append(f"固定出力（ソース）: {counts['sources']} 件")

    def section(title: str, entries: Sequence[str]) -> None:
        if not entries:
            return
        lines.append(f"{title}: {len(entries)} 件")
        for entry in entries:
            lines.append(f"  - {entry}")

    section(
        "host の違反",
        [
            f"{entry['name']} {entry['drv']}（{entry['reason']}）"
            for entry in result["host_violations"]
        ],
    )
    section(
        "許可の警告",
        [
            f"{entry['name']} {entry['drv']}（{entry['reason']}）"
            for entry in result["allow_warnings"]
        ],
    )
    section(
        "未使用の許可",
        [
            f"{entry['match']}（{entry['reason']}）"
            for entry in result["unused_allow"]
        ],
    )
    section(
        "判定不能",
        [f"{entry['name']} {entry['drv']}" for entry in result["unknown"]],
    )
    section(
        "版の違反",
        [
            f"{entry['rule']}: {entry['message']}"
            for entry in result["version_violations"]
        ],
    )
    section(
        "版の警告",
        [
            f"{entry['rule']}: {entry['message']}"
            for entry in result["version_warnings"]
        ],
    )
    if result["passed"]:
        lines.append("結果: 合格")
    else:
        lines.append(
            "結果: 不合格（host の違反 {host} 件、版の違反 {version} 件）".format(
                host=len(result["host_violations"]),
                version=len(result["version_violations"]),
            )
        )
    return lines


# ---------------------------------------------------------------------------
# nix and the command line
# ---------------------------------------------------------------------------


def run_nix(arguments: Sequence[str]) -> str:
    try:
        completed = subprocess.run(
            list(arguments),
            cwd=ROOT,
            capture_output=True,
            text=True,
            check=False,
        )
    except OSError as error:
        raise CommandError(f"{' '.join(arguments)}: {error}") from error
    if completed.returncode != 0:
        detail = (completed.stderr or completed.stdout or "").strip().splitlines()
        tail = detail[-1] if detail else ""
        raise CommandError(
            f"{' '.join(arguments)} が終了コード {completed.returncode} で失敗しました: {tail}"
        )
    return completed.stdout


def fetch_graph(attr: str) -> tuple[str, Any]:
    stdout = run_nix(["nix", "path-info", "--derivation", f".#{attr}"])
    lines = [line.strip() for line in stdout.splitlines() if line.strip()]
    if len(lines) != 1:
        raise CommandError(
            f"nix path-info --derivation .#{attr} が 1 行を返しませんでした"
        )
    drv = drv_basename(lines[0])
    show = run_nix(["nix", "derivation", "show", "-r", f"{lines[0]}^*"])
    try:
        data = json.loads(show)
    except ValueError as error:
        raise CommandError(
            f"nix derivation show の JSON を読めません: {error}"
        ) from error
    return drv, data


def load_json_file(path: Path, label: str) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except OSError as error:
        raise ConfigError(f"{label} を読めません: {path}: {error}") from error
    except ValueError as error:
        raise ConfigError(f"{label} の JSON が不正です: {path}: {error}") from error


def save_json_file(path: Path, data: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(data, ensure_ascii=False, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )


def load_platform(
    platform: str,
    policy: Mapping[str, Any],
    args: argparse.Namespace,
) -> tuple[dict[str, dict[str, Any]], list[str]]:
    targets = policy.get("targets") or {}
    if platform not in targets:
        raise ConfigError(f"policy にプラットフォーム {platform} がありません")
    target = targets[platform]
    attrs = list(target.get("attrs") or [])
    if not attrs:
        raise ConfigError(f"targets.{platform}.attrs が空です")

    merged: dict[str, dict[str, Any]] = {}
    roots: list[str] = []
    graph_dir = getattr(args, "graph_dir", None)
    save_dir = getattr(args, "save_graph", None)
    for index, attr in enumerate(attrs, start=1):
        if graph_dir:
            path = Path(graph_dir) / f"{platform}-{index}.json"
            data = load_json_file(path, "グラフ")
            graph = normalize_graph(data)
            inferred = graph_roots(graph)
            if len(inferred) != 1:
                raise GraphError(
                    f"{path} のルートを 1 件に決められません: "
                    + "、".join(inferred or ["なし"])
                )
            root = inferred[0]
        else:
            root, data = fetch_graph(attr)
            graph = normalize_graph(data)
            if root not in graph:
                raise GraphError(f"{attr} のルート {root} がグラフにありません")
            if save_dir:
                save_json_file(Path(save_dir) / f"{platform}-{index}.json", data)
        for drv, entry in graph.items():
            merged.setdefault(drv, entry)
        if root not in roots:
            roots.append(root)
    return merged, roots


def parse_arguments(argv: Sequence[str] | None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Nix の依存と版を構築せずに検査する"
    )
    parser.add_argument(
        "--platform",
        choices=(*PLATFORMS, "all"),
        default="all",
        help="検査するプラットフォーム（既定 all）",
    )
    parser.add_argument("--graph-dir", help="保存したグラフを読むディレクトリ")
    parser.add_argument("--save-graph", help="取得したグラフを保存するディレクトリ")
    parser.add_argument("--cache-snapshot", help="キャッシュの代わりに使う JSON")
    parser.add_argument(
        "--save-cache-snapshot", help="キャッシュの照会結果を保存する JSON"
    )
    parser.add_argument("--json", dest="json_path", help="同じ内容を書く JSON")
    return parser.parse_args(argv)


def main(
    argv: Sequence[str] | None = None,
    policy_path: Path | None = None,
    sleep: Callable[[float], None] | None = None,
) -> int:
    args = parse_arguments(argv)
    if sleep is None:
        sleep = time.sleep
    policy_path = Path(policy_path) if policy_path is not None else POLICY_PATH
    try:
        policy = load_json_file(policy_path, "policy")
        validate_policy(policy)
    except ConfigError as error:
        print(f"エラー: {error}", file=sys.stderr)
        return 2

    snapshot_table: dict[str, bool] = {}
    if args.cache_snapshot:
        try:
            raw_snapshot = load_json_file(Path(args.cache_snapshot), "キャッシュの記録")
        except ConfigError as error:
            print(f"エラー: {error}", file=sys.stderr)
            return 2
        if not isinstance(raw_snapshot, dict):
            print("エラー: キャッシュの記録が辞書ではありません", file=sys.stderr)
            return 2
        snapshot_table = {
            str(key): bool(value) for key, value in raw_snapshot.items()
        }
        cache_lookup = snapshot_lookup(snapshot_table)
    else:
        cache_lookup = network_lookup

    platforms = list(PLATFORMS) if args.platform == "all" else [args.platform]
    results: dict[str, dict[str, Any]] = {}
    reported_snapshot: dict[str, bool] = {}
    overall = 0

    for platform in platforms:
        try:
            graph, roots = load_platform(platform, policy, args)
        except (ConfigError, GraphError, CommandError) as error:
            result = error_result(platform, str(error))
            results[platform] = result
            overall = 2
            print("\n".join(result_lines(result)))
            continue
        result = evaluate(
            graph,
            policy,
            platform,
            roots,
            cache_lookup,
            sleep=sleep,
            snapshot=reported_snapshot,
        )
        results[platform] = result
        if result["exit_code"] == 2:
            overall = 2
        elif result["exit_code"] == 1 and overall == 0:
            overall = 1
        print("\n".join(result_lines(result)))

    if args.save_cache_snapshot and not args.cache_snapshot:
        save_json_file(Path(args.save_cache_snapshot), reported_snapshot)
    if args.json_path:
        save_json_file(
            Path(args.json_path),
            {"platforms": results, "exit_code": overall},
        )
    return overall


if __name__ == "__main__":
    sys.exit(main())
