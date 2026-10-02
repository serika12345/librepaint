#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

from __future__ import annotations

import argparse
from collections import defaultdict
import configparser
from dataclasses import dataclass
import hashlib
import os
from pathlib import Path
import subprocess
import sys


MACH_O_MAGICS = frozenset(
    {
        b"\xca\xfe\xba\xbe",
        b"\xbe\xba\xfe\xca",
        b"\xca\xfe\xba\xbf",
        b"\xbf\xba\xfe\xca",
        b"\xce\xfa\xed\xfe",
        b"\xcf\xfa\xed\xfe",
        b"\xfe\xed\xfa\xce",
        b"\xfe\xed\xfa\xcf",
    }
)
SYSTEM_PREFIXES = (
    "/System/Library/Frameworks/",
    "/System/Library/PrivateFrameworks/",
    "/usr/lib/",
)
DEVELOPMENT_SUFFIXES = (
    ".a",
    ".cmake",
    ".la",
    ".o",
    ".pc",
    ".prl",
)
FORBIDDEN_PREFIXES = (
    "Contents/PlugIns/permissions",
    "Contents/PlugIns/qmllint",
    "Contents/PlugIns/qmlls",
    "Contents/PlugIns/qmltooling",
    "Contents/Resources/qml/Qt/test",
    "Contents/Resources/qml/QtTest",
)
FORBIDDEN_FRAMEWORKS = frozenset(
    {
        "QtQuickTest.framework",
        "QtTest.framework",
    }
)
ICONV_API_SYMBOLS = frozenset(
    {
        "_iconv",
        "_iconv_close",
        "_iconv_open",
        "_libiconv",
        "_libiconv_close",
        "_libiconv_open",
    }
)
REQUIRED_PYQT_MODULES = (
    "QtCore",
    "QtGui",
    "QtNetwork",
    "QtQml",
    "QtWidgets",
    "QtXml",
    "sip",
)
REQUIRED_PATHS = (
    "Contents/MacOS/LibrePaint",
    "Contents/MacOS/kritarunner",
    "Contents/MacOS/ffmpeg",
    "Contents/MacOS/ffprobe",
    "Contents/Frameworks/PyKrita.framework/PyKrita",
    "Contents/Frameworks/Python.framework/Python",
    "Contents/Frameworks/libSDL3.dylib",
    "Contents/PlugIns/kritaplugins/kritaqmic.so",
    "Contents/PlugIns/kritaplugins/kritaexrimport.so",
    "Contents/PlugIns/kritaplugins/kritaheifimport.so",
    "Contents/PlugIns/kritaplugins/kritajpegimport.so",
    "Contents/PlugIns/kritaplugins/kritajxlimport.so",
    "Contents/PlugIns/kritaplugins/kritakraimport.so",
    "Contents/PlugIns/kritaplugins/kritaoraimport.so",
    "Contents/PlugIns/kritaplugins/kritapngimport.so",
    "Contents/PlugIns/kritaplugins/kritapsdimport.so",
    "Contents/PlugIns/kritaplugins/kritasvgimport.so",
    "Contents/PlugIns/kritaplugins/kritatiffimport.so",
    "Contents/PlugIns/kritaplugins/kritawebpimport.so",
    "Contents/PlugIns/kritaplugins/kritaxcfimport.so",
    "Contents/PlugIns/mlt/libmltavformat.so",
    "Contents/PlugIns/mlt/libmltfrei0r.so",
    "Contents/PlugIns/mlt/libmltsdl2.so",
    "Contents/PlugIns/frei0r-1/brightness.so",
    "Contents/PlugIns/platforms/libqcocoa.dylib",
    "Contents/Resources/fontconfig/fonts.conf",
    "Contents/Resources/mlt/profiles/atsc_1080p_25",
    "Contents/Resources/qt.conf",
    "Contents/Resources/qml/QtQml/qmldir",
    "Contents/Resources/qml/QtQuick/qmldir",
    "Contents/Resources/qml/QtQuick/Controls/qmldir",
    "Contents/Resources/qml/QtQuick/Layouts/qmldir",
    "Contents/Resources/qml/Qt5Compat/GraphicalEffects/qmldir",
    "Contents/Resources/qml/org/krita/components/qmldir",
)


class AuditError(RuntimeError):
    pass


@dataclass(frozen=True)
class MachORecord:
    path: Path
    architectures: tuple[str, ...]
    install_name: str | None
    dependencies: tuple[str, ...]
    rpaths: tuple[str, ...]


@dataclass(frozen=True)
class BundleReport:
    mach_o_count: int
    architecture: str
    regular_file_count: int
    byte_count: int
    krita_plugin_count: int
    mlt_plugin_count: int
    frei0r_plugin_count: int


def _is_mach_o(path: Path) -> bool:
    try:
        with path.open("rb") as stream:
            return stream.read(4) in MACH_O_MAGICS
    except OSError:
        return False


def _run_tool(command: list[str], relative_path: Path) -> str:
    result = subprocess.run(command, capture_output=True, text=True, check=False)
    if result.returncode != 0:
        diagnostic = result.stderr.strip() or result.stdout.strip()
        raise AuditError(
            f"{Path(command[0]).name} could not inspect "
            f"{relative_path.as_posix()}: {diagnostic}"
        )
    return result.stdout


def _parse_dependencies(output: str) -> tuple[str, ...]:
    dependencies = []
    for line in output.splitlines()[1:]:
        if not line[:1].isspace():
            continue
        dependency = line.strip().split(" (", 1)[0]
        if dependency:
            dependencies.append(dependency)
    return tuple(dependencies)


def _parse_install_name(output: str) -> str | None:
    lines = [line.strip() for line in output.splitlines()[1:] if line.strip()]
    return lines[0] if lines else None


def _parse_rpaths(output: str) -> tuple[str, ...]:
    rpaths = []
    expecting_path = False
    for line in output.splitlines():
        stripped = line.strip()
        if stripped == "cmd LC_RPATH":
            expecting_path = True
            continue
        if expecting_path and stripped.startswith("path "):
            rpaths.append(stripped.removeprefix("path ").rsplit(" (offset ", 1)[0])
            expecting_path = False
    return tuple(rpaths)


def _parse_nm_symbols(output: str) -> frozenset[str]:
    symbols = set()
    for line in output.splitlines():
        fields = line.split()
        if fields and fields[-1] in ICONV_API_SYMBOLS:
            symbols.add(fields[-1])
    return frozenset(symbols)


def _inspect_mach_o(root: Path, path: Path, lipo: str, otool: str) -> MachORecord:
    relative_path = path.relative_to(root)
    architectures = tuple(
        _run_tool([lipo, "-archs", str(path)], relative_path).strip().split()
    )
    install_name = _parse_install_name(
        _run_tool([otool, "-D", str(path)], relative_path)
    )
    dependencies = _parse_dependencies(
        _run_tool([otool, "-L", str(path)], relative_path)
    )
    if install_name is not None:
        dependencies = tuple(
            dependency
            for dependency in dependencies
            if dependency != install_name
        )
    rpaths = _parse_rpaths(_run_tool([otool, "-l", str(path)], relative_path))
    return MachORecord(
        path=relative_path,
        architectures=architectures,
        install_name=install_name,
        dependencies=dependencies,
        rpaths=rpaths,
    )


def _inside(root: Path, path: Path) -> bool:
    try:
        path.resolve(strict=False).relative_to(root.resolve())
    except ValueError:
        return False
    return True


def _expand_token_path(root: Path, record: MachORecord, value: str) -> Path | None:
    absolute_record = root / record.path
    token_bases = (
        ("@loader_path", absolute_record.parent),
        ("@executable_path", root / "Contents/MacOS"),
    )
    for token, base in token_bases:
        if value == token:
            return base
        prefix = f"{token}/"
        if value.startswith(prefix):
            return base / value[len(prefix) :]
    return None


def _valid_rpath(root: Path, record: MachORecord, rpath: str) -> bool:
    if "/nix/store/" in rpath:
        return False
    if rpath.startswith(SYSTEM_PREFIXES):
        return True
    expanded = _expand_token_path(root, record, rpath)
    return (
        expanded is not None
        and _inside(root, expanded)
        and expanded.resolve(strict=False).is_dir()
    )


def _dependency_candidates(
    root: Path, record: MachORecord, dependency: str
) -> tuple[Path, ...]:
    if dependency.startswith("@rpath/"):
        suffix = dependency.removeprefix("@rpath/")
        candidates = []
        for rpath in record.rpaths:
            expanded = _expand_token_path(root, record, rpath)
            if expanded is not None:
                candidates.append(expanded / suffix)
        return tuple(candidates)
    expanded = _expand_token_path(root, record, dependency)
    return (expanded,) if expanded is not None else ()


def _resolve_dependency(
    root: Path,
    record: MachORecord,
    dependency: str,
    records_by_real_path: dict[Path, MachORecord],
) -> MachORecord | None:
    if dependency.startswith(SYSTEM_PREFIXES):
        return None
    if dependency.startswith("/"):
        raise AuditError(
            f"non-system absolute dependency {dependency}: {record.path.as_posix()}"
        )

    candidates = _dependency_candidates(root, record, dependency)
    for candidate in candidates:
        if not _inside(root, candidate) or not candidate.is_file():
            continue
        target = candidate.resolve()
        resolved = records_by_real_path.get(target)
        if resolved is not None:
            return resolved

    raise AuditError(
        f"unresolved Mach-O dependency {dependency}: {record.path.as_posix()}"
    )


def _development_or_test_payload(root: Path) -> tuple[str, ...]:
    rejected = set()
    for path in root.rglob("*"):
        relative = path.relative_to(root).as_posix()
        parts = path.relative_to(root).parts
        forbidden_prefix = next(
            (
                prefix
                for prefix in FORBIDDEN_PREFIXES
                if relative == prefix or relative.startswith(f"{prefix}/")
            ),
            None,
        )
        if forbidden_prefix is not None:
            rejected.add(forbidden_prefix)
            continue
        forbidden_framework = next(
            (part for part in parts if part in FORBIDDEN_FRAMEWORKS), None
        )
        if forbidden_framework is not None:
            framework_index = parts.index(forbidden_framework)
            rejected.add("/".join(parts[: framework_index + 1]))
            continue
        if (
            path.name == "Headers"
            and path.is_dir()
            and "Contents" in parts
            and "Frameworks" in parts
        ):
            rejected.add(relative)
            continue
        if "PyQt6" in parts:
            pyqt_index = parts.index("PyQt6")
            pyqt_relative = parts[pyqt_index + 1 :]
            if (
                pyqt_relative[:1] == ("bindings",)
                or path.name == "py.typed"
                or path.suffix == ".pyi"
                or path.name.startswith("QtTest")
            ):
                rejected.add(relative)
                continue
        if path.is_file() and path.name.casefold().endswith(DEVELOPMENT_SUFFIXES):
            rejected.add(relative)
    return tuple(sorted(rejected))


def _invalid_symlinks(root: Path) -> tuple[str, ...]:
    diagnostics = []
    for path in sorted(root.rglob("*")):
        if not path.is_symlink():
            continue
        relative = path.relative_to(root).as_posix()
        try:
            target = path.resolve(strict=True)
        except (FileNotFoundError, RuntimeError):
            diagnostics.append(f"{relative}: missing or cyclic target")
            continue
        if not _inside(root, target):
            diagnostics.append(f"{relative}: target escapes bundle")
    return tuple(diagnostics)


def _feature_diagnostics(root: Path) -> tuple[str, ...]:
    missing = [relative for relative in REQUIRED_PATHS if not (root / relative).is_file()]
    fontconfig = root / "Contents/Resources/fontconfig/fonts.conf"
    try:
        fontconfig_text = fontconfig.read_text()
    except OSError:
        fontconfig_text = ""
    required_font_directories = (
        "/System/Library/Fonts",
        "/Library/Fonts",
        "~/Library/Fonts",
    )
    if (
        "/nix/store/" in fontconfig_text
        or not all(path in fontconfig_text for path in required_font_directories)
    ):
        missing.append("macOS font configuration")
    qt_configuration = root / "Contents/Resources/qt.conf"
    configuration = configparser.ConfigParser(interpolation=None)
    try:
        configuration.read(qt_configuration)
    except configparser.Error:
        configuration.clear()
    if not all(
        configuration.get("Paths", key, fallback=None) == value
        for key, value in {
            "Plugins": "PlugIns",
            "Imports": "Resources/qml",
            "QmlImports": "Resources/qml",
        }.items()
    ):
        missing.append("relocatable Qt runtime configuration")
    python_lib = root / "Contents/Frameworks/Python.framework/Versions/Current/lib"
    python_stdlibs = tuple(
        path for path in python_lib.glob("python*.*") if path.is_dir()
    )
    if not any((path / "encodings/__init__.py").is_file() for path in python_stdlibs):
        missing.append("Python standard library")
    python_runtime_files = (
        path / relative
        for path in python_stdlibs
        for relative in ("subprocess.py", "mimetypes.py")
    )
    if any(
        "/nix/store/" in path.read_text(errors="replace")
        for path in python_runtime_files
        if path.is_file()
    ):
        missing.append("portable Python standard library")
    pyqt_roots = tuple(
        path / "site-packages/PyQt6" for path in python_stdlibs
    )
    if not all(
        any(tuple(root.glob(f"{module}*.so")) for root in pyqt_roots)
        for module in REQUIRED_PYQT_MODULES
    ):
        missing.append("PyQt6 runtime")
    launchers = (
        root / "Contents/MacOS/LibrePaint",
        root / "Contents/MacOS/kritarunner",
    )
    if any(
        b"/nix/store/" in path.read_bytes()
        for path in launchers
        if path.is_file()
    ):
        missing.append("portable application launchers")
    if not any((root / "Contents/Resources/share/locale").rglob("*.mo")):
        missing.append("Gettext translations")
    if not any((root / "Contents/Resources/share/icons").rglob("*")):
        missing.append("runtime icons")
    return tuple(missing)


def _verify_signature(root: Path, codesign: str) -> None:
    result = subprocess.run(
        [codesign, "--verify", "--deep", "--strict", "--verbose=2", str(root)],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        diagnostic = result.stderr.strip() or result.stdout.strip()
        raise AuditError(f"invalid macOS code signature: {diagnostic}")


def inspect_bundle(
    root: Path,
    *,
    lipo: str = "lipo",
    otool: str = "otool",
    codesign: str = "codesign",
    require_features: bool = True,
    verify_signature: bool = True,
) -> BundleReport:
    root = root.resolve()
    if not root.is_dir() or root.suffix != ".app":
        raise AuditError(f"macOS application bundle does not exist: {root}")

    regular_files = tuple(
        path
        for path in root.rglob("*")
        if path.is_file() and not path.is_symlink()
    )
    candidates = tuple(path for path in regular_files if _is_mach_o(path))
    if not candidates:
        raise AuditError(f"macOS application bundle contains no Mach-O files: {root}")

    records = tuple(
        _inspect_mach_o(root, path, lipo, otool) for path in sorted(candidates)
    )
    diagnostics = []

    wrong_architectures = tuple(
        record for record in records if record.architectures != ("arm64",)
    )
    if wrong_architectures:
        diagnostics.append("unsupported Mach-O architectures:")
        diagnostics.extend(
            f"  {record.path.as_posix()}: {' '.join(record.architectures)}"
            for record in wrong_architectures
        )

    invalid_rpaths = []
    for record in records:
        invalid = tuple(
            rpath for rpath in record.rpaths if not _valid_rpath(root, record, rpath)
        )
        if invalid:
            invalid_rpaths.append(
                f"  {record.path.as_posix()}: {', '.join(invalid)}"
            )
    if invalid_rpaths:
        diagnostics.append("invalid RPATH entries:")
        diagnostics.extend(invalid_rpaths)

    install_name_records: dict[str, list[MachORecord]] = defaultdict(list)
    for record in records:
        if record.install_name is not None:
            install_name_records[record.install_name].append(record)
    conflicts = []
    for install_name, grouped_records in sorted(install_name_records.items()):
        if len(grouped_records) < 2:
            continue
        hashes = {
            hashlib.sha256((root / record.path).read_bytes()).hexdigest()
            for record in grouped_records
        }
        if len(hashes) > 1:
            paths = ", ".join(
                record.path.as_posix() for record in sorted(grouped_records, key=lambda item: item.path)
            )
            conflicts.append(f"  {install_name}: {paths}")
    if conflicts:
        diagnostics.append("conflicting Mach-O install names:")
        diagnostics.extend(conflicts)

    records_by_real_path = {
        (root / record.path).resolve(): record for record in records
    }
    dependency_errors = []
    iconv_errors = []
    undefined_iconv: dict[Path, frozenset[str]] = {}
    exported_iconv: dict[Path, frozenset[str]] = {}
    for record in records:
        for dependency in record.dependencies:
            try:
                resolved = _resolve_dependency(
                    root, record, dependency, records_by_real_path
                )
            except AuditError as error:
                dependency_errors.append(str(error))
                continue
            if Path(dependency).name != "libiconv.2.dylib":
                continue
            if record.path not in undefined_iconv:
                undefined_iconv[record.path] = _parse_nm_symbols(
                    _run_tool(["nm", "-u", str(root / record.path)], record.path)
                )
            if resolved is None:
                provided = frozenset(symbol for symbol in ICONV_API_SYMBOLS if symbol.startswith("_iconv"))
                provider_path = dependency
            else:
                if resolved.path not in exported_iconv:
                    exported_iconv[resolved.path] = _parse_nm_symbols(
                        _run_tool(["nm", "-gU", str(root / resolved.path)], resolved.path)
                    )
                provided = exported_iconv[resolved.path]
                provider_path = resolved.path.as_posix()
            missing = sorted(undefined_iconv[record.path] - provided)
            if missing:
                iconv_errors.append(
                    "iconv ABI mismatch: "
                    f"{', '.join(missing)} required by "
                    f"{record.path.as_posix()} but not provided by "
                    f"{provider_path}"
                )
    if dependency_errors:
        diagnostics.extend(sorted(dependency_errors))
    if iconv_errors:
        diagnostics.extend(sorted(iconv_errors))

    rejected_payload = _development_or_test_payload(root)
    if rejected_payload:
        diagnostics.append(
            "development or test payload: " + ", ".join(rejected_payload)
        )

    invalid_links = _invalid_symlinks(root)
    if invalid_links:
        diagnostics.append("invalid bundle symlinks: " + "; ".join(invalid_links))

    if require_features:
        missing_features = _feature_diagnostics(root)
        if missing_features:
            diagnostics.append(
                "missing macOS runtime features: " + ", ".join(missing_features)
            )

    if diagnostics:
        raise AuditError("\n".join(diagnostics))
    if verify_signature:
        _verify_signature(root, codesign)

    krita_plugin_count = len(
        tuple((root / "Contents/PlugIns/kritaplugins").glob("*.so"))
    )
    mlt_plugin_count = len(tuple((root / "Contents/PlugIns/mlt").glob("*.so")))
    frei0r_plugin_count = len(
        tuple((root / "Contents/PlugIns/frei0r-1").glob("*.so"))
    )
    return BundleReport(
        mach_o_count=len(records),
        architecture="arm64",
        regular_file_count=len(regular_files),
        byte_count=sum(path.stat().st_size for path in regular_files),
        krita_plugin_count=krita_plugin_count,
        mlt_plugin_count=mlt_plugin_count,
        frei0r_plugin_count=frei0r_plugin_count,
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Verify the runtime closure and payload of a macOS app bundle"
    )
    parser.add_argument("bundle", type=Path)
    parser.add_argument("--lipo", default="lipo")
    parser.add_argument("--otool", default="otool")
    parser.add_argument("--codesign", default="codesign")
    parser.add_argument("--skip-signature", action="store_true")
    options = parser.parse_args()

    try:
        report = inspect_bundle(
            options.bundle,
            lipo=options.lipo,
            otool=options.otool,
            codesign=options.codesign,
            verify_signature=not options.skip_signature,
        )
    except AuditError as error:
        print(f"audit-macos-bundle: {error}", file=sys.stderr)
        return 1

    print(
        "macOS bundle verified: "
        f"{report.mach_o_count} Mach-O files, {report.architecture}, "
        f"{report.krita_plugin_count} Krita plugins, "
        f"{report.mlt_plugin_count} MLT plugins, "
        f"{report.frei0r_plugin_count} frei0r plugins, "
        f"{report.regular_file_count} regular files, {report.byte_count} bytes"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
