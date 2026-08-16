#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import check_structural_dependency_baseline as structural_contract


SCRIPT_DIRECTORY = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIRECTORY.parents[1]
DEFAULT_BASELINE = (
    REPO_ROOT / "docs/architecture/structural-dependency-baseline.json"
)


def render_baseline(baseline: dict[str, object]) -> str:
    return json.dumps(baseline, indent=2, ensure_ascii=False) + "\n"


def _argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Update shared-target resolutions, target cycles, and internal "
            "header evidence while preserving reviewed maxima."
        )
    )
    parser.add_argument("--baseline", type=Path, default=DEFAULT_BASELINE)
    parser.add_argument(
        "--check",
        action="store_true",
        help="report whether the structural baseline is current",
    )
    return parser


def main(arguments: list[str] | None = None) -> int:
    options = _argument_parser().parse_args(arguments)
    try:
        current = structural_contract.load_baseline(options.baseline)
        rendered = render_baseline(
            structural_contract.updated_baseline(
                current, repository_root=REPO_ROOT
            )
        )
        if options.check:
            if options.baseline.read_text(encoding="utf-8") != rendered:
                raise structural_contract.StructuralBaselineError(
                    "structural dependency baseline is stale; run "
                    "scripts/architecture/"
                    "update_structural_dependency_baseline.py"
                )
        else:
            options.baseline.write_text(rendered, encoding="utf-8")
    except (OSError, structural_contract.StructuralBaselineError) as error:
        print(f"update-structural-dependency-baseline: {error}", file=sys.stderr)
        return 1
    action = "verified" if options.check else "updated"
    print(f"structural dependency baseline {action}: {options.baseline}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
