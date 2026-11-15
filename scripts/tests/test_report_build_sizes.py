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

TESTS_DIR = Path(__file__).resolve().parent
SCRIPT = TESTS_DIR.parent / "platform" / "report-build-sizes.py"
SPEC = importlib.util.spec_from_file_location(
    "report_build_sizes", SCRIPT
)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"cannot import {SCRIPT}")
report_build_sizes = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = report_build_sizes
SPEC.loader.exec_module(report_build_sizes)

MIB = 1024 * 1024
HASH_CHARS = "0123456789abcdfghijklmnpqrsvwxyz"


def digest(index: int) -> str:
    return HASH_CHARS[index] * 32


def drv_path(index: int, name: str) -> str:
    return f"/nix/store/{digest(index)}-{name}.drv"


def output_path(index: int, name: str) -> str:
    return f"/nix/store/{digest(index)}-{name}"


def make_entry(
    index: int,
    name: str,
    inputs: dict[str, list[str]] | None = None,
    outputs: dict[str, dict[str, object]] | None = None,
) -> tuple[str, dict[str, object]]:
    if outputs is None:
        outputs = {"out": {"path": output_path(index, name)}}
    return drv_path(index, name), {
        "outputs": outputs,
        "inputDrvs": inputs or {},
    }


def make_policy(target_patterns: list[str] | None = None) -> dict[str, object]:
    return {
        "targets": {
            "test": {
                "attrs": ["packages.test"],
                "target_patterns": list(
                    target_patterns
                    or ["^.+-x86_64-w64-mingw32(-|$)"]
                ),
            }
        },
        "own_patterns": ["^librepaint"],
        "small_patterns": ["-hook$"],
        "host_local_allow": {"test": [{"match": ".*", "reason": "test"}]},
        "version_rules": {},
    }


def fixed_graph():
    """Return a graph with two own, one small, one target and four host."""

    root_key, root = make_entry(0, "librepaint-1.0")
    own_key, own = make_entry(1, "librepaint-plugin-1.0")
    small_key, small = make_entry(2, "small-hook")
    target_key, target = make_entry(
        3, "qtbase-x86_64-w64-mingw32-6.12.0"
    )
    host_key, host = make_entry(4, "hosttool-1.0")
    missing_key, missing = make_entry(
        5, "missing-lib-1.0", outputs={"out": {}}
    )
    huge_key, huge = make_entry(6, "huge-1.0")
    small_tool_key, small_tool = make_entry(7, "smalltool-1.0")
    root["inputDrvs"] = {
        own_key: ["out"],
        small_key: ["out"],
        target_key: ["out"],
        host_key: ["out"],
        missing_key: ["out"],
        huge_key: ["out"],
        small_tool_key: ["out"],
    }
    entries = {
        key: entry
        for key, entry in (
            (root_key, root),
            (own_key, own),
            (small_key, small),
            (target_key, target),
            (host_key, host),
            (missing_key, missing),
            (huge_key, huge),
            (small_tool_key, small_tool),
        )
    }
    sizes = {
        output_path(0, "librepaint-1.0"): 1 * MIB,
        output_path(1, "librepaint-plugin-1.0"): 2 * MIB,
        output_path(2, "small-hook"): MIB // 2,
        output_path(3, "qtbase-x86_64-w64-mingw32-6.12.0"): 100 * MIB,
        output_path(4, "hosttool-1.0"): 30 * MIB,
        output_path(6, "huge-1.0"): 200 * MIB,
        output_path(7, "smalltool-1.0"): 5 * MIB,
    }
    return entries, [root_key], sizes


def report_for(top_n: int = 15) -> dict[str, object]:
    entries, roots, sizes = fixed_graph()
    graph = report_build_sizes.check_nix_deps.normalize_graph(
        {"derivations": entries}
    )
    normalized_roots = [root.rsplit("/", 1)[-1] for root in roots]

    def sizes_query(paths):
        return {path: sizes[path] for path in paths if path in sizes}

    return report_build_sizes.platform_report(
        graph,
        make_policy(),
        "test",
        normalized_roots,
        lambda _path: False,
        sizes_query=sizes_query,
        sleep=lambda _delay: None,
        top_n=top_n,
    )


class PathInfoShapeTest(unittest.TestCase):
    def test_dict_shape(self) -> None:
        def run(paths):
            return json.dumps(
                {path: {"path": path, "narSize": 10} for path in paths}
            )

        sizes = report_build_sizes.query_sizes(
            ["/nix/store/a", "/nix/store/b"], run
        )
        self.assertEqual(sizes, {"/nix/store/a": 10, "/nix/store/b": 10})

    def test_list_shape(self) -> None:
        def run(paths):
            return json.dumps(
                [{"path": path, "narSize": 20} for path in paths]
            )

        sizes = report_build_sizes.query_sizes(
            ["/nix/store/a", "/nix/store/b"], run
        )
        self.assertEqual(sizes, {"/nix/store/a": 20, "/nix/store/b": 20})

    def test_dict_without_nar_size_is_unknown(self) -> None:
        def run(paths):
            return json.dumps({"/nix/store/a": {"path": "/nix/store/a"}})

        self.assertEqual(report_build_sizes.query_sizes(["/nix/store/a"], run), {})

    def test_failed_batch_falls_back_to_single_paths(self) -> None:
        error = report_build_sizes.check_nix_deps.CommandError
        calls: list[list[str]] = []

        def run(paths):
            calls.append(list(paths))
            if len(paths) > 1:
                raise error("batch failed")
            return json.dumps([{"path": paths[0], "narSize": 7}])

        sizes = report_build_sizes.query_sizes(
            ["/nix/store/a", "/nix/store/b"], run
        )
        self.assertEqual(sizes, {"/nix/store/a": 7, "/nix/store/b": 7})
        self.assertEqual(calls, [["/nix/store/a", "/nix/store/b"],
                                 ["/nix/store/a"], ["/nix/store/b"]])


class StorePathTest(unittest.TestCase):
    """The graph and nix path-info may name outputs without the store."""

    def graph_with_shape(self, name_only: bool):
        entries, roots, sizes = fixed_graph()
        if name_only:
            for entry in entries.values():
                for output in (entry.get("outputs") or {}).values():
                    if isinstance(output, dict) and output.get("path"):
                        output["path"] = str(output["path"]).rsplit("/", 1)[-1]
        return entries, roots, sizes

    def report_through_path_info(
        self, name_only_graph: bool, name_only_answer: bool
    ) -> tuple[dict[str, object], list[list[str]]]:
        entries, roots, sizes = self.graph_with_shape(name_only_graph)
        graph = report_build_sizes.check_nix_deps.normalize_graph(
            {"derivations": entries}
        )
        normalized_roots = [root.rsplit("/", 1)[-1] for root in roots]
        calls: list[list[str]] = []

        def run(paths):
            calls.append(list(paths))
            answer = {}
            for path in paths:
                if path not in sizes:
                    continue
                key = path.rsplit("/", 1)[-1] if name_only_answer else path
                answer[key] = {"path": key, "narSize": sizes[path]}
            return json.dumps(answer)

        def sizes_query(paths):
            return report_build_sizes.query_sizes(paths, run)

        report = report_build_sizes.platform_report(
            graph,
            make_policy(),
            "test",
            normalized_roots,
            lambda _path: False,
            sizes_query=sizes_query,
            sleep=lambda _delay: None,
        )
        return report, calls

    def assert_totals(self, report: dict[str, object]) -> None:
        buckets = report["buckets"]
        self.assertEqual(buckets["own"]["size"], 3 * MIB)
        self.assertEqual(buckets["small"]["size"], MIB // 2)
        self.assertEqual(buckets["target"]["size"], 100 * MIB)
        self.assertEqual(buckets["host"]["size"], 235 * MIB)
        self.assertEqual(report["dependency"]["unknown"], 1)

    def assert_full_store_arguments(self, calls: list[list[str]]) -> None:
        paths = [path for call in calls for path in call]
        self.assertTrue(paths)
        for path in paths:
            self.assertTrue(path.startswith("/nix/store/"), path)

    def test_name_only_graph_and_name_only_answer(self) -> None:
        report, calls = self.report_through_path_info(True, True)
        self.assert_totals(report)
        self.assert_full_store_arguments(calls)

    def test_name_only_graph_and_full_answer(self) -> None:
        report, calls = self.report_through_path_info(True, False)
        self.assert_totals(report)
        self.assert_full_store_arguments(calls)

    def test_full_graph_and_full_answer(self) -> None:
        report, calls = self.report_through_path_info(False, False)
        self.assert_totals(report)
        self.assert_full_store_arguments(calls)


class BucketTotalTest(unittest.TestCase):
    def test_counts_and_sizes_per_bucket(self) -> None:
        result = report_for()
        buckets = result["buckets"]
        self.assertEqual(buckets["own"]["count"], 2)
        self.assertEqual(buckets["small"]["count"], 1)
        self.assertEqual(buckets["target"]["count"], 1)
        self.assertEqual(buckets["host"]["count"], 4)
        self.assertEqual(buckets["own"]["size"], 3 * MIB)
        self.assertEqual(buckets["small"]["size"], MIB // 2)
        self.assertEqual(buckets["target"]["size"], 100 * MIB)
        self.assertEqual(buckets["host"]["size"], 235 * MIB)

    def test_missing_output_is_unknown(self) -> None:
        result = report_for()
        self.assertEqual(result["buckets"]["host"]["unknown"], 1)
        self.assertEqual(result["dependency"]["unknown"], 1)

    def test_own_is_not_part_of_the_dependency_total(self) -> None:
        result = report_for()
        dependency = result["dependency"]
        self.assertEqual(dependency["count"], 6)
        self.assertEqual(dependency["size"], 335 * MIB + MIB // 2)


class TopTest(unittest.TestCase):
    def test_top_is_sorted_from_largest(self) -> None:
        result = report_for(top_n=3)
        self.assertEqual(
            [entry["name"] for entry in result["top"]],
            ["huge-1.0", "qtbase-x86_64-w64-mingw32-6.12.0", "hosttool-1.0"],
        )
        sizes = [entry["size"] for entry in result["top"]]
        self.assertEqual(sizes, sorted(sizes, reverse=True))

    def test_top_holds_only_target_and_host(self) -> None:
        result = report_for()
        buckets = {entry["bucket"] for entry in result["top"]}
        self.assertEqual(buckets, {"target", "host"})


def write_policy(path: Path) -> None:
    policy = make_policy()
    policy["targets"] = {"linux": policy["targets"]["test"]}
    policy["host_local_allow"] = {
        "linux": [{"match": ".*", "reason": "test"}]
    }
    path.write_text(json.dumps(policy), encoding="utf-8")


class CommandLineTest(unittest.TestCase):
    def test_json_report_matches_the_tables(self) -> None:
        entries, roots, sizes = fixed_graph()
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            graphs = root / "graphs"
            graphs.mkdir()
            (graphs / "linux-1.json").write_text(
                json.dumps({"derivations": entries}), encoding="utf-8"
            )
            policy_path = root / "policy.json"
            write_policy(policy_path)
            snapshot_path = root / "cache.json"
            snapshot_path.write_text("{}", encoding="utf-8")
            report_path = root / "report.json"
            stdout = io.StringIO()

            def sizes_query(paths):
                return {path: sizes[path] for path in paths if path in sizes}

            with contextlib.redirect_stdout(stdout):
                code = report_build_sizes.main(
                    [
                        "--platform",
                        "linux",
                        "--graph-dir",
                        str(graphs),
                        "--cache-snapshot",
                        str(snapshot_path),
                        "--json",
                        str(report_path),
                        "--top",
                        "2",
                    ],
                    policy_path=policy_path,
                    sleep=lambda _delay: None,
                    sizes_query=sizes_query,
                )
            report = json.loads(report_path.read_text(encoding="utf-8"))
            output = stdout.getvalue()

        self.assertEqual(code, 0)
        linux = report["platforms"]["linux"]
        self.assertEqual(linux["buckets"]["own"]["count"], 2)
        self.assertEqual(linux["dependency"]["count"], 6)
        self.assertEqual(linux["dependency"]["unknown"], 1)
        self.assertEqual(
            [entry["name"] for entry in linux["top"]],
            ["huge-1.0", "qtbase-x86_64-w64-mingw32-6.12.0"],
        )
        self.assertIn("## linux", output)
        self.assertIn("不明 1 件（構築されていない）", output)
        self.assertIn("## プラットフォームの全体", output)


if __name__ == "__main__":
    unittest.main()
