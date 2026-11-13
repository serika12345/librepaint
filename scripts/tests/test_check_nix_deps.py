#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

from __future__ import annotations

import contextlib
import importlib.util
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

TESTS_DIR = Path(__file__).resolve().parent
SCRIPT = TESTS_DIR.parent / "platform" / "check-nix-deps.py"
SPEC = importlib.util.spec_from_file_location("check_nix_deps", SCRIPT)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"cannot import {SCRIPT}")
check_nix_deps = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = check_nix_deps
SPEC.loader.exec_module(check_nix_deps)

POLICY_PATH = TESTS_DIR.parents[1] / "nix" / "dependency-policy.json"
FIXTURES = TESTS_DIR / "fixtures" / "nix-deps"
HASH_CHARS = "0123456789abcdfghijklmnpqrsvwxyz"


def digest(index: int) -> str:
    return HASH_CHARS[index] * 32


def drv_path(index: int, name: str) -> str:
    return f"/nix/store/{digest(index)}-{name}.drv"


def drv_key(index: int, name: str) -> str:
    return f"{digest(index)}-{name}.drv"


def output_path(index: int, name: str) -> str:
    return f"/nix/store/{digest(index)}-{name}"


def make_entry(
    index: int,
    name: str,
    inputs: dict[str, list[str]] | None = None,
    outputs: dict[str, dict[str, object]] | None = None,
    env: dict[str, object] | None = None,
    fixed: bool = False,
    absolute: bool = True,
) -> tuple[str, dict[str, object]]:
    if outputs is None:
        outputs = {"out": {"path": output_path(index, name)}}
    if fixed:
        for output in outputs.values():
            output.setdefault("hash", "sha256-test")
            output.pop("path", None)
    entry: dict[str, object] = {
        "outputs": outputs,
        "inputDrvs": inputs or {},
    }
    if env:
        entry["env"] = env
    key = drv_path(index, name) if absolute else drv_key(index, name)
    return key, entry


def make_policy(
    version_rules: dict[str, object] | None = None,
    allow: list[dict[str, object]] | None = None,
    target_patterns: list[str] | None = None,
    own: list[str] | None = None,
    small: list[str] | None = None,
) -> dict[str, object]:
    if allow is None:
        allow = [{"match": ".*", "reason": "test"}]
    return {
        "targets": {
            "test": {
                "attrs": ["packages.test"],
                "target_patterns": list(target_patterns or []),
            }
        },
        "own_patterns": list(own if own is not None else ["^librepaint"]),
        "small_patterns": list(small if small is not None else ["-hook$"]),
        "host_local_allow": {"test": list(allow)},
        "version_rules": version_rules or {},
    }


def cache_hits(*indexes: int):
    digests = {digest(index) for index in indexes}

    def lookup(path: str) -> bool:
        return path.rsplit("/", 1)[-1][:32] in digests

    return lookup


def run_check(
    entries: dict[str, dict[str, object]],
    roots: list[str],
    policy: dict[str, object],
    cached: tuple[int, ...] = (),
    platform: str = "test",
    lookup=None,
    sleep=None,
) -> dict[str, object]:
    data = {"derivations": entries}
    normalized_roots = [root.rsplit("/", 1)[-1] for root in roots]
    if lookup is None:
        lookup = cache_hits(*cached)
    if sleep is None:
        sleep = lambda _delay: None  # noqa: E731
    return check_nix_deps.check_platform(
        data,
        policy,
        platform,
        normalized_roots,
        lookup,
        sleep=sleep,
    )


def rules_of(result: dict[str, object]) -> list[str]:
    return [entry["rule"] for entry in result["version_violations"]]


def names_of(entries: list[dict[str, object]]) -> list[str]:
    return [str(entry["name"]) for entry in entries]


class SameVersionGroupTest(unittest.TestCase):
    def policy(self) -> dict[str, object]:
        return make_policy(
            version_rules={
                "same_version_groups": [
                    {
                        "name": "qt-modules",
                        "pname_regex": "^qt(base|declarative)$",
                        "version_prefix": "6.",
                    }
                ]
            }
        )

    def graph(self, qtbase: str, qtdeclarative: str):
        root_key, root = make_entry(0, "librepaint-1.0")
        qtbase_key, qtbase_entry = make_entry(1, qtbase)
        qtdecl_key, qtdecl_entry = make_entry(2, qtdeclarative)
        root["inputDrvs"] = {qtbase_key: ["out"], qtdecl_key: ["out"]}
        return (
            {
                root_key: root,
                qtbase_key: qtbase_entry,
                qtdecl_key: qtdecl_entry,
            },
            [root_key],
        )

    def test_different_versions_in_one_class(self) -> None:
        entries, roots = self.graph("qtbase-6.12.0", "qtdeclarative-6.11.1")
        result = run_check(entries, roots, self.policy())
        self.assertEqual(result["exit_code"], 1)
        self.assertEqual(rules_of(result), ["qt-modules"])

    def test_same_versions_pass(self) -> None:
        entries, roots = self.graph("qtbase-6.12.0", "qtdeclarative-6.12.0")
        result = run_check(entries, roots, self.policy())
        self.assertEqual(result["exit_code"], 0)
        self.assertEqual(result["version_violations"], [])

    def test_qt5_is_ignored(self) -> None:
        entries, roots = self.graph("qtbase-5.15.2", "qtdeclarative-6.11.1")
        result = run_check(entries, roots, self.policy())
        self.assertEqual(result["exit_code"], 0)
        self.assertEqual(result["version_violations"], [])


class CrossQtMinorTest(unittest.TestCase):
    def policy(self) -> dict[str, object]:
        return make_policy(
            version_rules={"cross_qt_minor_match": True},
            target_patterns=["^.+-x86_64-w64-mingw32(-|$)"],
        )

    def graph(self, target_version: str, host_version: str = "6.12.0"):
        root_key, root = make_entry(0, "librepaint-1.0")
        host_key, host = make_entry(1, f"qtbase-{host_version}")
        target_key, target = make_entry(
            2, f"qtbase-x86_64-w64-mingw32-{target_version}"
        )
        root["inputDrvs"] = {host_key: ["out"], target_key: ["out"]}
        return {
            root_key: root,
            host_key: host,
            target_key: target,
        }, [root_key]

    def test_minor_mismatch(self) -> None:
        entries, roots = self.graph("6.11.3")
        result = run_check(entries, roots, self.policy())
        self.assertEqual(result["exit_code"], 1)
        self.assertEqual(rules_of(result), ["cross_qt_minor_match"])

    def test_same_minor_passes(self) -> None:
        entries, roots = self.graph("6.12.0")
        result = run_check(entries, roots, self.policy())
        self.assertEqual(result["exit_code"], 0)
        self.assertEqual(result["version_violations"], [])

    def test_host_and_target_qtbase_stay_separate(self) -> None:
        entries, roots = self.graph("6.11.3")
        result = run_check(entries, roots, self.policy())
        self.assertEqual(result["counts"]["host"], 1)
        self.assertEqual(result["counts"]["target"], 1)
        self.assertEqual(len(entries), 3)


class PyQtMatchesQtTest(unittest.TestCase):
    def policy(self, target_patterns=None) -> dict[str, object]:
        return make_policy(
            version_rules={"pyqt_matches_qt": True},
            target_patterns=target_patterns,
        )

    def graph(self, pyqt_version: str, qtbases: list[tuple[int, str]], target_patterns=None):
        root_key, root = make_entry(0, "librepaint-1.0")
        pyqt_key, pyqt = make_entry(1, f"pyqt6-{pyqt_version}")
        root["inputDrvs"] = {pyqt_key: ["out"]}
        entries = {root_key: root, pyqt_key: pyqt}
        for index, name in qtbases:
            key, entry = make_entry(index, name)
            entries[key] = entry
            pyqt["inputDrvs"][key] = ["out"]
        return entries, [root_key]

    def test_pyqt_newer_than_qt(self) -> None:
        entries, roots = self.graph("6.11.0", [(2, "qtbase-6.12.0")])
        result = run_check(entries, roots, self.policy())
        self.assertEqual(result["exit_code"], 1)
        self.assertEqual(rules_of(result), ["pyqt_matches_qt"])

    def test_same_minor_passes(self) -> None:
        entries, roots = self.graph("6.12.0", [(2, "qtbase-6.12.0")])
        result = run_check(entries, roots, self.policy())
        self.assertEqual(result["exit_code"], 0)
        self.assertEqual(result["version_violations"], [])

    def test_same_class_qtbase_wins(self) -> None:
        patterns = ["^.+-x86_64-w64-mingw32(-|$)"]
        entries, roots = self.graph(
            "6.12.0",
            [(2, "qtbase-6.12.0"), (3, "qtbase-x86_64-w64-mingw32-6.11.3")],
            target_patterns=patterns,
        )
        result = run_check(entries, roots, self.policy(patterns))
        self.assertEqual(result["exit_code"], 0)
        self.assertEqual(result["version_violations"], [])


class MinVersionsTest(unittest.TestCase):
    def policy(self) -> dict[str, object]:
        return make_policy(
            version_rules={
                "min_versions": [
                    {
                        "when": {"pname": "pyqt6", "version_prefix": "6.12"},
                        "require": [
                            {"pname": "pyqt6-sip", "min": "13.13"},
                            {"pname": "sip", "min": "6.17"},
                            {"pname": "pyqt-builder", "min": "1.20"},
                        ],
                    }
                ]
            }
        )

    def graph(self, sip: str, pyqt_sip: str, builder: str = "1.21.0"):
        root_key, root = make_entry(0, "librepaint-1.0")
        pyqt_key, pyqt = make_entry(1, "pyqt6-6.12.0")
        sip_key, sip_entry = make_entry(2, f"sip-{sip}")
        pyqt_sip_key, pyqt_sip_entry = make_entry(3, f"pyqt6_sip-{pyqt_sip}")
        builder_key, builder_entry = make_entry(4, f"pyqt-builder-{builder}")
        root["inputDrvs"] = {
            pyqt_key: ["out"],
            sip_key: ["out"],
            pyqt_sip_key: ["out"],
            builder_key: ["out"],
        }
        return (
            {
                root_key: root,
                pyqt_key: pyqt,
                sip_key: sip_entry,
                pyqt_sip_key: pyqt_sip_entry,
                builder_key: builder_entry,
            },
            [root_key],
        )

    def test_old_sip(self) -> None:
        entries, roots = self.graph("6.16.1", "13.13.1")
        result = run_check(entries, roots, self.policy())
        self.assertEqual(result["exit_code"], 1)
        self.assertEqual(rules_of(result), ["min_versions"])

    def test_all_minimum_versions_pass(self) -> None:
        entries, roots = self.graph("6.17.3", "13.13.1")
        result = run_check(entries, roots, self.policy())
        self.assertEqual(result["exit_code"], 0)
        self.assertEqual(result["version_violations"], [])

    def test_old_pyqt6_sip(self) -> None:
        entries, roots = self.graph("6.17.3", "13.10.2")
        result = run_check(entries, roots, self.policy())
        self.assertEqual(result["exit_code"], 1)
        self.assertEqual(rules_of(result), ["min_versions"])

    def test_short_version_is_padded(self) -> None:
        entries, roots = self.graph("6.17.3", "13.13")
        result = run_check(entries, roots, self.policy())
        self.assertEqual(result["exit_code"], 0)
        self.assertEqual(result["version_violations"], [])


class PairsTest(unittest.TestCase):
    def policy(self) -> dict[str, object]:
        return make_policy(
            version_rules={
                "pairs": [
                    {
                        "name": "mlt-ffmpeg",
                        "pname": "mlt",
                        "depends_on_pname": "ffmpeg",
                        "allowed": {"7.42": ["8"]},
                    }
                ]
            }
        )

    def graph(self, ffmpeg: str):
        root_key, root = make_entry(0, "librepaint-1.0")
        mlt_key, mlt = make_entry(1, "mlt-7.42.0", {})
        ffmpeg_key, ffmpeg_entry = make_entry(2, f"ffmpeg-{ffmpeg}")
        mlt["inputDrvs"] = {ffmpeg_key: ["out"]}
        root["inputDrvs"] = {mlt_key: ["out"]}
        return {
            root_key: root,
            mlt_key: mlt,
            ffmpeg_key: ffmpeg_entry,
        }, [root_key]

    def test_wrong_major(self) -> None:
        entries, roots = self.graph("9.0.2")
        result = run_check(entries, roots, self.policy())
        self.assertEqual(result["exit_code"], 1)
        self.assertEqual(rules_of(result), ["mlt-ffmpeg"])

    def test_allowed_major_passes(self) -> None:
        entries, roots = self.graph("8.1.3")
        result = run_check(entries, roots, self.policy())
        self.assertEqual(result["exit_code"], 0)
        self.assertEqual(result["version_violations"], [])


class HostAllowTest(unittest.TestCase):
    def graph(self):
        root_key, root = make_entry(0, "librepaint-1.0")
        host_key, host = make_entry(1, "some-lib-1.0")
        root["inputDrvs"] = {host_key: ["out"]}
        return {root_key: root, host_key: host}, [root_key]

    def test_missing_allowance_is_a_violation(self) -> None:
        entries, roots = self.graph()
        result = run_check(entries, roots, make_policy(allow=[]))
        self.assertEqual(result["exit_code"], 1)
        self.assertEqual(names_of(result["host_violations"]), ["some-lib-1.0"])

    def test_allowance_passes(self) -> None:
        entries, roots = self.graph()
        result = run_check(
            entries,
            roots,
            make_policy(
                allow=[{"match": "^some-lib-", "reason": "test"}]
            ),
        )
        self.assertEqual(result["exit_code"], 0)
        self.assertEqual(result["allow_warnings"], [])
        self.assertEqual(result["unused_allow"], [])

    def test_provisional_only_is_a_warning(self) -> None:
        entries, roots = self.graph()
        result = run_check(
            entries,
            roots,
            make_policy(
                allow=[
                    {
                        "match": "^some-lib-",
                        "reason": "test",
                        "provisional": True,
                    }
                ]
            ),
        )
        self.assertEqual(result["exit_code"], 0)
        self.assertEqual(len(result["allow_warnings"]), 1)

    def test_non_provisional_match_removes_the_warning(self) -> None:
        entries, roots = self.graph()
        result = run_check(
            entries,
            roots,
            make_policy(
                allow=[
                    {
                        "match": "^some-",
                        "reason": "test",
                        "provisional": True,
                    },
                    {"match": "-lib-", "reason": "accepted"},
                ]
            ),
        )
        self.assertEqual(result["exit_code"], 0)
        self.assertEqual(result["allow_warnings"], [])

    def test_unused_allowance_is_reported(self) -> None:
        entries, roots = self.graph()
        result = run_check(
            entries,
            roots,
            make_policy(
                allow=[
                    {"match": "^some-lib-", "reason": "test"},
                    {"match": "^never-used-", "reason": "stale"},
                ]
            ),
        )
        self.assertEqual(result["exit_code"], 0)
        self.assertEqual(
            [entry["match"] for entry in result["unused_allow"]],
            ["^never-used-"],
        )


class TargetAndOwnTest(unittest.TestCase):
    def graph(self):
        root_key, root = make_entry(0, "librepaint-1.0")
        own_key, own = make_entry(1, "librepaint-plugin-1.0")
        target_key, target = make_entry(
            2, "qtbase-x86_64-w64-mingw32-6.12.0"
        )
        cross_key, cross = make_entry(
            3, "x86_64-w64-mingw32-gcc-wrapper-14"
        )
        root["inputDrvs"] = {
            own_key: ["out"],
            target_key: ["out"],
            cross_key: ["out"],
        }
        return {
            root_key: root,
            own_key: own,
            target_key: target,
            cross_key: cross,
        }, [root_key]

    def test_target_and_own_need_no_allowance(self) -> None:
        entries, roots = self.graph()
        result = run_check(
            entries,
            roots,
            make_policy(
                allow=[],
                target_patterns=["^.+-x86_64-w64-mingw32(-|$)"],
            ),
        )
        self.assertEqual(result["exit_code"], 1)
        self.assertEqual(
            names_of(result["host_violations"]),
            ["x86_64-w64-mingw32-gcc-wrapper-14"],
        )
        self.assertEqual(result["counts"]["own"], 2)
        self.assertEqual(result["counts"]["small"], 0)
        self.assertEqual(result["counts"]["target"], 1)
        self.assertEqual(result["counts"]["host"], 1)


class CacheWalkTest(unittest.TestCase):
    def graph(self):
        root_key, root = make_entry(0, "librepaint-1.0")
        glibc_key, glibc = make_entry(1, "glibc-2.40")
        hidden_key, hidden = make_entry(2, "hidden-1.0")
        host_key, host = make_entry(3, "hosttool-1.0")
        deeper_key, deeper = make_entry(4, "deeper-1.0")
        small_key, small = make_entry(5, "small-hook")
        small_child_key, small_child = make_entry(6, "small-child-1.0")
        own_key, own = make_entry(7, "librepaint-child-1.0")
        own_child_key, own_child = make_entry(8, "own-child-1.0")
        target_key, target = make_entry(
            9, "qtbase-x86_64-w64-mingw32-6.12.0"
        )
        target_tool_key, target_tool = make_entry(10, "target-host-tool-1.0")
        source_key, source = make_entry(11, "source-1.0", fixed=True)
        source_child_key, source_child = make_entry(12, "hidden-source-1.0")

        glibc["inputDrvs"] = {hidden_key: ["out"]}
        host["inputDrvs"] = {deeper_key: ["out"]}
        small["inputDrvs"] = {small_child_key: ["out"]}
        own["inputDrvs"] = {own_child_key: ["out"]}
        target["inputDrvs"] = {target_tool_key: ["out"]}
        source["inputDrvs"] = {source_child_key: ["out"]}
        root["inputDrvs"] = {
            glibc_key: ["out"],
            host_key: ["out"],
            small_key: ["out"],
            own_key: ["out"],
            target_key: ["out"],
            source_key: ["out"],
        }
        entries = {
            key: entry
            for key, entry in (
                (root_key, root),
                (glibc_key, glibc),
                (hidden_key, hidden),
                (host_key, host),
                (deeper_key, deeper),
                (small_key, small),
                (small_child_key, small_child),
                (own_key, own),
                (own_child_key, own_child),
                (target_key, target),
                (target_tool_key, target_tool),
                (source_key, source),
                (source_child_key, source_child),
            )
        }
        return entries, [root_key]

    def test_cached_inputs_are_not_visited(self) -> None:
        entries, roots = self.graph()
        result = run_check(
            entries,
            roots,
            make_policy(
                allow=[],
                target_patterns=["^.+-x86_64-w64-mingw32(-|$)"],
            ),
            cached=(1,),
        )
        violations = names_of(result["host_violations"])
        self.assertNotIn("glibc-2.40", violations)
        self.assertNotIn("hidden-1.0", violations)
        self.assertNotIn("source-1.0", violations)
        self.assertNotIn("hidden-source-1.0", violations)
        self.assertIn("hosttool-1.0", violations)
        self.assertIn("deeper-1.0", violations)
        self.assertIn("small-child-1.0", violations)
        self.assertIn("own-child-1.0", violations)
        self.assertIn("target-host-tool-1.0", violations)
        self.assertEqual(result["counts"]["cached"], 1)
        self.assertEqual(result["counts"]["sources"], 1)
        self.assertEqual(result["counts"]["own"], 2)
        self.assertEqual(result["counts"]["small"], 1)
        self.assertEqual(result["counts"]["target"], 1)
        self.assertEqual(result["counts"]["host"], 5)


class UnusedOutputTest(unittest.TestCase):
    def test_unused_output_does_not_prevent_a_cache_hit(self) -> None:
        root_key, root = make_entry(0, "librepaint-1.0")
        zlib_key, zlib = make_entry(
            1,
            "zlib-1.2",
            outputs={
                "out": {"path": output_path(1, "zlib-1.2")},
                "debug": {},
            },
        )
        missing_key, missing = make_entry(
            2, "devmissing-1.0", outputs={"out": {}}
        )
        root["inputDrvs"] = {zlib_key: ["out"], missing_key: ["out"]}
        result = run_check(
            {root_key: root, zlib_key: zlib, missing_key: missing},
            [root_key],
            make_policy(),
            cached=(1,),
        )
        self.assertEqual(result["counts"]["cached"], 1)
        self.assertEqual(
            names_of(result["unknown"]), ["devmissing-1.0"]
        )
        self.assertEqual(result["counts"]["host"], 1)
        self.assertEqual(result["counts"]["own"], 1)


class GraphShapeTest(unittest.TestCase):
    def result_for(self, filename: str) -> dict[str, object]:
        data = json.loads((FIXTURES / filename).read_text(encoding="utf-8"))
        return check_nix_deps.check_platform(
            data,
            make_policy(),
            "test",
            [drv_key(0, "librepaint-1.0")],
            cache_hits(1),
            sleep=lambda _delay: None,
        )

    def test_three_shapes_agree(self) -> None:
        results = [
            self.result_for("graph-wrapped.json"),
            self.result_for("graph-plain.json"),
            self.result_for("graph-inputs-drvs.json"),
        ]
        for result in results:
            self.assertEqual(result["exit_code"], 0)
            self.assertEqual(result["counts"]["own"], 1)
            self.assertEqual(result["counts"]["cached"], 1)
            self.assertEqual(result["host_violations"], [])
        self.assertEqual(results[0]["counts"], results[1]["counts"])
        self.assertEqual(results[1]["counts"], results[2]["counts"])

    def test_missing_input_reference(self) -> None:
        data = json.loads(
            (FIXTURES / "graph-missing-input.json").read_text(encoding="utf-8")
        )
        result = check_nix_deps.check_platform(
            data,
            make_policy(),
            "test",
            [drv_key(0, "librepaint-1.0")],
            cache_hits(),
            sleep=lambda _delay: None,
        )
        self.assertEqual(result["exit_code"], 2)
        self.assertIn("入力", str(result["error"]))


class CacheFailureTest(unittest.TestCase):
    def test_retries_then_exit_2(self) -> None:
        root_key, root = make_entry(0, "librepaint-1.0")
        zlib_key, zlib = make_entry(1, "zlib-1.2")
        root["inputDrvs"] = {zlib_key: ["out"]}
        calls = []

        def failing(path: str) -> bool:
            calls.append(path)
            raise ConnectionError("no network")

        sleeps = []
        result = run_check(
            {root_key: root, zlib_key: zlib},
            [root_key],
            make_policy(),
            lookup=failing,
            sleep=sleeps.append,
        )
        self.assertEqual(result["exit_code"], 2)
        self.assertEqual(calls and len(calls), 4)
        self.assertEqual(sleeps, [1, 2, 4])
        self.assertEqual(result["counts"]["own"], 0)
        self.assertEqual(result["counts"]["host"], 0)


def write_policy(path: Path) -> None:
    policy = {
        "targets": {
            platform: {"attrs": ["packages.test"], "target_patterns": []}
            for platform in ("linux", "macos", "windows", "android", "ios")
        },
        "own_patterns": ["^librepaint"],
        "small_patterns": [],
        "host_local_allow": {
            platform: [{"match": ".*", "reason": "test"}]
            for platform in ("linux", "macos", "windows", "android", "ios")
        },
        "version_rules": {},
    }
    path.write_text(json.dumps(policy), encoding="utf-8")


class ExitCodeTest(unittest.TestCase):
    def test_all_continues_after_a_broken_platform(self) -> None:
        valid = {
            "derivations": {
                drv_path(0, "librepaint-1.0"): {
                    "outputs": {
                        "out": {"path": output_path(0, "librepaint-1.0")}
                    },
                    "inputDrvs": {},
                }
            }
        }
        broken = json.loads(
            (FIXTURES / "graph-missing-input.json").read_text(encoding="utf-8")
        )
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            graphs = root / "graphs"
            graphs.mkdir()
            for platform in ("linux", "macos", "android", "ios"):
                (graphs / f"{platform}-1.json").write_text(
                    json.dumps(valid), encoding="utf-8"
                )
            (graphs / "windows-1.json").write_text(
                json.dumps(broken), encoding="utf-8"
            )
            policy_path = root / "policy.json"
            write_policy(policy_path)
            snapshot_path = root / "cache.json"
            snapshot_path.write_text("{}", encoding="utf-8")
            report_path = root / "report.json"
            stdout = io.StringIO()
            with contextlib.redirect_stdout(stdout):
                code = check_nix_deps.main(
                    [
                        "--platform",
                        "all",
                        "--graph-dir",
                        str(graphs),
                        "--cache-snapshot",
                        str(snapshot_path),
                        "--json",
                        str(report_path),
                    ],
                    policy_path=policy_path,
                    sleep=lambda _delay: None,
                )
            report = json.loads(report_path.read_text(encoding="utf-8"))
        self.assertEqual(code, 2)
        self.assertEqual(report["exit_code"], 2)
        self.assertEqual(report["platforms"]["linux"]["exit_code"], 0)
        self.assertEqual(report["platforms"]["windows"]["exit_code"], 2)
        self.assertEqual(report["platforms"]["macos"]["exit_code"], 0)

    def test_missing_target_is_exit_2(self) -> None:
        root_key, root = make_entry(0, "librepaint-1.0")
        result = run_check(
            {root_key: root},
            [root_key],
            make_policy(target_patterns=["^.+-x86_64-w64-mingw32(-|$)"]),
        )
        self.assertEqual(result["exit_code"], 2)
        self.assertIn("target", str(result["error"]))


class PolicyAndVersionTest(unittest.TestCase):
    def test_policy_patterns_compile(self) -> None:
        policy = json.loads(POLICY_PATH.read_text(encoding="utf-8"))
        check_nix_deps.validate_policy(policy)

    def test_version_parsing(self) -> None:
        self.assertEqual(
            check_nix_deps.version_tuple("6.12.0-rc1"), (6, 12, 0)
        )
        self.assertIsNone(check_nix_deps.version_tuple("unstable-2025"))
        self.assertEqual(
            check_nix_deps.compare_versions((13, 13), (13, 13, 0)), 0
        )
        self.assertEqual(
            check_nix_deps.compare_versions((13, 12), (13, 13)), -1
        )

    def test_parse_drv_name(self) -> None:
        self.assertEqual(
            check_nix_deps.parse_drv_name(
                "qtbase-x86_64-w64-mingw32-6.12.0"
            ),
            ("qtbase-x86_64-w64-mingw32", "6.12.0"),
        )
        self.assertEqual(
            check_nix_deps.parse_drv_name("python3-3.14.8-env"),
            ("python3", "3.14.8-env"),
        )
        self.assertEqual(
            check_nix_deps.parse_drv_name("hello"),
            ("hello", ""),
        )


if __name__ == "__main__":
    unittest.main()
