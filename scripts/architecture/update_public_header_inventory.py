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
    return {
        "schemaVersion": 2,
        "scope": {
            "publicHeaders": "complete",
            "majorClasses": "representative",
            "plugins": "representative",
        },
        "platforms": list(inventory_contract.PLATFORMS),
        "publicHeaderPolicy": inventory_contract.public_header_policy(),
        "publicHeaderSets": inventory_contract.build_public_header_sets(REPO_ROOT),
        "publicHeaderDetails": header_details,
        "majorClasses": inventory.get("majorClasses"),
        "plugins": inventory.get("plugins"),
    }


def render_inventory(inventory: dict[str, Any]) -> str:
    return json.dumps(inventory, indent=2, ensure_ascii=False) + "\n"


def _argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Update the complete kritaimage and kritaui public-header sets from "
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
