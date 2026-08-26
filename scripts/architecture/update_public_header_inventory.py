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
DEFAULT_INVENTORY = REPO_ROOT / "docs/architecture/public-surface-inventory.json"


def updated_inventory(inventory: dict[str, Any]) -> dict[str, Any]:
    header_details = inventory.get(
        "publicHeaderDetails", inventory.get("publicHeaders")
    )
    if header_details is None:
        raise inventory_contract.PublicSurfaceError(
            "public-surface inventory has no detailed public header records"
        )
    public_header_sets = inventory_contract.build_public_header_sets(REPO_ROOT)
    owner_by_path = {
        header["path"]: {
            "ownerTarget": header_set["ownerTarget"],
            "exportMacro": header_set["exportMacro"],
            "platforms": header_set["platforms"],
        }
        for header_set in public_header_sets
        for header in header_set["headers"]
    }

    def synchronize_owner(entry: dict[str, Any], path_field: str) -> dict[str, Any]:
        path = entry.get(path_field)
        owner = owner_by_path.get(path)
        if owner is None:
            raise inventory_contract.PublicSurfaceError(
                f"recorded public header is absent from complete discovery: {path}"
            )
        return {**entry, **owner}

    scope = inventory.get("scope", {})
    return {
        "schemaVersion": 3,
        "scope": {
            "publicHeaders": "complete",
            "majorClasses": "representative",
            "plugins": scope.get("plugins", "representative"),
        },
        "platforms": list(inventory_contract.PLATFORMS),
        "publicHeaderPolicy": inventory_contract.public_header_policy(),
        "publicHeaderSets": public_header_sets,
        "publicHeaderDetails": [
            synchronize_owner(entry, "path") for entry in header_details
        ],
        "majorClasses": [
            synchronize_owner(entry, "header")
            for entry in inventory.get("majorClasses", [])
        ],
        "pluginPolicy": inventory_contract.plugin_policy(),
        "pluginServiceTypeOwners": list(
            inventory_contract.PLUGIN_SERVICE_TYPE_OWNERS
        ),
        "plugins": inventory.get("plugins"),
    }


def render_inventory(inventory: dict[str, Any]) -> str:
    return json.dumps(inventory, indent=2, ensure_ascii=False) + "\n"


def _argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Update the complete image, import-export, and UI public-header sets from "
            "declared export macros and external production includes."
        )
    )
    parser.add_argument(
        "--inventory",
        type=Path,
        default=DEFAULT_INVENTORY,
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="report whether the recorded inventory already matches discovery",
    )
    return parser


def main(arguments: list[str] | None = None) -> int:
    options = _argument_parser().parse_args(arguments)
    try:
        current = inventory_contract.load_inventory(options.inventory)
        rendered = render_inventory(updated_inventory(current))
        if options.check:
            if options.inventory.read_text(encoding="utf-8") != rendered:
                raise inventory_contract.PublicSurfaceError(
                    "public header sets are stale; run "
                    "scripts/architecture/update_public_header_inventory.py"
                )
        else:
            options.inventory.write_text(rendered, encoding="utf-8")
    except (OSError, inventory_contract.PublicSurfaceError) as error:
        print(f"update-public-header-inventory: {error}", file=sys.stderr)
        return 1
    action = "verified" if options.check else "updated"
    print(f"public header inventory {action}: {options.inventory}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
