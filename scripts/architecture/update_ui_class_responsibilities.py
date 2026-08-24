#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any

import check_public_surface_inventory as inventory_contract


SCRIPT_DIRECTORY = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIRECTORY.parents[1]
DEFAULT_PUBLIC_SURFACE_INVENTORY = (
    REPO_ROOT / "docs/architecture/public-surface-inventory.json"
)
DEFAULT_CLASS_INVENTORY = (
    REPO_ROOT / "docs/architecture/ui-class-responsibilities.json"
)


def updated_inventory(
    inventory: dict[str, Any],
    public_surface_inventory: dict[str, Any],
) -> dict[str, Any]:
    assignments = {
        entry["name"]: entry["responsibilityArea"]
        for entry in inventory.get("classes", [])
        if isinstance(entry, dict)
        and isinstance(entry.get("name"), str)
        and isinstance(entry.get("responsibilityArea"), str)
    }
    discovered = inventory_contract.discover_ui_top_level_classes(
        repository_root=REPO_ROOT,
        public_surface_inventory=public_surface_inventory,
    )
    for entry in discovered:
        nested_area = (
            inventory_contract.UI_CLASS_NESTED_HEADER_RESPONSIBILITY_BY_PATH.get(
                entry["header"]
            )
        )
        if nested_area is not None:
            assignments.setdefault(entry["name"], nested_area)
    discovered_names = {entry["name"] for entry in discovered}
    missing = sorted(discovered_names - set(assignments))
    if missing:
        raise inventory_contract.PublicSurfaceError(
            "UI class responsibility assignments do not match discovery; "
            f"missing={missing}"
        )
    known_areas = {
        entry["id"] for entry in inventory_contract.UI_CLASS_RESPONSIBILITY_AREAS
    }
    assigned_discovered_areas = {
        assignments[name] for name in discovered_names
    }
    unknown_areas = sorted(assigned_discovered_areas - known_areas)
    if unknown_areas:
        raise inventory_contract.PublicSurfaceError(
            f"unknown UI class responsibility areas: {unknown_areas}"
        )
    return {
        "schemaVersion": 1,
        "scope": "libs/ui-root-and-classified-nested-public-classes",
        "ownerTarget": "kritaui",
        "platforms": list(inventory_contract.PLATFORMS),
        "classPolicy": inventory_contract.ui_class_policy(),
        "responsibilityAreas": list(
            inventory_contract.UI_CLASS_RESPONSIBILITY_AREAS
        ),
        "classes": [
            {**entry, "responsibilityArea": assignments[entry["name"]]}
            for entry in discovered
        ],
    }


def render_inventory(inventory: dict[str, Any]) -> str:
    return json.dumps(inventory, indent=2, ensure_ascii=False) + "\n"


def _argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Update structural evidence for the classified public classes in "
            "the top level of libs/ui."
        )
    )
    parser.add_argument(
        "--inventory",
        type=Path,
        default=DEFAULT_CLASS_INVENTORY,
    )
    parser.add_argument(
        "--public-surface-inventory",
        type=Path,
        default=DEFAULT_PUBLIC_SURFACE_INVENTORY,
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="report whether structural evidence already matches discovery",
    )
    return parser


def main(arguments: list[str] | None = None) -> int:
    options = _argument_parser().parse_args(arguments)
    try:
        current = inventory_contract.load_ui_class_inventory(options.inventory)
        public_surface = inventory_contract.load_inventory(
            options.public_surface_inventory
        )
        rendered = render_inventory(updated_inventory(current, public_surface))
        if options.check:
            if options.inventory.read_text(encoding="utf-8") != rendered:
                raise inventory_contract.PublicSurfaceError(
                    "UI class responsibility inventory is stale; run "
                    "scripts/architecture/update_ui_class_responsibilities.py"
                )
        else:
            options.inventory.write_text(rendered, encoding="utf-8")
    except (OSError, inventory_contract.PublicSurfaceError) as error:
        print(f"update-ui-class-responsibilities: {error}", file=sys.stderr)
        return 1
    action = "verified" if options.check else "updated"
    print(f"UI class responsibility inventory {action}: {options.inventory}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
