#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

from __future__ import annotations

import argparse
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
import re
import subprocess
import sys
import zipfile


PE_SUFFIXES = frozenset({".com", ".dll", ".exe", ".pyd"})
X86_64_FORMATS = frozenset({"coff-x86-64", "pei-x86-64"})
SYSTEM_DLLS = frozenset(
    {
        "advapi32.dll",
        "authz.dll",
        "avrt.dll",
        "avicap32.dll",
        "bcrypt.dll",
        "bcryptprimitives.dll",
        "cabinet.dll",
        "cfgmgr32.dll",
        "clbcatq.dll",
        "comctl32.dll",
        "comdlg32.dll",
        "credui.dll",
        "crypt32.dll",
        "cryptbase.dll",
        "cryptui.dll",
        "d2d1.dll",
        "d3d11.dll",
        "d3d12.dll",
        "d3d9.dll",
        "dcomp.dll",
        "dbghelp.dll",
        "dhcpcsvc.dll",
        "dnsapi.dll",
        "dwmapi.dll",
        "dwrite.dll",
        "dxgi.dll",
        "dxguid.dll",
        "dxva2.dll",
        "gdi32.dll",
        "gdiplus.dll",
        "hid.dll",
        "imagehlp.dll",
        "imm32.dll",
        "iphlpapi.dll",
        "kernel32.dll",
        "ksuser.dll",
        "mf.dll",
        "mfplat.dll",
        "mfreadwrite.dll",
        "mfuuid.dll",
        "mmdevapi.dll",
        "mpr.dll",
        "msimg32.dll",
        "msi.dll",
        "msvcrt.dll",
        "mswsock.dll",
        "ncrypt.dll",
        "netapi32.dll",
        "normaliz.dll",
        "ntasn1.dll",
        "ntdll.dll",
        "odbc32.dll",
        "ole32.dll",
        "oleacc.dll",
        "oleaut32.dll",
        "opengl32.dll",
        "powrprof.dll",
        "propsys.dll",
        "psapi.dll",
        "rpcrt4.dll",
        "secur32.dll",
        "sensapi.dll",
        "setupapi.dll",
        "shcore.dll",
        "shell32.dll",
        "shlwapi.dll",
        "strmiids.dll",
        "user32.dll",
        "userenv.dll",
        "usp10.dll",
        "uxtheme.dll",
        "version.dll",
        "wevtapi.dll",
        "windowscodecs.dll",
        "winhttp.dll",
        "wininet.dll",
        "winmm.dll",
        "winspool.drv",
        "wintrust.dll",
        "wlanapi.dll",
        "wldap32.dll",
        "ws2_32.dll",
        "wsock32.dll",
        "wtsapi32.dll",
        "xinput1_4.dll",
    }
)
SYSTEM_DLL_PATTERNS = (
    re.compile(r"^api-ms-win-.*[.]dll$"),
    re.compile(r"^ext-ms-.*[.]dll$"),
)
FORMAT_PATTERN = re.compile(r"file format\s+(\S+)")
IMPORT_PATTERN = re.compile(r"^\s*DLL Name:\s*(\S+)\s*$", re.MULTILINE)
DEVELOPMENT_SUFFIXES = (".a", ".cmake", ".dll.a", ".la", ".pc", ".prl")
FORBIDDEN_PATHS = frozenset(
    {
        "MakeinstallerNsis.cmake",
        "bin/Qt6QuickTest.dll",
        "bin/Qt6Test.dll",
        "bin/krita_version.exe",
        "lib/site-packages/PyQt6/QtTest.pyd",
    }
)
FORBIDDEN_DIRECTORIES = (
    "bin/plugins/qmltooling",
    "bin/qml/Qt/test",
    "bin/qml/QtTest",
    "include",
    "installer",
)
PYTHON_TEST_EXTENSION_PATTERN = re.compile(
    r"^(?:_.*_test|_test.*|_xxtestfuzz)[.]cpython-[^.]+[.]dll$",
    re.IGNORECASE,
)


# These are the runtime entry points used by the portable Windows distribution.
# Their PE imports establish the remaining linked dependency requirements.
REQUIRED_FEATURES = {
    "LibrePaint executable": "bin/LibrePaint.exe",
    "Default paint operations": "lib/kritaplugins/kritadefaultpaintops.dll",
    "Default tools": "lib/kritaplugins/kritadefaulttools.dll",
    "Qt Windows platform": "bin/plugins/platforms/qwindows.dll",
    "FFmpeg": "bin/ffmpeg.exe",
    "FFprobe": "bin/ffprobe.exe",
    "Python runtime": "bin/libpython3.*.dll",
    "Python standard library": "python/python3*.zip",
    "Krita Python plugin": "lib/kritaplugins/kritapykrita.dll",
    "Krita Python module": "lib/krita-python-libs/krita/__init__.py",
    "PyQt6 core": "lib/site-packages/PyQt6/QtCore.pyd",
    "PyQt6 GUI": "lib/site-packages/PyQt6/QtGui.pyd",
    "PyQt6 widgets": "lib/site-packages/PyQt6/QtWidgets.pyd",
    "G'MIC plugin": "lib/kritaplugins/krita_gmic_qt.dll",
    "G'MIC runtime data": "share/gmic/gmic_cluts.gmz",
    "MLT runtime": "bin/libmlt-7.dll",
    "MLT core module": "bin/libmltcore.dll",
    "MLT FFmpeg module": "bin/libmltavformat.dll",
    "MLT SDL audio module": "bin/libmltsdl2.dll",
    "SVG import plugin": "lib/kritaplugins/kritasvgimport.dll",
    "XCF import plugin": "lib/kritaplugins/kritaxcfimport.dll",
}
for _format, _identifier in (
    ("KRA", "kra"), ("PNG", "png"), ("JPEG", "jpeg"), ("OpenEXR", "exr"),
    ("TIFF", "tiff"), ("PSD", "psd"), ("WebP", "webp"), ("HEIF", "heif"),
    ("JPEG XL", "jxl"), ("OpenRaster", "ora"),
):
    for _operation in ("import", "export"):
        REQUIRED_FEATURES[f"{_format} {_operation} plugin"] = (
            f"lib/kritaplugins/krita{_identifier}{_operation}.dll"
        )


class AuditError(RuntimeError):
    pass


@dataclass(frozen=True)
class PeRecord:
    path: Path
    format: str
    imports: tuple[str, ...]


@dataclass(frozen=True)
class PackageReport:
    pe_count: int
    machine: str


def _is_system_dll(name: str) -> bool:
    lowered = name.casefold()
    return lowered in SYSTEM_DLLS or any(
        pattern.fullmatch(lowered) for pattern in SYSTEM_DLL_PATTERNS
    )


def _parse_objdump(path: Path, output: str) -> PeRecord:
    format_match = FORMAT_PATTERN.search(output)
    if format_match is None:
        raise AuditError(f"objdump did not report a PE format for {path.as_posix()}")
    imports = tuple(match.casefold() for match in IMPORT_PATTERN.findall(output))
    return PeRecord(path=path, format=format_match.group(1), imports=imports)


def _inspect_pe(root: Path, path: Path, objdump: str) -> PeRecord:
    result = subprocess.run(
        [objdump, "-p", str(path)],
        capture_output=True,
        text=True,
        check=False,
    )
    relative_path = path.relative_to(root)
    if result.returncode != 0:
        diagnostic = result.stderr.strip() or result.stdout.strip()
        raise AuditError(
            f"objdump could not inspect {relative_path.as_posix()}: {diagnostic}"
        )
    return _parse_objdump(relative_path, result.stdout)


def _development_or_test_payload(root: Path) -> tuple[str, ...]:
    rejected: set[str] = set()
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        relative = path.relative_to(root).as_posix()
        lowered = relative.casefold()
        if relative in FORBIDDEN_PATHS or any(
            relative == directory or relative.startswith(f"{directory}/")
            for directory in FORBIDDEN_DIRECTORIES
        ):
            rejected.add(relative)
        if lowered.endswith(DEVELOPMENT_SUFFIXES):
            rejected.add(relative)
        if PYTHON_TEST_EXTENSION_PATTERN.fullmatch(path.name):
            rejected.add(relative)

    for archive_path in root.rglob("python*.zip"):
        try:
            with zipfile.ZipFile(archive_path) as archive:
                for name in archive.namelist():
                    parts = tuple(part.casefold() for part in Path(name).parts)
                    if "test" in parts or "tests" in parts:
                        relative_archive = archive_path.relative_to(root).as_posix()
                        rejected.add(f"{relative_archive}!{name}")
        except zipfile.BadZipFile as error:
            relative_archive = archive_path.relative_to(root).as_posix()
            raise AuditError(
                f"invalid Python standard-library archive {relative_archive}: {error}"
            ) from error

    return tuple(sorted(rejected))


def inspect_package(root: Path, objdump: str) -> PackageReport:
    root = root.resolve()
    if not root.is_dir():
        raise AuditError(f"Windows package directory does not exist: {root}")

    candidates = sorted(
        path
        for path in root.rglob("*")
        if path.is_file() and path.suffix.casefold() in PE_SUFFIXES
    )
    if not candidates:
        raise AuditError(f"Windows package contains no PE files: {root}")

    records = tuple(_inspect_pe(root, path, objdump) for path in candidates)
    paths_by_basename: dict[str, list[Path]] = defaultdict(list)
    for record in records:
        paths_by_basename[record.path.name.casefold()].append(record.path)

    diagnostics: list[str] = []
    duplicates = {
        name: paths
        for name, paths in paths_by_basename.items()
        if len(paths) > 1
    }
    if duplicates:
        diagnostics.append("duplicate DLL basenames:")
        for name, paths in sorted(duplicates.items()):
            joined_paths = ", ".join(path.as_posix() for path in sorted(paths))
            diagnostics.append(f"  {name}: {joined_paths}")

    wrong_formats = tuple(
        record for record in records if record.format not in X86_64_FORMATS
    )
    if wrong_formats:
        diagnostics.append("unsupported PE formats:")
        diagnostics.extend(
            f"  {record.path.as_posix()}: {record.format}"
            for record in wrong_formats
        )

    rejected_payload = _development_or_test_payload(root)
    if rejected_payload:
        diagnostics.append(
            "development or test payload: " + ", ".join(rejected_payload)
        )

    for feature, pattern in REQUIRED_FEATURES.items():
        if not any(path.is_file() for path in root.glob(pattern)):
            diagnostics.append(f"missing required Windows feature: {feature} ({pattern})")

    pyqt_directory = root / "lib/site-packages/PyQt6"
    if not any(path.is_file() for path in pyqt_directory.glob("sip.cpython-*.dll")):
        diagnostics.append(
            "missing PyQt6 SIP runtime: lib/site-packages/PyQt6/sip.cpython-*.dll"
        )

    missing_consumers: dict[str, set[Path]] = defaultdict(set)
    # Packaging places shared DLL dependencies beside LibrePaint.exe and its
    # helper executables. Plugin/module discovery paths do not add directories
    # to the process DLL search path.
    provided_names = frozenset(
        record.path.name.casefold()
        for record in records
        if record.path.parent.as_posix().casefold() == "bin"
    )
    for record in records:
        for imported_name in record.imports:
            if imported_name not in provided_names and not _is_system_dll(imported_name):
                missing_consumers[imported_name].add(record.path)
    if missing_consumers:
        diagnostics.append("missing Windows runtime DLLs:")
        for name, consumers in sorted(missing_consumers.items()):
            joined_consumers = ", ".join(
                path.as_posix() for path in sorted(consumers)
            )
            diagnostics.append(f"  {name}: {joined_consumers}")

    if diagnostics:
        raise AuditError("\n".join(diagnostics))

    return PackageReport(pe_count=len(records), machine="x86_64")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Verify the architecture and runtime DLL closure of a Windows package"
    )
    parser.add_argument("package", type=Path)
    parser.add_argument("--objdump", default="objdump")
    options = parser.parse_args()

    try:
        report = inspect_package(options.package, options.objdump)
    except AuditError as error:
        print(f"audit-windows-package: {error}", file=sys.stderr)
        return 1

    print(
        f"Windows package verified: {report.pe_count} PE files, {report.machine}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
