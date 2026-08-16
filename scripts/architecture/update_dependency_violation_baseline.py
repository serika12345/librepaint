#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import check_dependency_violation_baseline as baseline_contract


SCRIPT_DIRECTORY = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIRECTORY.parents[1]


def render_baseline(baseline: dict[str, object]) -> str:
    return json.dumps(baseline, indent=2, ensure_ascii=False) + "\n"


def _argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Update confirmed direct-include and unresolved projection evidence "
            "without changing reviewed maximum scopes."
        )
    )
    parser.add_argument(
        "--baseline",
        type=Path,
        default=(
            REPO_ROOT / "docs/architecture/dependency-violation-baseline.json"
        ),
    )
    parser.add_argument(
        "--policy",
        type=Path,
        default=(
            REPO_ROOT / "docs/architecture/allowed-package-dependencies.json"
        ),
    )
    parser.add_argument(
        "--responsibility-map",
        type=Path,
        default=(
            REPO_ROOT / "docs/architecture/package-responsibilities.json"
        ),
    )
    parser.add_argument(
        "--ui-class-inventory",
        type=Path,
        default=(
            REPO_ROOT / "docs/architecture/ui-class-responsibilities.json"
        ),
    )
    parser.add_argument(
        "--ui-tool-class-inventory",
        type=Path,
        default=(
            REPO_ROOT / "docs/architecture/ui-tool-class-responsibilities.json"
        ),
    )
    parser.add_argument(
        "--graph-directory",
        type=Path,
        default=REPO_ROOT / "docs/architecture",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="report whether the discovered baseline evidence is current",
    )
    return parser


def main(arguments: list[str] | None = None) -> int:
    options = _argument_parser().parse_args(arguments)
    try:
        current = baseline_contract.load_baseline(options.baseline)
        updated = baseline_contract.updated_baseline(
            current,
            repository_root=REPO_ROOT,
            policy=baseline_contract.load_input(
                options.policy, "allowed dependency policy"
            ),
            responsibility_map=baseline_contract.load_input(
                options.responsibility_map, "package responsibility map"
            ),
            ui_class_inventory=baseline_contract.load_input(
                options.ui_class_inventory, "UI class inventory"
            ),
            ui_tool_class_inventory=baseline_contract.load_input(
                options.ui_tool_class_inventory, "UI tool class inventory"
            ),
            graph_directory=options.graph_directory,
        )
        rendered = render_baseline(updated)
        if options.check:
            if options.baseline.read_text(encoding="utf-8") != rendered:
                raise baseline_contract.DependencyBaselineError(
                    "dependency violation baseline is stale; run "
                    "scripts/architecture/"
                    "update_dependency_violation_baseline.py"
                )
        else:
            options.baseline.write_text(rendered, encoding="utf-8")
    except (OSError, baseline_contract.DependencyBaselineError) as error:
        print(f"update-dependency-violation-baseline: {error}", file=sys.stderr)
        return 1
    action = "verified" if options.check else "updated"
    print(f"dependency violation baseline {action}: {options.baseline}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
