#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any


SCRIPT_DIRECTORY = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIRECTORY.parents[1]
DEFAULT_ASSESSMENT = REPO_ROOT / "docs/architecture/document-boundary-assessment.json"

EXPECTED_SCOPE = "r1-g6e-p4-document-boundary-evaluation"
EXPECTED_DISPOSITIONS = [
    "move-as-unit",
    "retain-until-prerequisite",
    "split-by-responsibility",
]
METHOD_PATTERN = re.compile(
    r"KisDocument::((?:Private::)?(?:~?KisDocument|[A-Za-z_]\w*))\s*\("
)


class DocumentBoundaryError(RuntimeError):
    pass


def _load_json(path: Path, description: str) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise DocumentBoundaryError(f"cannot read {description}: {error}") from error
    if not isinstance(value, dict):
        raise DocumentBoundaryError(f"{description} must be an object")
    return value


def load_assessment(path: Path) -> dict[str, Any]:
    return _load_json(path, "document boundary assessment")


def _require_fields(value: dict[str, Any], fields: set[str], description: str) -> None:
    actual = set(value)
    if actual != fields:
        raise DocumentBoundaryError(
            f"{description} fields differ: missing={sorted(fields - actual)}, "
            f"extra={sorted(actual - fields)}"
        )


def _object(value: Any, description: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise DocumentBoundaryError(f"{description} must be an object")
    return value


def _array(value: Any, description: str) -> list[Any]:
    if not isinstance(value, list):
        raise DocumentBoundaryError(f"{description} must be an array")
    return value


def _string(value: Any, description: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise DocumentBoundaryError(f"{description} must be non-empty text")
    return value


def _strings(value: Any, description: str, *, allow_empty: bool = False) -> list[str]:
    result = [_string(item, f"{description} item") for item in _array(value, description)]
    if not allow_empty and not result:
        raise DocumentBoundaryError(f"{description} must not be empty")
    if result != sorted(set(result)):
        raise DocumentBoundaryError(f"{description} must be sorted and unique")
    return result


def _repository_path(
    repository_root: Path,
    value: Any,
    description: str,
    *,
    require_file: bool,
) -> str:
    relative = _string(value, description)
    path = Path(relative)
    if path.is_absolute() or ".." in path.parts:
        raise DocumentBoundaryError(f"{description} must be repository-relative")
    resolved = repository_root / path
    exists = resolved.is_file() if require_file else resolved.exists()
    if not exists:
        kind = "file" if require_file else "path"
        raise DocumentBoundaryError(f"{description} requires an existing {kind}: {relative}")
    return path.as_posix()


def _migration_contract(
    repository_root: Path,
    path: str,
) -> tuple[set[str], dict[str, dict[str, str]]]:
    plan = _load_json(repository_root / path, "package relocation plan")
    waves = {
        _string(_object(item, "migration wave").get("id"), "migration wave id")
        for item in _array(plan.get("migrationWaves"), "migration waves")
    }
    targets: dict[str, dict[str, str]] = {}
    for raw_package in _array(plan.get("packages"), "relocation packages"):
        package = _object(raw_package, "relocation package")
        responsibility = _string(package.get("responsibility"), "package responsibility")
        target = _object(package.get("target"), f"target for {responsibility}")
        targets[responsibility] = {
            _string(entry.get("name"), f"target name for {responsibility}"): _string(
                entry.get("directory"), f"target directory for {responsibility}"
            )
            for entry in (
                _object(item, f"target for {responsibility}")
                for item in _array(target.get("cmakeTargets"), f"targets for {responsibility}")
            )
        }
    return waves, targets


def _document_classes(repository_root: Path, inventory_path: str) -> set[str]:
    inventory = _load_json(repository_root / inventory_path, "UI class inventory")
    return {
        _string(entry.get("name"), "document class name")
        for entry in (
            _object(item, "UI class")
            for item in _array(inventory.get("classes"), "UI classes")
        )
        if entry.get("responsibilityArea") == "document-state"
    }


def discover_kis_document_methods(path: Path) -> set[str]:
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError as error:
        raise DocumentBoundaryError(f"cannot read KisDocument implementation: {error}") from error
    return {
        match.group(1)
        for line in lines
        if (match := METHOD_PATTERN.search(line)) is not None
    }


def _validate_destinations(
    assessment: dict[str, Any],
    *,
    targets: dict[str, dict[str, str]],
) -> set[str]:
    destinations = [
        _object(item, f"destination {index}")
        for index, item in enumerate(_array(assessment.get("destinations"), "destinations"))
    ]
    ids = [_string(item.get("id"), "destination id") for item in destinations]
    if ids != sorted(set(ids)):
        raise DocumentBoundaryError("destinations must be sorted and unique")
    for destination in destinations:
        destination_id = destination["id"]
        _require_fields(
            destination,
            {"id", "responsibility", "directory", "target", "ownership"},
            f"destination {destination_id}",
        )
        responsibility = _string(
            destination.get("responsibility"),
            f"responsibility for destination {destination_id}",
        )
        if responsibility not in targets:
            raise DocumentBoundaryError(
                f"destination {destination_id} has unknown responsibility {responsibility}"
            )
        target = _string(destination.get("target"), f"target for {destination_id}")
        if target not in targets[responsibility]:
            raise DocumentBoundaryError(
                f"destination {destination_id} target {target} is not planned for {responsibility}"
            )
        directory = _string(destination.get("directory"), f"directory for {destination_id}")
        target_directory = targets[responsibility][target]
        if directory != target_directory and not directory.startswith(f"{target_directory}/"):
            raise DocumentBoundaryError(
                f"destination {destination_id} directory {directory} is outside {target_directory}"
            )
        _string(destination.get("ownership"), f"ownership for {destination_id}")
    return set(ids)


def _validate_groups(
    raw_groups: Any,
    *,
    value_field: str,
    description: str,
    dispositions: set[str],
    destination_ids: set[str],
    waves: set[str],
) -> list[str]:
    groups = [
        _object(item, f"{description} {index}")
        for index, item in enumerate(_array(raw_groups, description))
    ]
    group_ids = [_string(group.get("id"), f"{description} id") for group in groups]
    if group_ids != sorted(set(group_ids)):
        raise DocumentBoundaryError(f"{description} must be sorted and unique")
    assigned: list[str] = []
    for group in groups:
        group_id = group["id"]
        _require_fields(
            group,
            {
                "id",
                value_field,
                "concern",
                "disposition",
                "destinationIds",
                "nextGate",
                "evidence",
            },
            f"{description} {group_id}",
        )
        assigned.extend(_strings(group.get(value_field), f"{value_field} for {group_id}"))
        disposition = _string(group.get("disposition"), f"disposition for {group_id}")
        if disposition not in dispositions:
            raise DocumentBoundaryError(
                f"{description} {group_id} has unknown disposition {disposition}"
            )
        recorded_destinations = _strings(
            group.get("destinationIds"), f"destination ids for {group_id}"
        )
        unknown_destinations = set(recorded_destinations) - destination_ids
        if unknown_destinations:
            raise DocumentBoundaryError(
                f"{description} {group_id} has unknown destinations {sorted(unknown_destinations)}"
            )
        gate = _string(group.get("nextGate"), f"next gate for {group_id}")
        if gate not in waves:
            raise DocumentBoundaryError(
                f"{description} {group_id} has unknown next gate {gate}"
            )
        _string(group.get("concern"), f"concern for {group_id}")
        _strings(group.get("evidence"), f"evidence for {group_id}")
    return assigned


def validate_assessment(assessment: dict[str, Any], *, repository_root: Path) -> None:
    _require_fields(
        assessment,
        {
            "schemaVersion",
            "scope",
            "sources",
            "dispositionKinds",
            "destinations",
            "classGroups",
            "kisDocumentMethodGroups",
            "abstractionAssessment",
            "nextAction",
        },
        "document boundary assessment",
    )
    if assessment.get("schemaVersion") != 1:
        raise DocumentBoundaryError("document boundary assessment schemaVersion must be 1")
    if assessment.get("scope") != EXPECTED_SCOPE:
        raise DocumentBoundaryError("document boundary assessment has an invalid scope")

    sources = _object(assessment.get("sources"), "assessment sources")
    _require_fields(
        sources,
        {
            "uiClassInventory",
            "kisDocumentHeader",
            "kisDocumentImplementation",
            "packageRelocationPlan",
            "startingDocumentClassCount",
            "previouslyResolvedClasses",
        },
        "assessment sources",
    )
    ui_inventory_path = _repository_path(
        repository_root, sources.get("uiClassInventory"), "UI class inventory", require_file=True
    )
    _repository_path(
        repository_root, sources.get("kisDocumentHeader"), "KisDocument header", require_file=True
    )
    implementation_path = _repository_path(
        repository_root,
        sources.get("kisDocumentImplementation"),
        "KisDocument implementation",
        require_file=True,
    )
    relocation_path = _repository_path(
        repository_root,
        sources.get("packageRelocationPlan"),
        "package relocation plan",
        require_file=True,
    )
    waves, targets = _migration_contract(repository_root, relocation_path)

    dispositions = _strings(assessment.get("dispositionKinds"), "disposition kinds")
    if dispositions != EXPECTED_DISPOSITIONS:
        raise DocumentBoundaryError(f"disposition kinds must be {EXPECTED_DISPOSITIONS}")
    destination_ids = _validate_destinations(assessment, targets=targets)

    assessed_classes = _validate_groups(
        assessment.get("classGroups"),
        value_field="classes",
        description="document class groups",
        dispositions=set(dispositions),
        destination_ids=destination_ids,
        waves=waves,
    )
    if len(assessed_classes) != len(set(assessed_classes)):
        raise DocumentBoundaryError("document classes must be assigned exactly once")
    expected_classes = _document_classes(repository_root, ui_inventory_path)
    if set(assessed_classes) != expected_classes:
        raise DocumentBoundaryError(
            "document class assessments do not match the current document-state classes; "
            f"missing={sorted(expected_classes - set(assessed_classes))}, "
            f"extra={sorted(set(assessed_classes) - expected_classes)}"
        )

    previous = [
        _object(item, f"previously resolved class {index}")
        for index, item in enumerate(
            _array(sources.get("previouslyResolvedClasses"), "previously resolved classes")
        )
    ]
    previous_names: list[str] = []
    for index, entry in enumerate(previous):
        _require_fields(entry, {"name", "destination", "gate"}, f"previously resolved class {index}")
        previous_names.append(_string(entry.get("name"), f"previous class name {index}"))
        _string(entry.get("destination"), f"previous destination {index}")
        _string(entry.get("gate"), f"previous gate {index}")
    if previous_names != sorted(set(previous_names)):
        raise DocumentBoundaryError("previously resolved classes must be sorted and unique")
    starting_count = sources.get("startingDocumentClassCount")
    if not isinstance(starting_count, int) or starting_count <= 0:
        raise DocumentBoundaryError("starting document class count must be positive")
    if starting_count != len(previous_names) + len(assessed_classes):
        raise DocumentBoundaryError(
            "starting document class count must equal resolved and remaining classes"
        )

    assessed_methods = _validate_groups(
        assessment.get("kisDocumentMethodGroups"),
        value_field="methods",
        description="KisDocument method groups",
        dispositions=set(dispositions),
        destination_ids=destination_ids,
        waves=waves,
    )
    if len(assessed_methods) != len(set(assessed_methods)):
        raise DocumentBoundaryError("KisDocument methods must be assigned exactly once")
    discovered_methods = discover_kis_document_methods(repository_root / implementation_path)
    if set(assessed_methods) != discovered_methods:
        raise DocumentBoundaryError(
            "KisDocument method assessments do not match current definitions; "
            f"missing={sorted(discovered_methods - set(assessed_methods))}, "
            f"extra={sorted(set(assessed_methods) - discovered_methods)}"
        )

    abstraction_entries = [
        _object(item, f"abstraction assessment {index}")
        for index, item in enumerate(
            _array(assessment.get("abstractionAssessment"), "abstraction assessment")
        )
    ]
    concerns: list[str] = []
    for index, entry in enumerate(abstraction_entries):
        _require_fields(
            entry,
            {"concern", "decision", "currentRequirement", "evidence", "nextGate"},
            f"abstraction assessment {index}",
        )
        concern = _string(entry.get("concern"), f"abstraction concern {index}")
        concerns.append(concern)
        decision = _string(entry.get("decision"), f"abstraction decision for {concern}")
        current_requirement = entry.get("currentRequirement")
        if decision not in {"do-not-introduce", "introduce"}:
            raise DocumentBoundaryError(f"unknown abstraction decision for {concern}: {decision}")
        if not isinstance(current_requirement, bool):
            raise DocumentBoundaryError(f"current requirement for {concern} must be boolean")
        if decision == "introduce" and not current_requirement:
            raise DocumentBoundaryError("introduced abstraction requires a current requirement")
        gate = _string(entry.get("nextGate"), f"abstraction next gate for {concern}")
        if gate not in waves:
            raise DocumentBoundaryError(f"abstraction assessment for {concern} has unknown gate {gate}")
        _strings(entry.get("evidence"), f"abstraction evidence for {concern}")
    if concerns != sorted(set(concerns)):
        raise DocumentBoundaryError("abstraction assessments must be sorted and unique")

    next_action = _object(assessment.get("nextAction"), "next action")
    _require_fields(
        next_action,
        {"gate", "purpose", "startingPaths", "prerequisites", "completionCriteria", "stopCondition"},
        "next action",
    )
    next_gate = _string(next_action.get("gate"), "next action gate")
    if next_gate not in waves:
        raise DocumentBoundaryError(f"next action has unknown gate {next_gate}")
    _string(next_action.get("purpose"), "next action purpose")
    for path in _strings(next_action.get("startingPaths"), "next action starting paths"):
        _repository_path(repository_root, path, "next action starting path", require_file=False)
    _strings(next_action.get("prerequisites"), "next action prerequisites")
    _strings(next_action.get("completionCriteria"), "next action completion criteria")
    _string(next_action.get("stopCondition"), "next action stop condition")


def _argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Validate the R1-G6e-P4 document boundary assessment."
    )
    parser.add_argument("--assessment", type=Path, default=DEFAULT_ASSESSMENT)
    return parser


def main(arguments: list[str] | None = None) -> int:
    options = _argument_parser().parse_args(arguments)
    try:
        assessment = load_assessment(options.assessment)
        validate_assessment(assessment, repository_root=REPO_ROOT)
    except DocumentBoundaryError as error:
        print(f"check-document-boundary-assessment: {error}", file=sys.stderr)
        return 1
    print(f"document boundary assessment verified: {options.assessment}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
