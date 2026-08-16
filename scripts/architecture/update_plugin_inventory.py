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
DEFAULT_GRAPH_DIRECTORY = REPO_ROOT / "docs/architecture"


def updated_inventory(
    inventory: dict[str, Any], graph_directory: Path
) -> dict[str, Any]:
    return {
        "schemaVersion": 3,
        "scope": {
            "publicHeaders": "complete",
            "majorClasses": "representative",
            "plugins": "complete",
        },
        "platforms": list(inventory_contract.PLATFORMS),
        "publicHeaderPolicy": inventory.get("publicHeaderPolicy"),
        "publicHeaderSets": inventory.get("publicHeaderSets"),
        "publicHeaderDetails": inventory.get("publicHeaderDetails"),
        "majorClasses": inventory.get("majorClasses"),
        "pluginPolicy": inventory_contract.plugin_policy(),
        "pluginServiceTypeOwners": list(
            inventory_contract.PLUGIN_SERVICE_TYPE_OWNERS
        ),
        "plugins": inventory_contract.discover_plugins(
            repository_root=REPO_ROOT,
            graph_directory=graph_directory,
        ),
    }


def render_inventory(inventory: dict[str, Any]) -> str:
    return json.dumps(inventory, indent=2, ensure_ascii=False) + "\n"


def _argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Update the complete product plugin inventory from registration "
            "macros, sibling metadata, CMake ownership, and target graphs."
        )
    )
    parser.add_argument(
        "--inventory",
        type=Path,
        default=DEFAULT_INVENTORY,
    )
    parser.add_argument(
        "--graph-directory",
        type=Path,
        default=DEFAULT_GRAPH_DIRECTORY,
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="report whether the recorded plugins already match discovery",
    )
    return parser


def main(arguments: list[str] | None = None) -> int:
    options = _argument_parser().parse_args(arguments)
    try:
        current = inventory_contract.load_inventory(options.inventory)
        rendered = render_inventory(
            updated_inventory(current, options.graph_directory)
        )
        if options.check:
            if options.inventory.read_text(encoding="utf-8") != rendered:
                raise inventory_contract.PublicSurfaceError(
                    "plugin inventory is stale; run "
                    "scripts/architecture/update_plugin_inventory.py"
                )
        else:
            options.inventory.write_text(rendered, encoding="utf-8")
    except (OSError, inventory_contract.PublicSurfaceError) as error:
        print(f"update-plugin-inventory: {error}", file=sys.stderr)
        return 1
    action = "verified" if options.check else "updated"
    print(f"plugin inventory {action}: {options.inventory}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
