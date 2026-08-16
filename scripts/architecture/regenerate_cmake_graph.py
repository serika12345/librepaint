#!/usr/bin/env python3

from __future__ import annotations

import argparse
import subprocess
import sys
import tempfile
from pathlib import Path


SCRIPT_DIRECTORY = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIRECTORY.parents[1]
if str(SCRIPT_DIRECTORY) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIRECTORY))

import extract_cmake_graph  # noqa: E402


PLATFORM_PRESETS = {
    "macos": "tdd-macos",
    "linux": "tdd-linux",
}


class RegenerationError(RuntimeError):
    """Raised when the graph cannot be regenerated or verified."""


def prepare_query(build_directory: Path) -> Path:
    query_path = build_directory / ".cmake/api/v1/query/codemodel-v2"
    query_path.parent.mkdir(parents=True, exist_ok=True)
    query_path.write_bytes(b"")
    return query_path


def _write_atomically(output_path: Path, content: str) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    temporary_path: Path | None = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="w",
            encoding="utf-8",
            dir=output_path.parent,
            prefix=f".{output_path.name}.",
            delete=False,
        ) as temporary_file:
            temporary_file.write(content)
            temporary_path = Path(temporary_file.name)
        temporary_path.replace(output_path)
    finally:
        if temporary_path is not None:
            temporary_path.unlink(missing_ok=True)


def regenerate(
    *,
    platform: str,
    reply_directory: Path | None = None,
    output_path: Path | None = None,
    configuration_name: str | None = None,
    check: bool = False,
) -> Path:
    try:
        preset = PLATFORM_PRESETS[platform]
    except KeyError as error:
        supported = ", ".join(sorted(PLATFORM_PRESETS))
        raise RegenerationError(
            f"unsupported platform {platform!r}; expected one of: {supported}"
        ) from error

    if reply_directory is None:
        build_directory = REPO_ROOT / "build" / preset
        prepare_query(build_directory)
        result = subprocess.run(
            [
                str(REPO_ROOT / "scripts/build-incremental"),
                platform,
                "configure",
            ],
            cwd=REPO_ROOT,
            check=False,
        )
        if result.returncode != 0:
            raise RegenerationError(
                f"CMake configuration failed for {platform} with exit code "
                f"{result.returncode}"
            )
        reply_directory = build_directory / ".cmake/api/v1/reply"

    graph = extract_cmake_graph.extract_graph(
        reply_directory,
        platform=platform,
        preset=preset,
        configuration_name=configuration_name,
    )
    content = extract_cmake_graph.serialize_graph(graph)
    if output_path is None:
        output_path = (
            REPO_ROOT / "docs/architecture" / f"cmake-targets-{platform}.json"
        )

    if check:
        try:
            recorded_content = output_path.read_text(encoding="utf-8")
        except FileNotFoundError as error:
            raise RegenerationError(f"recorded graph does not exist: {output_path}") from error
        if recorded_content != content:
            raise RegenerationError(
                f"recorded graph is stale: {output_path}; run "
                f"scripts/architecture/regenerate_cmake_graph.py {platform}"
            )
    else:
        _write_atomically(output_path, content)
    return output_path


def _argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Configure a native build and regenerate its CMake target ledger."
    )
    parser.add_argument("platform", choices=sorted(PLATFORM_PRESETS))
    parser.add_argument("--reply-directory", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--configuration")
    parser.add_argument("--check", action="store_true")
    return parser


def main(arguments: list[str] | None = None) -> int:
    options = _argument_parser().parse_args(arguments)
    try:
        output_path = regenerate(
            platform=options.platform,
            reply_directory=options.reply_directory,
            output_path=options.output,
            configuration_name=options.configuration,
            check=options.check,
        )
    except (RegenerationError, extract_cmake_graph.GraphExtractionError, OSError) as error:
        print(f"regenerate-cmake-graph: {error}", file=sys.stderr)
        return 1

    action = "verified" if options.check else "updated"
    print(f"cmake target graph {action}: {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
