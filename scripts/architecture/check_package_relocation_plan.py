#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
from pathlib import Path, PurePosixPath
from typing import Any


SCRIPT_DIRECTORY = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIRECTORY.parents[1]
PLATFORMS = ("macos", "linux", "ios", "android", "windows")
PLAN_SCOPE = "r1-g5-complete-package-relocation-plan"
PLAN_INPUTS = {
    "packageResponsibilityMap": (
        "docs/architecture/package-responsibilities.json"
    ),
    "allowedDependencyPolicy": (
        "docs/architecture/allowed-package-dependencies.json"
    ),
    "dependencyViolationBaseline": (
        "docs/architecture/dependency-violation-baseline.json"
    ),
    "structuralDependencyBaseline": (
        "docs/architecture/structural-dependency-baseline.json"
    ),
    "cmakeGraphs": "docs/architecture/cmake-targets-<platform>.json",
}
ROOT_FIELDS = {
    "schemaVersion",
    "scope",
    "inputs",
    "principles",
    "packages",
    "reviewedInternalHeaderDestinations",
    "compatibilityRoutes",
    "migrationWaves",
    "firstImplementationWave",
    "finalState",
}
PACKAGE_FIELDS = {
    "responsibility",
    "layer",
    "migrationWave",
    "current",
    "target",
    "completionCriteria",
}
CURRENT_FIELDS = {
    "sourceDirectories",
    "cmakeTargets",
    "uiClassAreas",
    "uiToolClassAreas",
    "publicSurfaces",
}
TARGET_FIELDS = {
    "rootDirectory",
    "cppNamespace",
    "primaryCmakeTarget",
    "allowedDependencies",
    "cmakeTargets",
}
TARGET_ENTRY_FIELDS = {"name", "directory", "role", "status"}
TARGET_STATUSES = {"retained", "new", "relocated"}
DESTINATION_FIELDS = {
    "ownerTarget",
    "headerPath",
    "destinationResponsibility",
    "destinationDirectory",
    "wave",
    "disposition",
}
ROUTE_FIELDS = {
    "id",
    "kind",
    "legacy",
    "replacement",
    "introducedBy",
    "owner",
    "maximumScope",
    "removalCondition",
    "verification",
}
WAVE_FIELDS = {
    "id",
    "order",
    "responsibilities",
    "purpose",
    "prerequisites",
    "sourceMoves",
    "createsTargets",
    "compatibilityRoutes",
    "requiredContracts",
    "reverseBaselineReductions",
    "maximumReverseDirectIncludesAfterWave",
    "maximumInternalDirectReferencesAfterWave",
    "completionCriteria",
    "stopCondition",
}
MOVE_FIELDS = {"from", "to", "target"}
REDUCTION_FIELDS = {
    "sourceResponsibility",
    "dependencyResponsibility",
    "from",
    "to",
}
FINAL_STATE = {
    "reverseDirectIncludes": 0,
    "unresolvedProjections": 0,
    "targetCycleComponents": 0,
    "internalHeaderDirectReferences": 0,
    "temporaryCompatibilityRoutes": 0,
}


class RelocationPlanError(RuntimeError):
    """Raised when the R1-G5 relocation plan is incomplete or stale."""


def _load_json(path: Path, description: str) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as error:
        raise RelocationPlanError(f"{description} does not exist: {path}") from error
    except json.JSONDecodeError as error:
        raise RelocationPlanError(
            f"invalid JSON in {path}: line {error.lineno}, column {error.colno}"
        ) from error
    if not isinstance(value, dict):
        raise RelocationPlanError(f"expected an object for {description}")
    return value


def load_plan(path: Path) -> dict[str, Any]:
    return _load_json(path, "package relocation plan")


def _object(value: Any, description: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise RelocationPlanError(f"expected an object for {description}")
    return value


def _array(value: Any, description: str) -> list[Any]:
    if not isinstance(value, list):
        raise RelocationPlanError(f"expected an array for {description}")
    return value


def _string(value: Any, description: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise RelocationPlanError(
            f"expected a non-empty string for {description}"
        )
    return value


def _integer(value: Any, description: str) -> int:
    if not isinstance(value, int) or isinstance(value, bool) or value < 0:
        raise RelocationPlanError(
            f"expected a nonnegative integer for {description}"
        )
    return value


def _fields(
    value: dict[str, Any], expected: set[str], description: str
) -> None:
    actual = set(value)
    if actual != expected:
        raise RelocationPlanError(
            f"unexpected fields for {description}; "
            f"missing={sorted(expected - actual)}, "
            f"unexpected={sorted(actual - expected)}"
        )


def _strings(value: Any, description: str, *, nonempty: bool = False) -> list[str]:
    result = [
        _string(item, f"{description} entry")
        for item in _array(value, description)
    ]
    if nonempty and not result:
        raise RelocationPlanError(f"expected at least one entry for {description}")
    if result != sorted(set(result)):
        raise RelocationPlanError(f"expected sorted unique values for {description}")
    return result


def _ordered_strings(
    value: Any, description: str, *, nonempty: bool = False
) -> list[str]:
    result = [
        _string(item, f"{description} entry")
        for item in _array(value, description)
    ]
    if nonempty and not result:
        raise RelocationPlanError(f"expected at least one entry for {description}")
    if len(result) != len(set(result)):
        raise RelocationPlanError(f"expected unique values for {description}")
    return result


def _is_below(path: str, root: str) -> bool:
    candidate = PurePosixPath(path)
    parent = PurePosixPath(root)
    return candidate == parent or parent in candidate.parents


def _repository_inputs(repository_root: Path) -> dict[str, dict[str, Any]]:
    return {
        key: _load_json(repository_root / value, key)
        for key, value in PLAN_INPUTS.items()
        if key != "cmakeGraphs"
    }


def _current_targets(repository_root: Path) -> set[str]:
    result: set[str] = set()
    for platform in PLATFORMS:
        path = repository_root / f"docs/architecture/cmake-targets-{platform}.json"
        graph = _load_json(path, f"{platform} CMake graph")
        if graph.get("schemaVersion") != 1 or graph.get("platform") != platform:
            raise RelocationPlanError(f"invalid CMake graph: {path}")
        for item in _array(graph.get("targets"), f"{platform} targets"):
            entry = _object(item, f"{platform} target")
            result.add(_string(entry.get("name"), f"{platform} target name"))
    return result


def _validate_packages(
    plan: dict[str, Any], inputs: dict[str, dict[str, Any]], current_targets: set[str]
) -> tuple[dict[str, dict[str, Any]], set[str], set[str]]:
    responsibility_map = inputs["packageResponsibilityMap"]
    policy = inputs["allowedDependencyPolicy"]
    current_by_id = {
        _string(item.get("id"), "current responsibility id"): item
        for raw in _array(
            responsibility_map.get("responsibilities"), "current responsibilities"
        )
        for item in [_object(raw, "current responsibility")]
    }
    policy_by_id = {
        _string(item.get("id"), "policy responsibility id"): item
        for raw in _array(policy.get("responsibilities"), "policy responsibilities")
        for item in [_object(raw, "policy responsibility")]
    }
    packages = [
        _object(item, "package")
        for item in _array(plan.get("packages"), "packages")
    ]
    ids = [_string(item.get("responsibility"), "package responsibility") for item in packages]
    if ids != sorted(current_by_id) or set(ids) != set(policy_by_id):
        raise RelocationPlanError("packages do not cover the responsibility policy")

    result: dict[str, dict[str, Any]] = {}
    new_targets: set[str] = set()
    planned_targets: set[str] = set()
    target_owners: dict[str, str] = {}
    root_owners: dict[str, str] = {}
    for package in packages:
        responsibility = package["responsibility"]
        _fields(package, PACKAGE_FIELDS, f"package {responsibility}")
        current = _object(package.get("current"), f"{responsibility} current state")
        target = _object(package.get("target"), f"{responsibility} target state")
        _fields(current, CURRENT_FIELDS, f"{responsibility} current state")
        _fields(target, TARGET_FIELDS, f"{responsibility} target state")
        evidence = current_by_id[responsibility]
        policy_entry = policy_by_id[responsibility]
        expected_current = {
            "sourceDirectories": evidence.get("sourceDirectories"),
            "cmakeTargets": evidence.get("ownerTargets"),
            "uiClassAreas": evidence.get("uiClassAreas"),
            "uiToolClassAreas": evidence.get("uiToolClassAreas"),
            "publicSurfaces": [
                _string(item.get("id"), f"{responsibility} public surface")
                for raw in _array(
                    policy_entry.get("publicSurfaces"),
                    f"{responsibility} public surfaces",
                )
                for item in [_object(raw, f"{responsibility} public surface")]
            ],
        }
        if current != expected_current:
            raise RelocationPlanError(
                f"current package evidence is stale for {responsibility}"
            )
        layer = _integer(package.get("layer"), f"{responsibility} layer")
        if layer != policy_entry.get("layer"):
            raise RelocationPlanError(f"layer is stale for {responsibility}")
        expected_dependencies = sorted(
            {
                _string(item.get("responsibility"), "allowed dependency")
                for raw in _array(
                    policy_entry.get("allowedDependencies"),
                    f"{responsibility} allowed dependencies",
                )
                for item in [_object(raw, "allowed dependency")]
            }
        )
        if _strings(
            target.get("allowedDependencies"),
            f"{responsibility} allowed dependencies",
        ) != expected_dependencies:
            raise RelocationPlanError(
                f"allowed dependencies are stale for {responsibility}"
            )
        root = _string(target.get("rootDirectory"), f"{responsibility} root")
        if PurePosixPath(root).is_absolute() or ".." in PurePosixPath(root).parts:
            raise RelocationPlanError(f"invalid package root for {responsibility}")
        if root in root_owners:
            raise RelocationPlanError(f"duplicate package root: {root}")
        root_owners[root] = responsibility
        namespace = _string(
            target.get("cppNamespace"), f"{responsibility} C++ namespace"
        )
        if not namespace.startswith("Krita::"):
            raise RelocationPlanError(
                f"target namespace must begin with Krita:: for {responsibility}"
            )
        primary = _string(
            target.get("primaryCmakeTarget"), f"{responsibility} primary target"
        )
        entries = [
            _object(item, f"{responsibility} target entry")
            for item in _array(target.get("cmakeTargets"), f"{responsibility} targets")
        ]
        names: list[str] = []
        for entry in entries:
            _fields(entry, TARGET_ENTRY_FIELDS, f"{responsibility} target entry")
            name = _string(entry.get("name"), f"{responsibility} target name")
            directory = _string(
                entry.get("directory"), f"{responsibility} target directory"
            )
            _string(entry.get("role"), f"{responsibility} target role")
            status = _string(entry.get("status"), f"{responsibility} target status")
            if status not in TARGET_STATUSES:
                raise RelocationPlanError(f"invalid target status for {name}: {status}")
            if name in target_owners:
                raise RelocationPlanError(f"planned target has multiple owners: {name}")
            target_owners[name] = responsibility
            names.append(name)
            if status == "new":
                if name in current_targets:
                    raise RelocationPlanError(f"new target already exists: {name}")
                if not _is_below(directory, root):
                    raise RelocationPlanError(
                        f"new target directory is outside package root: {name}"
                    )
                new_targets.add(name)
            elif name not in current_targets:
                raise RelocationPlanError(f"current target is missing: {name}")
        if names != sorted(set(names)) or primary not in names:
            raise RelocationPlanError(
                f"target list must be sorted, unique, and contain the primary for {responsibility}"
            )
        _ordered_strings(
            package.get("completionCriteria"),
            f"{responsibility} completion criteria",
            nonempty=True,
        )
        wave = package.get("migrationWave")
        if wave is not None:
            _string(wave, f"{responsibility} migration wave")
        planned_targets.update(names)
        result[responsibility] = package
    return result, new_targets, planned_targets


def _validate_routes(plan: dict[str, Any], wave_ids: set[str]) -> dict[str, dict[str, Any]]:
    routes: dict[str, dict[str, Any]] = {}
    for raw in _array(plan.get("compatibilityRoutes"), "compatibility routes"):
        route = _object(raw, "compatibility route")
        route_id = _string(route.get("id"), "compatibility route id")
        _fields(route, ROUTE_FIELDS, f"compatibility route {route_id}")
        if route_id in routes:
            raise RelocationPlanError(f"duplicate compatibility route: {route_id}")
        for field in ROUTE_FIELDS - {"id"}:
            description = field.replace("removalCondition", "removal condition")
            _string(route.get(field), description)
        if route["introducedBy"] not in wave_ids:
            raise RelocationPlanError(f"unknown compatibility route wave: {route_id}")
        if route["owner"] != "R1-G7":
            raise RelocationPlanError(f"compatibility route owner must be R1-G7: {route_id}")
        routes[route_id] = route
    return routes


def _validate_waves(
    plan: dict[str, Any],
    packages: dict[str, dict[str, Any]],
    new_targets: set[str],
) -> tuple[list[dict[str, Any]], dict[str, int]]:
    waves = [
        _object(item, "migration wave")
        for item in _array(plan.get("migrationWaves"), "migration waves")
    ]
    ids = [_string(wave.get("id"), "migration wave id") for wave in waves]
    orders = [_integer(wave.get("order"), "migration wave order") for wave in waves]
    if len(ids) != len(set(ids)) or orders != list(range(1, len(waves) + 1)):
        raise RelocationPlanError("migration waves must have unique IDs and consecutive order")
    if plan.get("firstImplementationWave") != (ids[0] if ids else None):
        raise RelocationPlanError("first implementation wave must be the first migration wave")
    order_by_id = dict(zip(ids, orders))
    routes = _validate_routes(plan, set(ids))
    referenced_routes: list[str] = []
    created_targets: list[str] = []
    migrated: list[str] = []
    for wave in waves:
        wave_id = wave["id"]
        _fields(wave, WAVE_FIELDS, f"migration wave {wave_id}")
        responsibilities = _ordered_strings(
            wave.get("responsibilities"),
            f"{wave_id} responsibilities",
            nonempty=True,
        )
        for responsibility in responsibilities:
            if responsibility not in packages:
                raise RelocationPlanError(f"unknown responsibility in {wave_id}: {responsibility}")
        migrated.extend(responsibilities)
        _string(wave.get("purpose"), f"{wave_id} purpose")
        _ordered_strings(wave.get("prerequisites"), f"{wave_id} prerequisites", nonempty=True)
        for raw_move in _array(wave.get("sourceMoves"), f"{wave_id} source moves"):
            move = _object(raw_move, f"{wave_id} source move")
            _fields(move, MOVE_FIELDS, f"{wave_id} source move")
            for field in MOVE_FIELDS:
                _string(move.get(field), f"{wave_id} source move {field}")
        created_targets.extend(
            _ordered_strings(wave.get("createsTargets"), f"{wave_id} created targets")
        )
        wave_routes = _ordered_strings(
            wave.get("compatibilityRoutes"), f"{wave_id} compatibility routes"
        )
        for route_id in wave_routes:
            if route_id not in routes or routes[route_id]["introducedBy"] != wave_id:
                raise RelocationPlanError(
                    f"compatibility route is assigned to the wrong wave: {route_id}"
                )
        referenced_routes.extend(wave_routes)
        _ordered_strings(
            wave.get("requiredContracts"), f"{wave_id} required contracts", nonempty=True
        )
        _ordered_strings(
            wave.get("completionCriteria"), f"{wave_id} completion criteria", nonempty=True
        )
        _string(wave.get("stopCondition"), f"{wave_id} stop condition")
        _integer(
            wave.get("maximumReverseDirectIncludesAfterWave"),
            f"{wave_id} reverse dependency maximum",
        )
        internal_maximum = _object(
            wave.get("maximumInternalDirectReferencesAfterWave"),
            f"{wave_id} internal reference maximum",
        )
        _fields(internal_maximum, {"kritaimage", "kritaui"}, f"{wave_id} internal maximum")
        for owner in ("kritaimage", "kritaui"):
            _integer(internal_maximum.get(owner), f"{wave_id} {owner} maximum")
    expected_migrated = sorted(
        responsibility
        for responsibility, package in packages.items()
        if package.get("migrationWave") is not None
    )
    if sorted(migrated) != expected_migrated or len(migrated) != len(set(migrated)):
        raise RelocationPlanError("migration waves do not cover planned packages exactly once")
    if set(created_targets) != new_targets or len(created_targets) != len(set(created_targets)):
        raise RelocationPlanError("migration waves do not create planned targets exactly once")
    if set(referenced_routes) != set(routes) or len(referenced_routes) != len(set(referenced_routes)):
        raise RelocationPlanError("migration waves do not introduce compatibility routes exactly once")

    for responsibility, package in packages.items():
        wave_id = package.get("migrationWave")
        source_order = order_by_id.get(wave_id, 0)
        for dependency in package["target"]["allowedDependencies"]:
            dependency_order = order_by_id.get(
                packages[dependency].get("migrationWave"), 0
            )
            if dependency_order > source_order:
                raise RelocationPlanError(
                    "migration wave order violates allowed dependency order: "
                    f"{responsibility} precedes {dependency}"
                )
    actual_wave_by_responsibility = {
        responsibility: wave["id"]
        for wave in waves
        for responsibility in wave["responsibilities"]
    }
    for responsibility, package in packages.items():
        if package.get("migrationWave") != actual_wave_by_responsibility.get(
            responsibility
        ):
            raise RelocationPlanError(
                f"package migration wave mismatch for {responsibility}"
            )
    return waves, order_by_id


def _validate_reverse_reductions(
    waves: list[dict[str, Any]], baseline: dict[str, Any]
) -> None:
    baseline_by_pair = {
        (
            _string(item.get("sourceResponsibility"), "violation source"),
            _string(item.get("dependencyResponsibility"), "violation dependency"),
        ): _integer(item.get("maximumDirectIncludes"), "violation maximum")
        for raw in _array(baseline.get("violations"), "dependency violations")
        for item in [_object(raw, "dependency violation")]
    }
    remaining = sum(baseline_by_pair.values())
    reduced: set[tuple[str, str]] = set()
    for wave in waves:
        for raw in _array(
            wave.get("reverseBaselineReductions"),
            f"{wave['id']} reverse reductions",
        ):
            reduction = _object(raw, f"{wave['id']} reverse reduction")
            _fields(reduction, REDUCTION_FIELDS, f"{wave['id']} reverse reduction")
            pair = (
                _string(reduction.get("sourceResponsibility"), "reduction source"),
                _string(reduction.get("dependencyResponsibility"), "reduction dependency"),
            )
            if pair not in baseline_by_pair or pair in reduced:
                raise RelocationPlanError(f"invalid reverse dependency reduction: {pair}")
            before = _integer(reduction.get("from"), "reduction starting maximum")
            after = _integer(reduction.get("to"), "reduction ending maximum")
            if before != baseline_by_pair[pair] or after != 0:
                raise RelocationPlanError(f"reverse dependency reduction is stale: {pair}")
            remaining -= before - after
            reduced.add(pair)
        maximum = wave["maximumReverseDirectIncludesAfterWave"]
        if maximum != remaining:
            raise RelocationPlanError(
                "reverse dependency reduction does not match wave maximum: "
                f"{wave['id']} expected {remaining}, found {maximum}"
            )
    if reduced != set(baseline_by_pair) or remaining != 0:
        raise RelocationPlanError("reverse dependency reductions do not reach zero")


def _validate_internal_destinations(
    plan: dict[str, Any],
    waves: list[dict[str, Any]],
    packages: dict[str, dict[str, Any]],
    structural: dict[str, Any],
) -> None:
    baseline_sets = {
        _string(item.get("ownerTarget"), "internal header owner target"): item
        for raw in _array(
            structural.get("internalHeaderBaseline"), "internal header baseline"
        )
        for item in [_object(raw, "internal header set")]
    }
    baseline_headers = {
        (owner, _string(header.get("path"), f"{owner} internal header path")): len(
            _array(header.get("consumerPaths"), f"{owner} internal header consumers")
        )
        for owner, item in baseline_sets.items()
        for raw_header in _array(item.get("headers"), f"{owner} internal headers")
        for header in [_object(raw_header, f"{owner} internal header")]
    }
    destinations = [
        _object(item, "internal header destination")
        for item in _array(
            plan.get("reviewedInternalHeaderDestinations"),
            "reviewed internal header destinations",
        )
    ]
    covered: dict[tuple[str, str], str] = {}
    removed_by_wave: dict[str, dict[str, int]] = {
        wave["id"]: {owner: 0 for owner in baseline_sets} for wave in waves
    }
    for destination in destinations:
        _fields(destination, DESTINATION_FIELDS, "internal header destination")
        owner = _string(destination.get("ownerTarget"), "destination owner target")
        header_path = _string(destination.get("headerPath"), "destination header path")
        responsibility = _string(
            destination.get("destinationResponsibility"),
            "destination responsibility",
        )
        directory = _string(
            destination.get("destinationDirectory"), "destination directory"
        )
        wave_id = _string(destination.get("wave"), "destination wave")
        _string(destination.get("disposition"), "destination disposition")
        if owner not in baseline_sets or responsibility not in packages:
            raise RelocationPlanError("unknown internal header destination owner")
        if wave_id != packages[responsibility].get("migrationWave"):
            raise RelocationPlanError("internal header destination uses the wrong wave")
        if not _is_below(directory, packages[responsibility]["target"]["rootDirectory"]):
            raise RelocationPlanError("internal header destination is outside its package root")
        matches = (
            [key for key in baseline_headers if key[0] == owner]
            if header_path == "*"
            else [(owner, header_path)]
        )
        if not matches or any(key not in baseline_headers for key in matches):
            raise RelocationPlanError("internal header destination is absent from the baseline")
        for key in matches:
            if key in covered:
                raise RelocationPlanError("internal header destination is duplicated")
            covered[key] = wave_id
            removed_by_wave[wave_id][owner] += baseline_headers[key]
    if set(covered) != set(baseline_headers):
        raise RelocationPlanError(
            "internal header destinations do not cover the baseline"
        )

    remaining = {
        owner: _integer(item.get("maximumDirectReferences"), f"{owner} baseline maximum")
        for owner, item in baseline_sets.items()
    }
    if set(remaining) != {"kritaimage", "kritaui"}:
        raise RelocationPlanError("unexpected internal header baseline owners")
    for wave in waves:
        for owner, count in removed_by_wave[wave["id"]].items():
            remaining[owner] -= count
        if wave["maximumInternalDirectReferencesAfterWave"] != remaining:
            raise RelocationPlanError(
                f"internal header reduction does not match wave maximum: {wave['id']}"
            )
    if any(remaining.values()):
        raise RelocationPlanError("internal header reductions do not reach zero")


def validate_plan(
    plan: dict[str, Any], *, repository_root: Path = REPO_ROOT
) -> None:
    _fields(plan, ROOT_FIELDS, "package relocation plan")
    if plan.get("schemaVersion") != 1 or plan.get("scope") != PLAN_SCOPE:
        raise RelocationPlanError("unexpected package relocation plan identity")
    if plan.get("inputs") != PLAN_INPUTS:
        raise RelocationPlanError("package relocation plan inputs are stale")
    principles = _object(plan.get("principles"), "relocation principles")
    if not principles:
        raise RelocationPlanError("relocation principles must not be empty")
    for key, value in principles.items():
        _string(key, "relocation principle name")
        _string(value, f"relocation principle {key}")

    inputs = _repository_inputs(repository_root)
    current_targets = _current_targets(repository_root)
    packages, new_targets, _planned_targets = _validate_packages(
        plan, inputs, current_targets
    )
    waves, _order_by_id = _validate_waves(plan, packages, new_targets)
    _validate_reverse_reductions(
        waves, inputs["dependencyViolationBaseline"]
    )
    _validate_internal_destinations(
        plan, waves, packages, inputs["structuralDependencyBaseline"]
    )
    structural = inputs["structuralDependencyBaseline"]
    if any(
        item.get("status") != "disproved-by-direct-include-attribution"
        for item in _array(
            structural.get("projectionResolutions"), "projection resolutions"
        )
    ):
        raise RelocationPlanError("unresolved structural dependency projection")
    cycle_baseline = _object(
        structural.get("targetCycleBaseline"), "target cycle baseline"
    )
    if cycle_baseline.get("maximumComponents") != 0:
        raise RelocationPlanError("target cycle baseline must remain zero")
    if plan.get("finalState") != FINAL_STATE:
        raise RelocationPlanError("final relocation state must reduce every baseline to zero")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Validate the complete R1-G5 package relocation plan."
    )
    parser.add_argument(
        "--plan",
        type=Path,
        default=REPO_ROOT / "docs/architecture/package-relocation-plan.json",
    )
    args = parser.parse_args()
    try:
        plan = load_plan(args.plan)
        validate_plan(plan, repository_root=REPO_ROOT)
    except RelocationPlanError as error:
        print(f"package relocation plan check failed: {error}")
        return 1
    print(
        "package relocation plan verified: "
        f"{len(plan['packages'])} packages, "
        f"{len(plan['migrationWaves'])} waves, "
        f"first={plan['firstImplementationWave']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
