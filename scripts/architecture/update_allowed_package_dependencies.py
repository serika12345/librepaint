#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import check_allowed_package_dependencies as dependency_contract


SCRIPT_DIRECTORY = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIRECTORY.parents[1]
DEFAULT_POLICY = (
    REPO_ROOT / "docs/architecture/allowed-package-dependencies.json"
)
DEFAULT_RESPONSIBILITY_MAP = (
    REPO_ROOT / "docs/architecture/package-responsibilities.json"
)


def render_policy(policy: dict[str, object]) -> str:
    return json.dumps(policy, indent=2, ensure_ascii=False) + "\n"


def _argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Update the current CMake target projection in the allowed "
            "package dependency policy."
        )
    )
    parser.add_argument("--policy", type=Path, default=DEFAULT_POLICY)
    parser.add_argument(
        "--responsibility-map",
        type=Path,
        default=DEFAULT_RESPONSIBILITY_MAP,
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="report whether the current target projection is current",
    )
    return parser


def main(arguments: list[str] | None = None) -> int:
    options = _argument_parser().parse_args(arguments)
    try:
        current = dependency_contract.load_policy(options.policy)
        responsibility_map = dependency_contract.load_responsibility_map(
            options.responsibility_map
        )
        rendered = render_policy(
            dependency_contract.updated_policy(
                current, responsibility_map=responsibility_map
            )
        )
        if options.check:
            if options.policy.read_text(encoding="utf-8") != rendered:
                raise dependency_contract.DependencyPolicyError(
                    "allowed package dependency policy is stale; run "
                    "scripts/architecture/"
                    "update_allowed_package_dependencies.py"
                )
        else:
            options.policy.write_text(rendered, encoding="utf-8")
    except (OSError, dependency_contract.DependencyPolicyError) as error:
        print(f"update-allowed-package-dependencies: {error}", file=sys.stderr)
        return 1
    action = "verified" if options.check else "updated"
    print(f"allowed package dependency policy {action}: {options.policy}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
