#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import check_package_responsibility_map as responsibility_contract


SCRIPT_DIRECTORY = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIRECTORY.parents[1]
DEFAULT_MAP = REPO_ROOT / "docs/architecture/package-responsibilities.json"
DEFAULT_GRAPH_DIRECTORY = REPO_ROOT / "docs/architecture"
DEFAULT_PUBLIC_SURFACE_INVENTORY = (
    REPO_ROOT / "docs/architecture/public-surface-inventory.json"
)
DEFAULT_UI_CLASS_INVENTORY = (
    REPO_ROOT / "docs/architecture/ui-class-responsibilities.json"
)
DEFAULT_UI_TOOL_CLASS_INVENTORY = (
    REPO_ROOT / "docs/architecture/ui-tool-class-responsibilities.json"
)


def render_map(responsibility_map: dict[str, object]) -> str:
    return json.dumps(responsibility_map, indent=2, ensure_ascii=False) + "\n"


def _argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Update the current package responsibility evidence from the "
            "recorded CMake graphs and public-surface inventories."
        )
    )
    parser.add_argument("--map", type=Path, default=DEFAULT_MAP)
    parser.add_argument(
        "--graph-directory", type=Path, default=DEFAULT_GRAPH_DIRECTORY
    )
    parser.add_argument(
        "--public-surface-inventory",
        type=Path,
        default=DEFAULT_PUBLIC_SURFACE_INVENTORY,
    )
    parser.add_argument(
        "--ui-class-inventory",
        type=Path,
        default=DEFAULT_UI_CLASS_INVENTORY,
    )
    parser.add_argument(
        "--ui-tool-class-inventory",
        type=Path,
        default=DEFAULT_UI_TOOL_CLASS_INVENTORY,
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="report whether the responsibility evidence is current",
    )
    return parser


def main(arguments: list[str] | None = None) -> int:
    options = _argument_parser().parse_args(arguments)
    try:
        current = responsibility_contract.load_map(options.map)
        rendered = render_map(
            responsibility_contract.updated_map(
                current,
                graph_directory=options.graph_directory,
                public_surface_inventory_path=(
                    options.public_surface_inventory
                ),
                ui_class_inventory_path=options.ui_class_inventory,
                ui_tool_class_inventory_path=(
                    options.ui_tool_class_inventory
                ),
            )
        )
        if options.check:
            if options.map.read_text(encoding="utf-8") != rendered:
                raise responsibility_contract.ResponsibilityMapError(
                    "package responsibility map is stale; run "
                    "scripts/architecture/"
                    "update_package_responsibility_map.py"
                )
        else:
            options.map.write_text(rendered, encoding="utf-8")
    except (OSError, responsibility_contract.ResponsibilityMapError) as error:
        print(f"update-package-responsibility-map: {error}", file=sys.stderr)
        return 1
    action = "verified" if options.check else "updated"
    print(f"package responsibility map {action}: {options.map}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
