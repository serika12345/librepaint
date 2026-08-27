#!/usr/bin/env python3

from __future__ import annotations

import argparse
import importlib.util
import hashlib
import json
import re
import subprocess
import sys
from collections.abc import Iterable, Mapping
from pathlib import Path, PurePosixPath
from typing import Any


RECORD_KINDS = frozenset({"class", "struct", "union"})
ROUTINE_KINDS = frozenset({"function", "prototype", "slot"})
IGNORED_ROUTINE_NAMES = frozenset(
    {
        "Q_ENUM",
        "Q_ENUMS",
        "Q_FLAG",
        "Q_FLAGS",
        "Q_GADGET",
        "Q_OBJECT",
        "Q_PROPERTY",
    }
)
CTAGS_IGNORED_TOKENS = (
    "Q_ENUM",
    "Q_ENUMS",
    "Q_FLAG",
    "Q_FLAGS",
    "Q_GADGET",
    "Q_OBJECT",
    "Q_PROPERTY",
)
CONTRACT_CLASSIFICATIONS = frozenset(
    {"maintained", "known_defect", "open_design_question"}
)
REPO_ROOT = Path(__file__).resolve().parents[2]
REGISTRY_PATH = REPO_ROOT / "docs/architecture/public-api-test-contracts.json"


class PublicApiContractError(RuntimeError):
    """Raised when the public API behavior-contract registry is invalid."""


def _fingerprint(lines: Iterable[str]) -> str:
    digest = hashlib.sha256()
    for line in sorted(lines):
        digest.update(line.encode("utf-8"))
        digest.update(b"\n")
    return digest.hexdigest()


def fingerprint_public_headers(public_headers: Iterable[str]) -> str:
    return _fingerprint(set(public_headers))


def fingerprint_public_apis(apis: Iterable[Mapping[str, str]]) -> str:
    return _fingerprint(
        json.dumps(
            {
                "header": api["header"],
                "id": api["id"],
                "kind": api["kind"],
                "symbol": api["symbol"],
            },
            ensure_ascii=False,
            sort_keys=True,
            separators=(",", ":"),
        )
        for api in apis
    )


def _string(tag: Mapping[str, Any], key: str) -> str:
    value = tag.get(key)
    return value if isinstance(value, str) else ""


def _qualified_name(tag: Mapping[str, Any]) -> str:
    name = _string(tag, "name")
    scope = _string(tag, "scope")
    return f"{scope}::{name}" if scope else name


def _type_name(tag: Mapping[str, Any]) -> str:
    value = _string(tag, "typeref")
    return value.split(":", 1)[1] if ":" in value else value


def _enum_access(tags: Iterable[Mapping[str, Any]]) -> dict[str, str]:
    return {
        _qualified_name(tag): _string(tag, "access")
        for tag in tags
        if tag.get("kind") == "enum"
    }


def _canonical_kind(tag: Mapping[str, Any]) -> str | None:
    kind = _string(tag, "kind")
    if kind in RECORD_KINDS or kind in {"enum", "enumerator", "member"}:
        return kind
    if kind in ROUTINE_KINDS:
        return "method" if _string(tag, "scopeKind") in RECORD_KINDS else "function"
    if kind == "typedef":
        return "alias"
    if kind in {"variable", "externvar"}:
        return "variable"
    if kind == "namespaceAlias":
        return "namespace_alias"
    return None


def extract_public_apis(
    tags: Iterable[Mapping[str, Any]], public_headers: set[str]
) -> list[dict[str, str]]:
    tag_list = list(tags)
    enum_access = _enum_access(tag_list)
    apis: dict[str, dict[str, str]] = {}
    for tag in tag_list:
        header = _string(tag, "path")
        kind = _canonical_kind(tag)
        name = _string(tag, "name")
        access = _string(tag, "access")
        if header not in public_headers or kind is None or not name:
            continue
        if access in {"private", "protected"}:
            continue
        if kind in RECORD_KINDS and "end" not in tag:
            continue
        if kind == "enumerator" and enum_access.get(_string(tag, "scope")) in {
            "private",
            "protected",
        }:
            continue
        if kind in {"method", "function"} and name in IGNORED_ROUTINE_NAMES:
            continue

        qualified = _qualified_name(tag)
        signature = _string(tag, "signature") if kind in {"method", "function"} else ""
        type_name = _type_name(tag)
        identifier = f"{kind}:{qualified}{signature}"
        symbol = f"{qualified}{signature}"
        if type_name:
            symbol += f": {type_name}" if kind in {"alias", "member", "variable"} else f" -> {type_name}"
        apis.setdefault(
            identifier,
            {
                "header": header,
                "id": identifier,
                "kind": kind,
                "symbol": symbol,
            },
        )

    return [apis[identifier] for identifier in sorted(apis)]


def _load_public_contracts_module() -> Any:
    path = Path(__file__).with_name("check_public_contracts.py")
    spec = importlib.util.spec_from_file_location("check_public_contracts", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot import {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def discover_public_headers(repository_root: Path) -> list[str]:
    module = _load_public_contracts_module()
    return module.discover_public_headers(repository_root)


def collect_ctags(
    repository_root: Path, public_headers: list[str]
) -> list[dict[str, Any]]:
    command = [
        "ctags",
        "--options=NONE",
        "--language-force=C++",
        "--output-format=json",
        "--excmd=number",
        "--fields=+{access}{signature}{typeref}{scope}{end}",
        "--kinds-C++=+p+x+A",
        "--extras=-F",
        "-I",
        ",".join(CTAGS_IGNORED_TOKENS),
        "-o",
        "-",
        *public_headers,
    ]
    completed = subprocess.run(
        command,
        cwd=repository_root,
        check=True,
        text=True,
        capture_output=True,
    )
    tags: list[dict[str, Any]] = []
    for line_number, line in enumerate(completed.stdout.splitlines(), start=1):
        if not line:
            continue
        try:
            value = json.loads(line)
        except json.JSONDecodeError as error:
            raise RuntimeError(
                f"invalid ctags JSON at output line {line_number}"
            ) from error
        if isinstance(value, dict) and value.get("_type") == "tag":
            tags.append(value)
    return tags


def collect_public_apis(
    repository_root: Path, public_headers: list[str] | None = None
) -> list[dict[str, str]]:
    if public_headers is None:
        public_headers = discover_public_headers(repository_root)
    tags = collect_ctags(repository_root, public_headers)
    return extract_public_apis(tags, set(public_headers))


def _cmake_owns_test(
    definitions: Iterable[str], target: str, source_name: str
) -> bool:
    target_pattern = re.compile(
        rf"(?<![A-Za-z0-9_]){re.escape(target)}(?![A-Za-z0-9_])"
    )
    source_pattern = re.compile(
        rf"(?<![A-Za-z0-9_]){re.escape(source_name)}(?![A-Za-z0-9_])"
    )
    return any(
        target_pattern.search(text) and source_pattern.search(text)
        for text in definitions
    )


def validate_contracts(
    repository_root: Path,
    apis: Iterable[Mapping[str, str]],
    contracts: Iterable[Mapping[str, Any]],
) -> set[str]:
    api_ids = {api["id"] for api in apis}
    covered: set[str] = set()
    cmake_definitions = [
        path.read_text(encoding="utf-8")
        for path in repository_root.rglob("CMakeLists.txt")
        if path.is_file()
    ]
    for index, contract in enumerate(contracts):
        prefix = f"contract {index}"
        values: dict[str, str] = {}
        for field in ("target", "source", "test", "behavior", "classification"):
            value = contract.get(field)
            if not isinstance(value, str) or not value.strip():
                raise PublicApiContractError(f"{prefix} has invalid {field}")
            values[field] = value
        if values["classification"] not in CONTRACT_CLASSIFICATIONS:
            raise PublicApiContractError(
                f"{prefix} has invalid classification: {values['classification']}"
            )

        source_path = PurePosixPath(values["source"])
        if source_path.is_absolute() or ".." in source_path.parts:
            raise PublicApiContractError(f"{prefix} has invalid source path")
        source = repository_root / source_path
        if not source.is_file():
            raise PublicApiContractError(
                f"{prefix} test source does not exist: {values['source']}"
            )
        if not _cmake_owns_test(
            cmake_definitions, values["target"], source_path.name
        ):
            raise PublicApiContractError(
                f"{prefix} target does not own its test source: "
                f"{values['target']} -> {values['source']}"
            )
        test_pattern = re.compile(
            rf"\b{re.escape(values['test'])}\s*\("
        )
        if not test_pattern.search(source.read_text(encoding="utf-8")):
            raise PublicApiContractError(
                f"{prefix} test function does not exist: {values['test']}"
            )

        declared_apis = contract.get("apis")
        if (
            not isinstance(declared_apis, list)
            or not declared_apis
            or not all(isinstance(api, str) and api for api in declared_apis)
        ):
            raise PublicApiContractError(f"{prefix} has invalid apis")
        if declared_apis != sorted(set(declared_apis)):
            raise PublicApiContractError(
                f"{prefix} apis must be unique and sorted"
            )
        unknown = sorted(set(declared_apis) - api_ids)
        if unknown:
            raise PublicApiContractError(
                f"{prefix} references unknown public APIs: {unknown}"
            )
        covered.update(declared_apis)
    return covered


def validate_registry(
    repository_root: Path,
    public_headers: list[str],
    apis: list[dict[str, str]],
    registry: Mapping[str, Any],
) -> dict[str, Any]:
    if registry.get("schemaVersion") != 1:
        raise PublicApiContractError("unsupported registry schemaVersion")
    scope = registry.get("scope")
    if not isinstance(scope, Mapping):
        raise PublicApiContractError("registry scope must be an object")
    expected_scope = {
        "publicHeaderCount": len(public_headers),
        "publicHeaderFingerprint": fingerprint_public_headers(public_headers),
        "publicApiCount": len(apis),
        "publicApiFingerprint": fingerprint_public_apis(apis),
    }
    if any(scope.get(key) != value for key, value in expected_scope.items()):
        raise PublicApiContractError(
            "public API inventory changed; review the declarations and "
            "synchronize the contract scope"
        )

    contracts = registry.get("contracts")
    if not isinstance(contracts, list):
        raise PublicApiContractError("registry contracts must be an array")
    covered_ids = validate_contracts(repository_root, apis, contracts)
    missing_count = len(apis) - len(covered_ids)

    migration = registry.get("migration")
    if not isinstance(migration, Mapping):
        raise PublicApiContractError("registry migration must be an object")
    expected_missing = migration.get("expectedMissingApis")
    if not isinstance(expected_missing, int) or expected_missing < 0:
        raise PublicApiContractError(
            "registry expectedMissingApis must be a non-negative integer"
        )
    if missing_count != expected_missing:
        raise PublicApiContractError(
            "public API missing-contract count changed: "
            f"expected {expected_missing}, actual {missing_count}"
        )
    return {
        "publicHeaders": len(public_headers),
        "publicApis": len(apis),
        "covered": len(covered_ids),
        "missing": missing_count,
        "missingApis": [api for api in apis if api["id"] not in covered_ids],
    }


def _load_registry(path: Path) -> Mapping[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as error:
        raise PublicApiContractError(
            f"public API contract registry does not exist: {path}"
        ) from error
    except json.JSONDecodeError as error:
        raise PublicApiContractError(
            f"invalid public API contract registry: line {error.lineno}, "
            f"column {error.colno}"
        ) from error
    if not isinstance(value, Mapping):
        raise PublicApiContractError(
            "public API contract registry must be an object"
        )
    return value


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Verify explicit behavior contracts for public C++ APIs."
    )
    parser.add_argument(
        "--report",
        type=Path,
        help="write the complete missing-API work queue as JSON",
    )
    arguments = parser.parse_args(argv)
    try:
        public_headers = discover_public_headers(REPO_ROOT)
        apis = collect_public_apis(REPO_ROOT, public_headers)
        result = validate_registry(
            REPO_ROOT,
            public_headers,
            apis,
            _load_registry(REGISTRY_PATH),
        )
        if arguments.report is not None:
            report = {
                "schemaVersion": 1,
                "publicHeaders": result["publicHeaders"],
                "publicApis": result["publicApis"],
                "coveredApis": result["covered"],
                "missingApis": result["missingApis"],
            }
            arguments.report.parent.mkdir(parents=True, exist_ok=True)
            arguments.report.write_text(
                json.dumps(report, ensure_ascii=False, indent=2) + "\n",
                encoding="utf-8",
            )
    except (OSError, subprocess.SubprocessError, PublicApiContractError) as error:
        print(f"check-public-api-contracts: {error}", file=sys.stderr)
        return 1
    print(
        "public API behavior contracts verified: "
        f"{result['covered']}/{result['publicApis']} covered, "
        f"{result['missing']} migration entries remain across "
        f"{result['publicHeaders']} public headers"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
