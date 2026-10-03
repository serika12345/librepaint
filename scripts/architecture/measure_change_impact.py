#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path, PurePosixPath
from typing import Iterable, Sequence

SCRIPT_DIRECTORY = Path(__file__).resolve().parent
if str(SCRIPT_DIRECTORY) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIRECTORY))

from extract_cmake_graph import GraphExtractionError, extract_graph


REPO_ROOT = Path(__file__).resolve().parents[2]
SOURCE_DIRECTORIES = (
    "krita",
    "libs",
    "packaging",
    "pch",
    "plugins",
    "qmlmodules",
    "sdk",
    "winquirks",
)
SOURCE_SUFFIXES = frozenset(
    {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".m", ".mm"}
)
TEST_PATH_PARTS = frozenset({"benchmarks", "test", "tests"})
LINK_TARGET_TYPES = frozenset(
    {"EXECUTABLE", "MODULE_LIBRARY", "SHARED_LIBRARY", "STATIC_LIBRARY"}
)
INCLUDE_PATTERN = re.compile(
    r'^[ \t]*#[ \t]*include[ \t]*[<"]([^>"]+)[>"]', re.MULTILINE
)
OBJECT_TARGET_PATTERN = re.compile(r"(?:^|/)CMakeFiles/([^/]+)\.dir/")


class ChangeImpactError(RuntimeError):
    """Raised when the configured build tree cannot provide a measurement."""


@dataclass(frozen=True)
class DependencyRecords:
    objects: set[str]
    stale_records: int


@dataclass(frozen=True)
class DirectIncluders:
    product_files: list[str]
    test_files: list[str]


def _normalized_path(path: Path) -> str:
    return os.path.normcase(os.path.normpath(str(path.absolute())))


def _is_test_path(path: str | PurePosixPath) -> bool:
    value = PurePosixPath(path)
    return any(part in TEST_PATH_PARTS for part in value.parts) or value.name.endswith(
        ("_test.cpp", "_test.cc", "_test.cxx")
    )


def _is_matching_header(
    dependency: str,
    *,
    build_directory: Path,
    normalized_header: str,
) -> bool:
    if Path(dependency).name != Path(normalized_header).name:
        return False
    path = Path(dependency)
    if not path.is_absolute():
        path = build_directory / path
    return _normalized_path(path) == normalized_header


def parse_dependency_records(
    lines: Iterable[str],
    *,
    build_directory: Path,
    header_path: Path,
) -> DependencyRecords:
    normalized_header = _normalized_path(header_path)
    current_object: str | None = None
    current_valid = False
    objects: set[str] = set()
    stale_records = 0

    for raw_line in lines:
        line = raw_line.rstrip("\n")
        if line and not line.startswith(" "):
            if ": #deps " not in line:
                current_object = None
                current_valid = False
                continue
            current_object = line.split(": #deps ", 1)[0]
            current_valid = line.endswith("(VALID)")
            continue
        if current_object is None or not line.startswith("    "):
            continue
        dependency = line[4:]
        if not _is_matching_header(
            dependency,
            build_directory=build_directory,
            normalized_header=normalized_header,
        ):
            continue
        if current_valid:
            objects.add(current_object)
        else:
            stale_records += 1

    return DependencyRecords(objects=objects, stale_records=stale_records)


def _include_matches_header(
    include: str,
    *,
    header: PurePosixPath,
    unique_basename: bool,
) -> bool:
    include_path = PurePosixPath(include)
    if include_path == header:
        return True
    if (
        len(include_path.parts) > 1
        and header.parts[-len(include_path.parts) :] == include_path.parts
    ):
        return True
    return unique_basename and include_path.name == header.name


def discover_direct_includers(
    repository_root: Path,
    header_path: Path,
) -> DirectIncluders:
    header = PurePosixPath(header_path.as_posix())
    basename_matches: list[Path] = []
    for directory in SOURCE_DIRECTORIES:
        source_root = repository_root / directory
        if source_root.is_dir():
            basename_matches.extend(source_root.rglob(header.name))
    unique_basename = len([path for path in basename_matches if path.is_file()]) == 1

    product_files: list[str] = []
    test_files: list[str] = []
    for directory in SOURCE_DIRECTORIES:
        source_root = repository_root / directory
        if not source_root.is_dir():
            continue
        for path in source_root.rglob("*"):
            if not path.is_file() or path.suffix not in SOURCE_SUFFIXES:
                continue
            try:
                includes = INCLUDE_PATTERN.findall(path.read_text(encoding="utf-8"))
            except UnicodeDecodeError as error:
                raise ChangeImpactError(f"source is not UTF-8: {path}") from error
            if not any(
                _include_matches_header(
                    include,
                    header=header,
                    unique_basename=unique_basename,
                )
                for include in includes
            ):
                continue
            relative = path.relative_to(repository_root).as_posix()
            if _is_test_path(relative):
                test_files.append(relative)
            else:
                product_files.append(relative)

    return DirectIncluders(
        product_files=sorted(product_files),
        test_files=sorted(test_files),
    )


def target_name_from_object(object_path: str) -> str | None:
    match = OBJECT_TARGET_PATTERN.search(object_path)
    return match.group(1) if match else None


def _source_package(source_directory: str) -> str:
    parts = PurePosixPath(source_directory).parts
    if not parts:
        return "."
    depth = 3 if parts[0] == "plugins" else 2
    return "/".join(parts[: min(depth, len(parts))])


def _load_compile_commands(build_directory: Path) -> dict[str, str]:
    path = build_directory / "compile_commands.json"
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as error:
        raise ChangeImpactError(f"compilation database is missing: {path}") from error
    except json.JSONDecodeError as error:
        raise ChangeImpactError(
            f"invalid compilation database {path}: line {error.lineno}, column {error.colno}"
        ) from error
    if not isinstance(value, list):
        raise ChangeImpactError(f"compilation database must be an array: {path}")

    result: dict[str, str] = {}
    for entry in value:
        if not isinstance(entry, dict):
            continue
        output = entry.get("output")
        source = entry.get("file")
        if not isinstance(output, str) or not isinstance(source, str):
            continue
        output_path = Path(output)
        try:
            relative_output = output_path.relative_to(build_directory).as_posix()
        except ValueError:
            relative_output = output_path.as_posix()
        result[relative_output] = source
    return result


def _run_ninja_lines(build_directory: Path, arguments: Sequence[str]) -> list[str]:
    try:
        result = subprocess.run(
            ["ninja", "-C", str(build_directory), *arguments],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
    except FileNotFoundError as error:
        raise ChangeImpactError("ninja is not available in the current environment") from error
    except subprocess.CalledProcessError as error:
        diagnostic = error.stderr.strip() or error.stdout.strip()
        raise ChangeImpactError(f"ninja {' '.join(arguments)} failed: {diagnostic}") from error
    return result.stdout.splitlines(keepends=True)


def _load_dependency_records(
    build_directory: Path,
    header_path: Path,
) -> DependencyRecords:
    try:
        process = subprocess.Popen(
            ["ninja", "-C", str(build_directory), "-t", "deps"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
    except FileNotFoundError as error:
        raise ChangeImpactError("ninja is not available in the current environment") from error
    assert process.stdout is not None
    records = parse_dependency_records(
        process.stdout,
        build_directory=build_directory,
        header_path=header_path,
    )
    _stdout, stderr = process.communicate()
    if process.returncode:
        raise ChangeImpactError(f"ninja -t deps failed: {stderr.strip()}")
    return records


def _split_targets(
    names: set[str],
    targets: dict[str, dict[str, object]],
) -> tuple[set[str], set[str], set[str]]:
    product: set[str] = set()
    test: set[str] = set()
    unknown: set[str] = set()
    for name in names:
        target = targets.get(name)
        if target is None:
            unknown.add(name)
            continue
        source_directory = target.get("sourceDirectory")
        if not isinstance(source_directory, str):
            unknown.add(name)
        elif _is_test_path(source_directory):
            test.add(name)
        else:
            product.add(name)
    return product, test, unknown


def _reverse_dependency_closure(
    names: set[str],
    targets: dict[str, dict[str, object]],
) -> set[str]:
    closure = set(names)
    while True:
        consumers = {
            name
            for name, target in targets.items()
            if closure.intersection(target.get("dependencies", []))
        }
        expanded = closure | consumers
        if expanded == closure:
            return closure
        closure = expanded


def _count_commands(build_directory: Path, target: str) -> int:
    lines = _run_ninja_lines(build_directory, ["-t", "commands", target])
    return sum(1 for line in lines if line.strip())


def measure_change_impact(
    *,
    repository_root: Path,
    build_directory: Path,
    header: Path,
    platform: str,
    owner_target: str | None,
    contract_target: str | None,
) -> dict[str, object]:
    repository_root = repository_root.resolve()
    build_directory = build_directory.resolve()
    header_path = (repository_root / header).resolve()
    try:
        header_relative = header_path.relative_to(repository_root)
    except ValueError as error:
        raise ChangeImpactError(f"header is outside the repository: {header_path}") from error
    if not header_path.is_file():
        raise ChangeImpactError(f"header does not exist: {header_path}")

    graph = extract_graph(
        build_directory / ".cmake/api/v1/reply",
        platform=platform,
        build_profile=build_directory.name,
    )
    targets = {
        target["name"]: target
        for target in graph["targets"]
        if isinstance(target, dict) and isinstance(target.get("name"), str)
    }
    compile_commands = _load_compile_commands(build_directory)
    dependencies = _load_dependency_records(build_directory, header_path)
    direct = discover_direct_includers(repository_root, header_relative)
    compile_objects = dependencies.objects & compile_commands.keys()
    auxiliary_records = dependencies.objects - compile_objects

    object_targets: set[str] = set()
    unknown_objects: set[str] = set()
    generated_compiles = 0
    automoc_compiles = 0
    for object_path in compile_objects:
        target_name = target_name_from_object(object_path)
        if target_name is None:
            unknown_objects.add(object_path)
        else:
            object_targets.add(target_name)
        source = compile_commands[object_path]
        if Path(source).is_relative_to(build_directory) or not Path(
            source
        ).is_relative_to(repository_root):
            generated_compiles += 1
        if "_autogen/mocs_compilation" in object_path:
            automoc_compiles += 1

    product_targets, test_targets, unknown_targets = _split_targets(
        object_targets, targets
    )
    product_objects = {
        object_path
        for object_path in compile_objects
        if target_name_from_object(object_path) in product_targets
    }
    test_objects = {
        object_path
        for object_path in compile_objects
        if target_name_from_object(object_path) in test_targets
    }
    link_product = {
        name
        for name in product_targets
        if targets[name].get("type") in LINK_TARGET_TYPES
    }
    link_test = {
        name
        for name in test_targets
        if targets[name].get("type") in LINK_TARGET_TYPES
    }
    relink_seeds = link_product | link_test
    if owner_target is not None:
        relink_seeds.add(owner_target)
    relink_closure = _reverse_dependency_closure(relink_seeds, targets)
    relink_targets = {
        name
        for name in relink_closure
        if name in targets and targets[name].get("type") in LINK_TARGET_TYPES
    }
    relink_product, relink_test, relink_unknown = _split_targets(
        relink_targets, targets
    )
    packages = sorted(
        {
            _source_package(str(targets[name]["sourceDirectory"]))
            for name in product_targets | test_targets
        }
    )

    owner: dict[str, object] | None = None
    if owner_target is not None:
        if owner_target not in targets:
            raise ChangeImpactError(f"owner target is missing from the CMake graph: {owner_target}")
        consumers = {
            name
            for name, target in targets.items()
            if owner_target in target.get("dependencies", [])
        }
        owner_product, owner_test, owner_unknown = _split_targets(consumers, targets)
        owner = {
            "name": owner_target,
            "directDependencies": targets[owner_target].get("dependencies", []),
            "directConsumers": {
                "total": len(consumers),
                "product": len(owner_product),
                "test": len(owner_test),
                "unknown": len(owner_unknown),
            },
        }

    contract: dict[str, object] | None = None
    if contract_target is not None:
        if contract_target not in targets:
            raise ChangeImpactError(
                f"contract target is missing from the CMake graph: {contract_target}"
            )
        contract = {
            "name": contract_target,
            "directDependencies": targets[contract_target].get("dependencies", []),
            "cleanCommandClosure": _count_commands(build_directory, contract_target),
        }

    return {
        "schemaVersion": 1,
        "platform": platform,
        "configuration": graph["configuration"],
        "buildProfile": graph["buildProfile"],
        "header": header_relative.as_posix(),
        "directIncluders": {
            "total": len(direct.product_files) + len(direct.test_files),
            "product": len(direct.product_files),
            "test": len(direct.test_files),
        },
        "transitiveCompiles": {
            "total": len(compile_objects),
            "product": len(product_objects),
            "test": len(test_objects),
            "unknown": len(compile_objects - product_objects - test_objects),
            "generated": generated_compiles,
            "automoc": automoc_compiles,
            "auxiliaryRecordsExcluded": len(auxiliary_records),
            "staleRecordsExcluded": dependencies.stale_records,
        },
        "affectedTargets": {
            "total": len(object_targets),
            "product": len(product_targets),
            "test": len(test_targets),
            "unknown": len(unknown_targets) + len(unknown_objects),
            "sourcePackages": packages,
        },
        "linkTargets": {
            "total": len(link_product) + len(link_test),
            "product": len(link_product),
            "test": len(link_test),
        },
        "relinkTargets": {
            "total": len(relink_targets),
            "product": len(relink_product),
            "test": len(relink_test),
            "unknown": len(relink_unknown),
        },
        "ownerTarget": owner,
        "contractTarget": contract,
    }


def serialize_text(report: dict[str, object]) -> str:
    direct = report["directIncluders"]
    compiles = report["transitiveCompiles"]
    targets = report["affectedTargets"]
    links = report["linkTargets"]
    relinks = report["relinkTargets"]
    assert isinstance(direct, dict)
    assert isinstance(compiles, dict)
    assert isinstance(targets, dict)
    assert isinstance(links, dict)
    assert isinstance(relinks, dict)
    lines = [
        f"change impact: {report['header']}",
        f"build: {report['platform']} {report['configuration']} ({report['buildProfile']})",
        "direct includers: "
        f"{direct['total']} total, {direct['product']} product, {direct['test']} test",
        "transitive compiles: "
        f"{compiles['total']} total, {compiles['product']} product, "
        f"{compiles['test']} test, {compiles['unknown']} unknown",
        "generated compiles: "
        f"{compiles['generated']} total, {compiles['automoc']} AUTOMOC",
        "affected targets: "
        f"{targets['total']} total, {targets['product']} product, "
        f"{targets['test']} test, {targets['unknown']} unknown",
        f"source packages: {len(targets['sourcePackages'])}",
        "link targets: "
        f"{links['total']} total, {links['product']} product, {links['test']} test",
        "relink closure: "
        f"{relinks['total']} total, {relinks['product']} product, "
        f"{relinks['test']} test, {relinks['unknown']} unknown",
    ]
    owner = report.get("ownerTarget")
    if isinstance(owner, dict):
        consumers = owner["directConsumers"]
        assert isinstance(consumers, dict)
        lines.append(
            f"owner {owner['name']} consumers: {consumers['total']} total, "
            f"{consumers['product']} product, {consumers['test']} test"
        )
    contract = report.get("contractTarget")
    if isinstance(contract, dict):
        dependencies = ", ".join(contract["directDependencies"])
        lines.append(
            f"contract {contract['name']}: {contract['cleanCommandClosure']} commands; "
            f"direct dependencies: {dependencies}"
        )
    if compiles["staleRecordsExcluded"]:
        lines.append(
            f"stale dependency records excluded: {compiles['staleRecordsExcluded']}"
        )
    if compiles["auxiliaryRecordsExcluded"]:
        lines.append(
            "non-compilation dependency records excluded: "
            f"{compiles['auxiliaryRecordsExcluded']}"
        )
    return "\n".join(lines) + "\n"


def _argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Measure the build impact of changing one repository header."
    )
    parser.add_argument("build_directory", type=Path)
    parser.add_argument("header", type=Path)
    parser.add_argument("--platform", required=True)
    parser.add_argument("--owner-target")
    parser.add_argument("--contract-target")
    parser.add_argument("--json", action="store_true")
    return parser


def main(arguments: list[str] | None = None) -> int:
    options = _argument_parser().parse_args(arguments)
    try:
        report = measure_change_impact(
            repository_root=REPO_ROOT,
            build_directory=options.build_directory,
            header=options.header,
            platform=options.platform,
            owner_target=options.owner_target,
            contract_target=options.contract_target,
        )
    except (ChangeImpactError, GraphExtractionError, OSError) as error:
        print(f"measure-change-impact: {error}", file=sys.stderr)
        return 1
    if options.json:
        print(json.dumps(report, ensure_ascii=False, indent=2))
    else:
        sys.stdout.write(serialize_text(report))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
