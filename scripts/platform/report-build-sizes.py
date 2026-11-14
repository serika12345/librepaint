#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

"""Report the output size of the dependencies that must be built locally.

The tool reuses check-nix-deps.py for the derivation graph, the cache lookup
and the own/small/target/host classification, then asks nix path-info for the
NAR size of every local output. A size is known only while the output is in
the local store; outputs that are not built yet are reported as unknown and
are never estimated.
"""

from __future__ import annotations

import argparse
import importlib.util
import json
from pathlib import Path
import sys
import time
from typing import Any, Callable, Mapping, Sequence

ROOT = Path(__file__).resolve().parents[2]
CHECK_SCRIPT = Path(__file__).resolve().parent / "check-nix-deps.py"
POLICY_PATH = ROOT / "nix" / "dependency-policy.json"

BATCH_SIZE = 200
DEFAULT_TOP = 15
MIB = 1024 * 1024
BUCKETS = ("own", "small", "target", "host")
DEPENDENCY_BUCKETS = ("small", "target", "host")


def load_check_module() -> Any:
    """Load check-nix-deps.py, whose file name is not a module name."""

    spec = importlib.util.spec_from_file_location("check_nix_deps", CHECK_SCRIPT)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot import {CHECK_SCRIPT}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


check_nix_deps = load_check_module()


# ---------------------------------------------------------------------------
# Output sizes
# ---------------------------------------------------------------------------


def parse_path_info(data: Any) -> dict[str, int]:
    """Return path -> narSize from either accepted nix path-info shape."""

    sizes: dict[str, int] = {}

    def record(path: Any, size: Any) -> None:
        if not isinstance(path, str) or not path:
            return
        if isinstance(size, bool) or not isinstance(size, int) or size < 0:
            return
        sizes[path] = size

    if isinstance(data, dict):
        for key, value in data.items():
            if isinstance(value, dict):
                record(value.get("path") or key, value.get("narSize"))
            else:
                record(key, value)
    elif isinstance(data, list):
        for item in data:
            if isinstance(item, dict):
                record(item.get("path"), item.get("narSize"))
    return sizes


def run_path_info(paths: Sequence[str]) -> str:
    return check_nix_deps.run_nix(["nix", "path-info", "--json", *paths])


def ask_batch(
    batch: Sequence[str],
    run: Callable[[Sequence[str]], str],
) -> dict[str, int]:
    """Ask for one batch and retry every path alone if the batch fails."""

    try:
        return parse_path_info(json.loads(run(batch)))
    except (check_nix_deps.CommandError, ValueError):
        sizes: dict[str, int] = {}
        for path in batch:
            try:
                sizes.update(parse_path_info(json.loads(run([path]))))
            except (check_nix_deps.CommandError, ValueError):
                continue
        return sizes


def query_sizes(
    paths: Sequence[str],
    run: Callable[[Sequence[str]], str] | None = None,
) -> dict[str, int]:
    """Return the known NAR sizes of the paths, in batches of BATCH_SIZE."""

    if run is None:
        run = run_path_info
    sizes: dict[str, int] = {}
    unique = list(dict.fromkeys(paths))
    for start in range(0, len(unique), BATCH_SIZE):
        batch = unique[start : start + BATCH_SIZE]
        sizes.update(ask_batch(batch, run))
    return sizes


def output_paths(
    graph: Mapping[str, Mapping[str, Any]], drv: str
) -> list[str]:
    """Return the output paths that the graph records for one derivation."""

    entry = graph.get(drv) or {}
    outputs = entry.get("outputs") or {}
    paths: list[str] = []
    for name in sorted(outputs):
        output = outputs[name]
        if isinstance(output, dict) and output.get("path"):
            paths.append(str(output["path"]))
    return paths


# ---------------------------------------------------------------------------
# Aggregation
# ---------------------------------------------------------------------------


def build_report(
    view: Any,
    graph: Mapping[str, Mapping[str, Any]],
    local: Sequence[str],
    own_patterns: Sequence[Any],
    small_patterns: Sequence[Any],
    sizes: Mapping[str, int],
    top_n: int = DEFAULT_TOP,
) -> dict[str, Any]:
    """Aggregate counts and sizes per bucket, plus the largest target/host."""

    buckets = {
        bucket: {"count": 0, "unknown": 0, "size": 0} for bucket in BUCKETS
    }
    entries: list[dict[str, Any]] = []
    for drv in sorted(local):
        bucket = check_nix_deps.bucket_of(
            view, own_patterns, small_patterns, drv
        )
        info = view.info(drv)
        paths = output_paths(graph, drv)
        total = 0
        known = bool(paths)
        for path in paths:
            value = sizes.get(path)
            if value is None:
                known = False
            else:
                total += value
        buckets[bucket]["count"] += 1
        if known:
            buckets[bucket]["size"] += total
        else:
            buckets[bucket]["unknown"] += 1
        entries.append(
            {
                "name": info.name,
                "bucket": bucket,
                "size": total if known else None,
            }
        )

    dependency = {
        "count": sum(buckets[bucket]["count"] for bucket in DEPENDENCY_BUCKETS),
        "unknown": sum(
            buckets[bucket]["unknown"] for bucket in DEPENDENCY_BUCKETS
        ),
        "size": sum(buckets[bucket]["size"] for bucket in DEPENDENCY_BUCKETS),
    }
    top = [
        entry
        for entry in entries
        if entry["bucket"] in ("target", "host") and entry["size"] is not None
    ]
    top.sort(key=lambda entry: (-entry["size"], entry["name"]))
    return {
        "buckets": buckets,
        "dependency": dependency,
        "top": top[:top_n],
    }


def platform_report(
    graph: Mapping[str, Mapping[str, Any]],
    policy: Mapping[str, Any],
    platform: str,
    roots: Sequence[str],
    cache_lookup: Callable[[str], bool],
    sizes_query: Callable[[Sequence[str]], Mapping[str, int]] | None = None,
    sleep: Callable[[float], None] | None = None,
    top_n: int = DEFAULT_TOP,
) -> dict[str, Any]:
    """Walk one platform graph and return its bucket counts and sizes."""

    if sizes_query is None:
        sizes_query = query_sizes
    if sleep is None:
        sleep = time.sleep
    query = check_nix_deps.make_query(cache_lookup, sleep, {}, {})
    walked = check_nix_deps.walk_graph(graph, roots, query)
    view = check_nix_deps.PlatformView(graph, policy, platform)
    local = list(walked["local"])
    paths = [
        path for drv in local for path in output_paths(graph, drv)
    ]
    sizes = sizes_query(paths) if paths else {}
    own_patterns = check_nix_deps.compile_patterns(
        policy.get("own_patterns") or [], "own_patterns"
    )
    small_patterns = check_nix_deps.compile_patterns(
        policy.get("small_patterns") or [], "small_patterns"
    )
    report = build_report(
        view, graph, local, own_patterns, small_patterns, sizes, top_n
    )
    report["platform"] = platform
    return report


# ---------------------------------------------------------------------------
# Markdown output
# ---------------------------------------------------------------------------


def mib(size: int) -> str:
    return f"{size / MIB:.1f}"


def platform_lines(report: Mapping[str, Any], top_n: int) -> list[str]:
    platform = report["platform"]
    lines = [f"## {platform}", ""]
    if report.get("error"):
        lines.append(f"エラー: {report['error']}")
        return lines

    lines.append("| 分類 | 件数 | サイズ不明 | 合計サイズ (MiB) |")
    lines.append("| --- | ---: | ---: | ---: |")
    for bucket in BUCKETS:
        row = report["buckets"][bucket]
        lines.append(
            f"| {bucket} | {row['count']} | {row['unknown']} "
            f"| {mib(row['size'])} |"
        )
    dependency = report["dependency"]
    lines.append(
        "| 依存の合計（small + target + host） "
        f"| {dependency['count']} | {dependency['unknown']} "
        f"| {mib(dependency['size'])} |"
    )
    if dependency["unknown"]:
        lines.append("")
        lines.append(
            f"不明 {dependency['unknown']} 件（構築されていない）"
        )

    lines.append("")
    lines.append(
        f"### サイズの大きい順（target と host、上位 {top_n} 件）"
    )
    lines.append("")
    if not report["top"]:
        lines.append("サイズが分かる target と host はありません。")
        return lines
    lines.append("| # | 派生物 | 分類 | サイズ (MiB) |")
    lines.append("| ---: | --- | --- | ---: |")
    for index, entry in enumerate(report["top"], start=1):
        lines.append(
            f"| {index} | {entry['name']} | {entry['bucket']} "
            f"| {mib(entry['size'])} |"
        )
    return lines


def total_lines(reports: Sequence[Mapping[str, Any]]) -> list[str]:
    lines = [
        "## プラットフォームの全体",
        "",
        "| プラットフォーム | 件数 | サイズ不明 | 依存の合計 (MiB) |",
        "| --- | ---: | ---: | ---: |",
    ]
    for report in reports:
        platform = report["platform"]
        if report.get("error"):
            detail = str(report["error"]).replace("|", "\\|")
            lines.append(f"| {platform} | - | - | エラー: {detail} |")
            continue
        dependency = report["dependency"]
        lines.append(
            f"| {platform} | {dependency['count']} "
            f"| {dependency['unknown']} | {mib(dependency['size'])} |"
        )
    return lines


# ---------------------------------------------------------------------------
# Command line
# ---------------------------------------------------------------------------


def parse_arguments(argv: Sequence[str] | None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="ビルドが必須な依存の出力サイズを集計する"
    )
    parser.add_argument(
        "--platform",
        choices=check_nix_deps.PLATFORMS,
        help="集計するプラットフォーム（既定は policy のすべて）",
    )
    parser.add_argument("--graph-dir", help="保存したグラフを読むディレクトリ")
    parser.add_argument(
        "--cache-snapshot", help="キャッシュの代わりに使う JSON"
    )
    parser.add_argument("--json", dest="json_path", help="同じ内容を書く JSON")
    parser.add_argument(
        "--top",
        type=int,
        default=DEFAULT_TOP,
        help=f"大きく表示する件数（既定 {DEFAULT_TOP}）",
    )
    return parser.parse_args(argv)


def main(
    argv: Sequence[str] | None = None,
    policy_path: Path | None = None,
    sleep: Callable[[float], None] | None = None,
    sizes_query: Callable[[Sequence[str]], Mapping[str, int]] | None = None,
) -> int:
    args = parse_arguments(argv)
    if sleep is None:
        sleep = time.sleep
    if sizes_query is None:
        sizes_query = query_sizes
    policy_path = Path(policy_path) if policy_path is not None else POLICY_PATH
    try:
        policy = check_nix_deps.load_json_file(policy_path, "policy")
        check_nix_deps.validate_policy(policy)
    except check_nix_deps.ConfigError as error:
        print(f"エラー: {error}", file=sys.stderr)
        return 2
    if args.top < 0:
        print("エラー: --top は 0 以上にしてください", file=sys.stderr)
        return 2

    targets = policy.get("targets") or {}
    if args.platform is None:
        platforms = list(targets)
    elif args.platform not in targets:
        print(
            f"エラー: policy にプラットフォーム {args.platform} がありません",
            file=sys.stderr,
        )
        return 2
    else:
        platforms = [args.platform]

    if args.cache_snapshot:
        try:
            raw_snapshot = check_nix_deps.load_json_file(
                Path(args.cache_snapshot), "キャッシュの記録"
            )
        except check_nix_deps.ConfigError as error:
            print(f"エラー: {error}", file=sys.stderr)
            return 2
        if not isinstance(raw_snapshot, dict):
            print("エラー: キャッシュの記録が辞書ではありません", file=sys.stderr)
            return 2
        cache_lookup = check_nix_deps.snapshot_lookup(
            {str(key): bool(value) for key, value in raw_snapshot.items()}
        )
    else:
        cache_lookup = check_nix_deps.network_lookup

    reports: list[dict[str, Any]] = []
    overall = 0
    for platform in platforms:
        try:
            graph, roots = check_nix_deps.load_platform(
                platform, policy, args
            )
            report = platform_report(
                graph,
                policy,
                platform,
                roots,
                cache_lookup,
                sizes_query=sizes_query,
                sleep=sleep,
                top_n=args.top,
            )
        except (
            check_nix_deps.ConfigError,
            check_nix_deps.GraphError,
            check_nix_deps.CommandError,
            check_nix_deps.CacheQueryError,
        ) as error:
            report = {"platform": platform, "error": str(error)}
            overall = 2
        reports.append(report)
        print("\n".join(platform_lines(report, args.top)))
        print()

    print("\n".join(total_lines(reports)))
    if args.json_path:
        check_nix_deps.save_json_file(
            Path(args.json_path),
            {"platforms": {report["platform"]: report for report in reports},
             "exit_code": overall},
        )
    return overall


if __name__ == "__main__":
    sys.exit(main())
