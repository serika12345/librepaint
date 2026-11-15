#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

"""Check that named PE imports resolve inside a LibrePaint Windows ZIP.

A Windows package can mix DLLs from different builds. A DLL that calls an
export which the packaged provider no longer has fails at load time with
entry point not found (0xC0000139). This tool reads the ZIP directly, builds
the export table of every packaged DLL, and reports each named import from
`bin/` and `lib/kritaplugins/` that the provider does not export.

Only providers inside the ZIP are checked. Imports of Windows system DLLs
and other external DLLs are left to the operating system. The exit status is
1 when any import is missing, so a package can be rejected before it is
copied to a measurement device.

The test shell already provides `pefile`. A shell without it can use the
temporary Nix environment in `check_zip_imports.nix`:

    ./scripts/run-shared-test-env nix-shell scripts/perf/check_zip_imports.nix \
        --run 'python3 scripts/perf/check_zip_imports.py PACKAGE.zip'

Usage:
  check_zip_imports.py PACKAGE.zip
"""

from __future__ import annotations

import argparse
import zipfile

import pefile


PE_SUFFIXES = frozenset({".com", ".dll", ".exe", ".pyd"})
CONSUMER_DIRECTORIES = ("bin/", "lib/kritaplugins/")
# pefile defaults to 0x2000 exported names and 0x2000 imported symbols per
# file. Qt6Widgets.dll in the official Qt package has more (8463) exported
# names, and libkritaapplicationui.dll has more imported symbols; the
# defaults drop the tail of both tables.
MAX_SYMBOL_EXPORTS = 1_000_000
pefile.MAX_IMPORT_SYMBOLS = 1_000_000


def _entry_suffix(name: str) -> str:
    base = name.rsplit("/", 1)[-1]
    dot = base.rfind(".")
    return base[dot:].casefold() if dot >= 0 else ""


def _directory_entries(names: list[str], directory: str) -> list[str]:
    prefix = directory.casefold()
    found = []
    for name in names:
        lowered = name.casefold()
        if not lowered.startswith(prefix):
            continue
        remainder = name[len(directory):]
        if not remainder or "/" in remainder:
            continue
        if _entry_suffix(name) in PE_SUFFIXES:
            found.append(name)
    return sorted(found)


def _decode(value: bytes | str | None) -> str:
    if value is None:
        return ""
    if isinstance(value, bytes):
        return value.decode("ascii", "replace")
    return value


def _load(
    archive: zipfile.ZipFile,
    name: str,
    directory: int,
) -> pefile.PE:
    pe = pefile.PE(
        data=archive.read(name),
        fast_load=True,
        max_symbol_exports=MAX_SYMBOL_EXPORTS,
    )
    pe.parse_data_directories(directories=[directory])
    return pe


def read_exports(archive: zipfile.ZipFile, name: str) -> frozenset[str]:
    pe = _load(archive, name, pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_EXPORT"])
    exports: set[str] = set()
    directory = getattr(pe, "DIRECTORY_ENTRY_EXPORT", None)
    if directory is not None:
        for symbol in directory.symbols:
            if symbol.name is not None:
                exports.add(_decode(symbol.name))
    return frozenset(exports)


def read_named_imports(archive: zipfile.ZipFile, name: str) -> list[tuple[str, str]]:
    pe = _load(archive, name, pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_IMPORT"])
    imports: list[tuple[str, str]] = []
    for entry in getattr(pe, "DIRECTORY_ENTRY_IMPORT", []):
        provider = _decode(entry.dll)
        for imported in entry.imports:
            if imported.name is None:
                continue
            imports.append((provider, _decode(imported.name)))
    return imports


def check_archive(path: str) -> list[tuple[str, str, str]]:
    with zipfile.ZipFile(path) as archive:
        names = archive.namelist()
        providers = {
            name.rsplit("/", 1)[-1].casefold(): name
            for name in names
            if _entry_suffix(name) == ".dll"
        }
        exports = {
            provider: read_exports(archive, name)
            for provider, name in providers.items()
        }

        missing: set[tuple[str, str, str]] = set()
        for directory in CONSUMER_DIRECTORIES:
            for name in _directory_entries(names, directory):
                consumer = name.rsplit("/", 1)[-1]
                for provider, symbol in read_named_imports(archive, name):
                    provider_name = provider.casefold()
                    if provider_name not in exports:
                        continue
                    if symbol not in exports[provider_name]:
                        missing.add((consumer, provider, symbol))
    return sorted(missing)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Check named PE imports against the DLLs in a ZIP."
    )
    parser.add_argument("package", help="LibrePaint Windows ZIP")
    arguments = parser.parse_args()

    missing = check_archive(arguments.package)
    if missing:
        for consumer, provider, symbol in missing:
            print(f"{consumer}, {provider}, {symbol}")
        print(f"check_zip_imports: {len(missing)} missing import(s)", flush=True)
        return 1

    print("check_zip_imports: OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
