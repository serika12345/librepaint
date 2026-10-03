#!/usr/bin/env python3

from __future__ import annotations

import argparse
from collections import deque
import hashlib
import os
import plistlib
import re
import shutil
import pathlib
import stat
import subprocess
import sys


MACH_O_MAGICS = frozenset(
    {
        b'\xca\xfe\xba\xbe',
        b'\xbe\xba\xfe\xca',
        b'\xca\xfe\xba\xbf',
        b'\xbf\xba\xfe\xca',
        b'\xce\xfa\xed\xfe',
        b'\xcf\xfa\xed\xfe',
        b'\xfe\xed\xfa\xce',
        b'\xfe\xed\xfa\xcf',
    }
)
ICONV_API_SYMBOLS = frozenset(
    {
        '_iconv',
        '_iconv_close',
        '_iconv_open',
        '_libiconv',
        '_libiconv_close',
        '_libiconv_open',
    }
)


def main():
    parser = argparse.ArgumentParser(prog='macos deploy',
                                     description='Utility to create the LibrePaint.app bundle')

    parser.add_argument('--buildroot', help="Directory where the source and _install are located",
                        default=os.getenv("BUILDROOT", False))
    parser.add_argument('--install-dir', dest='install_dir', help="Path of install directory to deploy")
    parser.add_argument('--output-dir', dest='output_dir', help="Destination path to place the app")
    parser.add_argument('--source', dest='source', help="source location of LibrePaint")
    parser.add_argument('--krita-source', dest='source', help=argparse.SUPPRESS)
    parser.add_argument(
        '--signing-identity',
        default=os.getenv('MACOS_CODESIGN_IDENTITY', '-'),
        help="codesign identity for the finished bundle (default: ad-hoc '-')",
    )
    args = parser.parse_args()

    # --- Locations
    if args.buildroot:
        if args.install_dir:
            print("WARNING: --install_dir ignored as --buildroot or env BUILDROOT is present")
        if args.source:
            print("WARNING: --source ignored as --buildroot or env BUILDROOT is present")

        krita_root =pathlib.Path(args.buildroot).resolve()
        krita_install_dir = pathlib.Path(os.path.join(krita_root, "_install"))
        krita_source_dir = pathlib.Path(os.path.join(krita_root, "krita"))
        krita_dmg = pathlib.Path(os.path.join(krita_root, "_dmg"))

        if args.output_dir:
            krita_dmg = pathlib.Path(args.output_dir).resolve()

    else:
        if not args.install_dir or not args.output_dir or not args.source:
            print("ERROR: if --buildroot or env BUILDROOT is missing, --install-dir, --output-dir, and --source must all be present")
            exit(1)

        krita_install_dir = pathlib.Path(args.install_dir).resolve()
        krita_source_dir = pathlib.Path(args.source).resolve()
        krita_dmg = pathlib.Path(args.output_dir).resolve()


    kritaDeploy(
        krita_install_dir,
        krita_dmg,
        krita_source_dir,
        signing_identity=args.signing_identity,
    )


# --- helpers
class DeployCmd:
    # achmod appends the mod to file
    achmod = lambda target, mode: os.lchmod(target, stat.S_IMODE(os.stat(target).st_mode) | mode)
    # xchmod removes the bit flags from file
    xchmod = lambda target, mode: os.lchmod(target, stat.S_IMODE(os.stat(target).st_mode) & ~mode)


def cmdLog(cmd: list):
    print(f'## RUNNING: {" ".join(map(lambda m: str(m), cmd))}')
    return


def copyDirSub(src: pathlib.Path, dst: pathlib.Path, extra_args: list=None
               , only_contents: bool=True
               , capture_output=True):
    if only_contents:
        src = str(src) + os.sep
    cmd = ['rsync','-priul']
    if extra_args:
        cmd.extend(extra_args)
    cmd.extend([src,dst])
    cmdLog(cmd)
    subprocess.run(cmd,text=True, check=True,capture_output=capture_output)
    makeTreeOwnerWritable(pathlib.Path(dst))


def findNixQmlRuntimeRoots(install_dir: pathlib.Path) -> tuple[pathlib.Path, ...]:
    """Find the QML trees carried by a Nix application's runtime closure."""
    result = subprocess.run(
        ['nix-store', '-qR', install_dir],
        capture_output=True,
        text=True,
        check=True,
    )
    application_root = install_dir.joinpath('lib', 'qt-6', 'qml').resolve()
    roots = {
        pathlib.Path(store_path).joinpath('lib', 'qt-6', 'qml').resolve()
        for store_path in result.stdout.splitlines()
        if pathlib.Path(store_path).joinpath('lib', 'qt-6', 'qml').is_dir()
    }
    roots = {
        root for root in roots
        if root == application_root
        or (
            root.joinpath('QtQml', 'qmldir').is_file()
            and root.joinpath('QtQuick', 'qmldir').is_file()
        )
        or root.joinpath('Qt5Compat', 'GraphicalEffects', 'qmldir').is_file()
    }
    ordered = sorted(root for root in roots if root != application_root)
    if application_root.is_dir():
        ordered.append(application_root)
    return tuple(ordered)


def findNixQtPluginRoots(install_dir: pathlib.Path) -> tuple[pathlib.Path, ...]:
    """Find the Qt platform and image plugin trees used by the application."""
    result = subprocess.run(
        ['nix-store', '-qR', install_dir],
        capture_output=True,
        text=True,
        check=True,
    )
    roots = {
        pathlib.Path(store_path).joinpath('lib', 'qt-6', 'plugins').resolve()
        for store_path in result.stdout.splitlines()
        if pathlib.Path(store_path).joinpath('lib', 'qt-6', 'plugins').is_dir()
    }
    return tuple(sorted(
        root for root in roots
        if root.joinpath('platforms', 'libqcocoa.dylib').is_file()
        or root.joinpath('imageformats', 'libqsvg.dylib').is_file()
    ))


def copyNixQtPlugins(
        install_dir: pathlib.Path,
        plugins: pathlib.Path,
        ) -> tuple[pathlib.Path, ...]:
    """Merge the required Qt platform, image, and icon plugins."""
    roots = findNixQtPluginRoots(install_dir)
    if not roots:
        raise FileNotFoundError(
            f'no Qt runtime plugins found in the closure of {install_dir}'
        )
    for root in roots:
        copyDirSub(root, plugins)
    return roots


def installSdl3Runtime(
        install_dir: pathlib.Path,
        frameworks: pathlib.Path,
        ) -> pathlib.Path:
    """Install the SDL3 library loaded dynamically by sdl2-compat."""
    result = subprocess.run(
        ['nix-store', '-qR', install_dir],
        capture_output=True,
        text=True,
        check=True,
    )
    candidates = {
        pathlib.Path(store_path).joinpath('lib', 'libSDL3.dylib').resolve()
        for store_path in result.stdout.splitlines()
        if pathlib.Path(store_path).joinpath('lib', 'libSDL3.dylib').is_file()
    }
    if len(candidates) != 1:
        diagnostic = ', '.join(map(str, sorted(candidates))) or 'missing'
        raise RuntimeError(
            f'expected one SDL3 runtime in {install_dir} closure: {diagnostic}'
        )
    destination = frameworks.joinpath('libSDL3.dylib')
    shutil.copy2(next(iter(candidates)), destination)
    destination.chmod(destination.stat().st_mode | stat.S_IWUSR)
    return destination


def copyNixQmlRuntime(
        install_dir: pathlib.Path,
        resources: pathlib.Path,
        ) -> tuple[pathlib.Path, ...]:
    """Merge the Qt and application QML runtimes into the bundle."""
    roots = findNixQmlRuntimeRoots(install_dir)
    if not roots:
        raise FileNotFoundError(
            f'no QML runtime found in the closure of {install_dir}'
        )
    destination = resources.joinpath('qml')
    _removePath(destination)
    destination.mkdir()
    for root in roots:
        copyDirSub(root, destination)
    return roots


def isBinary(file: pathlib.Path) -> bool:
    try:
        with pathlib.Path(file).open('rb') as stream:
            return stream.read(4) in MACH_O_MAGICS
    except OSError:
        return False


def getLinkedLibs(lib: pathlib.Path) -> list[pathlib.Path]:
    libsUsed = []
    result = subprocess.run(['otool','-L',lib],capture_output=True,text=True,check=True)
    libList = result.stdout.split("\n")
    for entry in libList[1:]:
        # on fat-binaries we do not want to search twice
        if "architecture" in entry or not entry:
            break

        libEntry = entry.strip().split(" (", 1)[0]
        libsUsed.append(pathlib.Path(libEntry))

    return libsUsed


def _iconvSymbols(
        binary: pathlib.Path,
        *,
        undefined: bool,
        ) -> frozenset[str]:
    """Return the Apple or GNU iconv API symbols used by a Mach-O file."""
    command = ['nm', '-u' if undefined else '-gU', binary]
    result = subprocess.run(command, capture_output=True, text=True, check=True)
    return frozenset(
        fields[-1]
        for line in result.stdout.splitlines()
        if (fields := line.split()) and fields[-1] in ICONV_API_SYMBOLS
    )


def _systemDependencyReplacement(dependency: pathlib.Path) -> str | None:
    """Map Nix's Apple iconv shim back to the stable macOS system ABI."""
    if dependency.name != 'libiconv.2.dylib' or not dependency.is_file():
        return None
    symbols = _iconvSymbols(dependency, undefined=False)
    if '_iconv' in symbols and '_libiconv' not in symbols:
        return '/usr/lib/libiconv.2.dylib'
    return None


def _dependencyFrameworkRoot(path: pathlib.Path) -> pathlib.Path | None:
    return next(
        (
            candidate for candidate in (path, *path.parents)
            if candidate.name.endswith('.framework')
        ),
        None,
    )


def copyNixStoreDependencyClosure(app: pathlib.Path) -> tuple[pathlib.Path, ...]:
    """Copy original absolute dependencies and relocate each Mach-O once."""
    contents = app.joinpath('Contents').resolve()
    frameworks = contents.joinpath('Frameworks')
    frameworks.mkdir(parents=True, exist_ok=True)
    pending = deque()
    original_hashes = {}
    source_hashes = {}
    destinations_by_source = {}
    system_replacements = {}
    copied = []
    python_binary = frameworks.joinpath('Python.framework', 'Versions', 'Current', 'Python')
    python_binary = python_binary.resolve() if python_binary.is_file() else None

    def digest(path):
        with path.open('rb') as stream:
            return hashlib.file_digest(stream, 'sha256').digest()

    def register(path):
        if not path.is_file() or not isBinary(path):
            return
        real_path = path.resolve(strict=True)
        if not real_path.is_relative_to(contents):
            raise RuntimeError(f'runtime symlink escapes bundle: {path}')
        if real_path not in original_hashes:
            original_hashes[real_path] = digest(real_path)
            pending.append(real_path)

    def destination_for(dependency):
        if dependency.is_absolute():
            existing = destinations_by_source.get(dependency.resolve())
            if existing is not None:
                return existing
        framework = _dependencyFrameworkRoot(dependency)
        if framework is not None:
            return frameworks.joinpath(framework.name, dependency.relative_to(framework))
        return frameworks.joinpath(dependency.name)

    for path in sorted(contents.rglob('*')):
        register(path)
    inspected = 0
    print(f'Dependency closure: {len(pending)} Mach-O files queued', flush=True)
    while pending:
        binary = pending.popleft()
        result = subprocess.run(['otool', '-D', binary], capture_output=True,
                                text=True, check=True)
        ids = [line.strip() for line in result.stdout.splitlines()[1:] if line.strip()]
        install_name = ids[0] if ids else None
        changes = []
        if install_name:
            new_id = '@rpath/' + os.path.relpath(binary, frameworks)
            if new_id != install_name:
                changes.extend(['-id', new_id])

        for dependency in getLinkedLibs(binary):
            reference = str(dependency)
            if reference == install_name or not dependency.is_absolute():
                continue
            if reference.startswith(('/System/Library/', '/usr/lib/')):
                continue
            if not dependency.is_file() or not isBinary(dependency):
                raise RuntimeError(f'unusable runtime dependency: {binary}: {dependency}')
            source = dependency.resolve(strict=True)
            if source not in system_replacements:
                system_replacements[source] = _systemDependencyReplacement(dependency)
            replacement = system_replacements[source]
            if (python_binary is not None
                    and dependency.name == f'libpython{python_binary.parent.name}.dylib'):
                replacement = '@loader_path/' + os.path.relpath(python_binary, binary.parent)
            if replacement is None:
                destination = destination_for(dependency)
                if source not in source_hashes:
                    source_hashes[source] = digest(source)
                if destination.is_file():
                    real_destination = destination.resolve(strict=True)
                    if original_hashes.get(real_destination) != source_hashes[source]:
                        if (_dependencyFrameworkRoot(dependency) is not None
                                or destination not in copied):
                            raise RuntimeError(
                                f'conflicting runtime dependency: {dependency} -> {destination}'
                            )
                        destination = frameworks.joinpath(source_hashes[source].hex(), dependency.name)
                        if (destination.exists()
                                and original_hashes.get(destination.resolve()) != source_hashes[source]):
                            raise RuntimeError(f'conflicting runtime dependency: {dependency} -> {destination}')
                if not destination.is_file():
                    framework = _dependencyFrameworkRoot(dependency)
                    if framework is not None:
                        copyDirSub(framework, frameworks, only_contents=False)
                        for path in sorted(frameworks.joinpath(framework.name).rglob('*')):
                            register(path)
                    else:
                        destination.parent.mkdir(parents=True, exist_ok=True)
                        shutil.copy2(source, destination)
                        destination.chmod(destination.stat().st_mode | stat.S_IWUSR)
                        register(destination)
                    if not destination.is_file() or not isBinary(destination):
                        raise RuntimeError(f'copied runtime dependency is unusable: {dependency}')
                    copied.append(destination)
                destinations_by_source[source] = destination
                replacement = '@loader_path/' + os.path.relpath(destination, binary.parent)
            changes.extend(['-change', reference, replacement])
        if changes:
            subprocess.run(['install_name_tool', *changes, binary], check=True)
        inspected += 1
        if inspected % 100 == 0 or not pending:
            print(f'Dependency closure: inspected {inspected}, pending {len(pending)}', flush=True)
    return tuple(copied)


def kritaCreatePyKrita(src: pathlib.Path, dst: pathlib.Path, version: str):
    frame_name = "PyKrita"
    frame_version = version

    frame_loc = dict()
    frame_root = dst.joinpath(frame_name + ".framework") # PyKrita.framework
    frame_loc['root'] = frame_root
    frame_loc['versions'] = frame_root.joinpath('Versions') # PyKrita.framework/Versions
    frame_loc[frame_version] = frame_loc['versions'].joinpath(frame_version) # PyKrita.framework/Versions/x.y.z

    for key in frame_loc:
        frame_loc[key].mkdir(exist_ok=True)
    for name in ['Resources','lib']:
        frame_loc[frame_version].joinpath(name).mkdir(exist_ok=True)

    copyDirSub(src.joinpath('lib', 'krita-python-libs'),frame_loc[frame_version].joinpath('lib'))
    krita_so = pathlib.Path('lib', 'PyKrita', 'krita.so')
    shutil.move(frame_loc[frame_version].joinpath(krita_so),frame_loc[frame_version].joinpath(frame_name))

    # Create symlinks
    frame_loc[frame_version].joinpath(krita_so).symlink_to(pathlib.Path('..', '..', frame_name))
    frame_loc['versions'].joinpath('Current').symlink_to(frame_version)

    frame_loc['root'].joinpath(frame_name).symlink_to(pathlib.Path('Versions','Current',frame_name))
    frame_loc['root'].joinpath('Resources').symlink_to(pathlib.Path('Versions','Current','Resources'))

    krita_python_lib = dst.joinpath('krita-python-libs')
    krita_python_link = frame_loc['versions'].joinpath('Current', 'lib').relative_to(dst)
    krita_python_lib.symlink_to(krita_python_link)

    info_plist = frame_loc[frame_version].joinpath('Resources', 'Info.plist')

    plistbuddy_loc = pathlib.Path('/', 'usr', 'libexec', 'PlistBuddy')
    plistbuddy = lambda key, value: subprocess.run([plistbuddy_loc, info_plist, '-c', f'Add:{key} string {value}'])

    plistbuddy("CFBundleExecutable", frame_name)
    plistbuddy("CFBundleIdentifier", 'local.librepaint.macos.pykrita')
    plistbuddy("CFBundlePackageType", "FMWK")
    plistbuddy("CFBundleShortVersionString", f'{frame_version}')
    plistbuddy("CFBundleVersion", f'{frame_version}')

    return


# TODO: test if frame_python_lib is actually pointing to 'pythonx.y'
def kritaStripPythonFramework(frameworkPath: pathlib.Path):
    print("Removing unnecessary files from Python.Framework to be packaged...")
    frame_python_lib = next(frameworkPath.joinpath('Versions','Current', 'lib').glob('python*'),
                            frameworkPath.joinpath('Versions','Current', 'lib','python3.10'))
    print(f'found frame_python_lib: {frame_python_lib}')

    relocatePythonStandardLibrary(frame_python_lib)

    files_for_rm = list()
    files_for_rm.extend([file for file in frameworkPath.rglob("test*") if file.is_dir() and file.name in ['test', 'tests']])
    files_for_rm.extend([file for file in frameworkPath.joinpath('Versions','Current', 'bin').rglob("*")
                         if not file.name.startswith('python')])

    files_for_rm.append(frame_python_lib.joinpath('bin', 'python3-intel64'))

    for name in "tkinter ensurepip distutils lib2to3 turtledemo idlelib".split():
        files_for_rm.append(frame_python_lib.joinpath(name))
    for pattern in "pip* PyQt_builder* setuptools* sip* easy-install.pth".split():
        files_for_rm.extend(frame_python_lib.joinpath('site-packages').glob(pattern))

    runtime_pyqt_modules = {
        'QtCore',
        'QtGui',
        'QtNetwork',
        'QtQml',
        'QtWidgets',
        'QtXml',
        'sip',
    }
    pyqt_root = frame_python_lib.joinpath('site-packages', 'PyQt6')
    files_for_rm.extend(
        path
        for path in pyqt_root.glob('*.so')
        if path.name.split('.', 1)[0] not in runtime_pyqt_modules
    )

    # removal of Python.app
    files_for_rm.append(frameworkPath.joinpath('Versions','Current', 'Resources','Python.app'))

    for file in files_for_rm:
        if file.exists(follow_symlinks=False):
            if file.is_dir():
                shutil.rmtree(file, ignore_errors=True)
            else:
                file.unlink(missing_ok=True)
        else:
            print(f'path does not exist: {file}')

    return


def installPythonFrameworkInfo(framework: pathlib.Path) -> pathlib.Path:
    """Install the metadata required to sign the bundled Python framework."""
    version = framework.joinpath('Versions', 'Current').readlink().name
    info_plist = framework.joinpath(
        'Versions', version, 'Resources', 'Info.plist'
    )
    info_plist.parent.mkdir(parents=True, exist_ok=True)
    with info_plist.open('wb') as handle:
        plistlib.dump(
            {
                'CFBundleDevelopmentRegion': 'English',
                'CFBundleExecutable': 'Python',
                'CFBundleIdentifier': 'org.python.python',
                'CFBundleInfoDictionaryVersion': '6.0',
                'CFBundleName': 'Python',
                'CFBundlePackageType': 'FMWK',
                'CFBundleShortVersionString': version,
                'CFBundleVersion': version,
            },
            handle,
        )
    return info_plist


def relocatePythonStandardLibrary(frame_python_lib: pathlib.Path):
    """Replace Nix build-host paths that Python would use at runtime."""
    replacements = {
        frame_python_lib.joinpath('subprocess.py'): (
            re.compile(r"/nix/store/[0-9a-z]+-[^/'\"\s]+/bin/sh"),
            "/bin/sh",
        ),
        frame_python_lib.joinpath('mimetypes.py'): (
            re.compile(
                r"/nix/store/[0-9a-z]+-[^/'\"\s]+/etc/mime\.types"
            ),
            "/etc/apache2/mime.types",
        ),
    }
    changed = []
    for path, (pattern, replacement) in replacements.items():
        if not path.is_file():
            continue
        contents = path.read_text()
        relocated = pattern.sub(replacement, contents)
        if relocated != contents:
            path.write_text(relocated)
            changed.append(path)
    return tuple(changed)


def installFontconfigConfiguration(
        source: pathlib.Path,
        resources: pathlib.Path,
        ) -> pathlib.Path:
    """Install the host-independent macOS font search contract."""
    destination = resources.joinpath('fontconfig', 'fonts.conf')
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(
        source.joinpath('packaging', 'macos', 'fonts.conf'),
        destination,
    )
    return destination


def installQtConfiguration(
        source: pathlib.Path,
        resources: pathlib.Path,
        ) -> pathlib.Path:
    """Install relocatable Qt plugin and QML search locations."""
    destination = resources.joinpath('qt.conf')
    shutil.copy2(
        source.joinpath('packaging', 'macos', 'qt.conf'),
        destination,
    )
    return destination


def _removePath(path: pathlib.Path) -> bool:
    if path.is_symlink() or path.is_file():
        path.unlink()
        return True
    if path.is_dir():
        shutil.rmtree(path)
        return True
    return False


def makeTreeOwnerWritable(root: pathlib.Path) -> None:
    """Allow deployment to transform files copied from an immutable Nix output."""
    for path in (root, *root.rglob('*')):
        if not path.is_symlink():
            path.chmod(path.stat().st_mode | stat.S_IWUSR)


def installBundleLauncher(
        source: pathlib.Path,
        macos: pathlib.Path,
        executable_name: str,
        wrapped_source: pathlib.Path | None = None,
        ) -> pathlib.Path:
    """Build a relocatable launcher for a bundled Mach-O executable."""
    executable = macos.joinpath(executable_name)
    wrapped = macos.joinpath(f'.{executable_name}-wrapped')
    if wrapped_source is not None:
        shutil.copy2(wrapped_source, wrapped)
    if not wrapped.is_file():
        raise FileNotFoundError(f"bundled executable is missing: {wrapped}")
    wrapped.chmod(wrapped.stat().st_mode | stat.S_IWUSR | stat.S_IXUSR)
    executable.unlink(missing_ok=True)
    command = [
        '/usr/bin/clang',
        '-Os',
        f'-DBUNDLED_EXECUTABLE="{wrapped.name}"',
        source.joinpath('packaging', 'macos', 'macos-bundle-launcher.c'),
        '-o',
        executable,
    ]
    cmdLog(command)
    subprocess.run(command, check=True)
    return executable


def removeDeploymentOnlyPayload(app: pathlib.Path) -> tuple[pathlib.Path, ...]:
    """Remove development and test artifacts from the runtime payload."""
    removable_paths = (
        'Contents/Frameworks/QtQuickTest.framework',
        'Contents/Frameworks/QtTest.framework',
        'Contents/PlugIns/permissions',
        'Contents/PlugIns/qmllint',
        'Contents/PlugIns/qmlls',
        'Contents/PlugIns/qmltooling',
        'Contents/Resources/qml/Qt/test',
        'Contents/Resources/qml/QtTest',
    )
    removed = []
    for relative_path in removable_paths:
        path = app.joinpath(relative_path)
        if _removePath(path):
            removed.append(path)

    frameworks = app.joinpath('Contents', 'Frameworks')
    header_paths = sorted(
        (
            path
            for path in frameworks.rglob('Headers')
            if path.is_dir() or path.is_symlink()
        ),
        key=lambda path: len(path.parts),
        reverse=True,
    )
    for path in header_paths:
        if _removePath(path):
            removed.append(path)

    python_framework = frameworks.joinpath('Python.framework', 'Versions')
    pyqt_roots = set(
        python_framework.glob('*/lib/python*/site-packages/PyQt6')
    )
    for pyqt_root in sorted(pyqt_roots):
        python_development_paths = [
            pyqt_root.joinpath('bindings'),
            pyqt_root.joinpath('py.typed'),
            *pyqt_root.glob('*.pyi'),
            *pyqt_root.glob('QtTest*'),
        ]
        for path in python_development_paths:
            if _removePath(path):
                removed.append(path)

    development_suffixes = ('.a', '.cmake', '.la', '.o', '.pc', '.prl')
    for path in sorted(app.joinpath('Contents').rglob('*')):
        if path.is_file() and path.name.casefold().endswith(development_suffixes):
            path.unlink()
            removed.append(path)

    return tuple(sorted(removed))


def _bundleRpathIsValid(
        app: pathlib.Path, binary: pathlib.Path, rpath: str) -> bool:
    if '/nix/store/' in rpath:
        return False
    if rpath.startswith((
            '/System/Library/Frameworks/',
            '/System/Library/PrivateFrameworks/',
            '/usr/lib/',
    )):
        return True

    token_bases = (
        ('@loader_path', binary.parent),
        ('@executable_path', app.joinpath('Contents', 'MacOS')),
    )
    expanded = None
    for token, base in token_bases:
        if rpath == token:
            expanded = base
            break
        prefix = token + '/'
        if rpath.startswith(prefix):
            expanded = base.joinpath(rpath.removeprefix(prefix))
            break
    if expanded is None:
        return False

    app_root = app.resolve()
    expanded = expanded.resolve(strict=False)
    try:
        expanded.relative_to(app_root)
    except ValueError:
        return False
    return expanded.is_dir()


def cleanInvalidBundleRpaths(
        app: pathlib.Path,
        binaries: list[pathlib.Path],
        ) -> tuple[tuple[pathlib.Path, str], ...]:
    """Delete RPATH entries that cannot resolve inside the deployed bundle."""
    removed = []
    for binary in binaries:
        result = subprocess.run(
            ['otool', '-l', binary], capture_output=True, text=True, check=True)
        expecting_path = False
        rpaths = []
        for line in result.stdout.splitlines():
            stripped = line.strip()
            if stripped == 'cmd LC_RPATH':
                expecting_path = True
                continue
            if expecting_path and stripped.startswith('path '):
                rpaths.append(stripped.removeprefix('path ').rsplit(' (offset ', 1)[0])
                expecting_path = False

        for rpath in rpaths:
            if _bundleRpathIsValid(app, binary, rpath):
                continue
            subprocess.run(['install_name_tool', '-delete_rpath', rpath, binary], check=True)
            removed.append((binary, rpath))
    return tuple(removed)


def signAppBundle(app: pathlib.Path, signing_identity: str) -> None:
    """Sign the completed bundle with the selected identity."""
    subprocess.run(
        ['codesign', '--force', '--deep', '--sign', signing_identity, app],
        check=True,
    )


def auditAppBundle(
        app: pathlib.Path,
        source: pathlib.Path,
        *,
        verify_signature: bool,
        ) -> None:
    """Apply the release bundle contract before and after signing."""
    command = [
        sys.executable,
        source.joinpath('scripts', 'platform', 'audit-macos-bundle.py'),
        app,
    ]
    if not verify_signature:
        command.append('--skip-signature')
    subprocess.run(command, check=True)



def kritaDeploy(
        from_install: pathlib.Path,
        dst: pathlib.Path,
        source: pathlib.Path,
        signing_identity: str = '-',
        ):

    krita_dmg = dst
    krita_install_dir = from_install
    krita_source_dir = source

    krita_app = dict()
    krita_app['root'] = pathlib.Path(os.path.join(krita_dmg, "LibrePaint.app"))
    krita_app['contents'] = krita_app['root'].joinpath("Contents")
    krita_app['plugins'] = pathlib.Path(os.path.join(krita_app['contents'], 'PlugIns'))
    krita_app['frameworks'] = pathlib.Path(os.path.join(krita_app['contents'], 'Frameworks'))
    krita_app['macos'] = pathlib.Path(os.path.join(krita_app['contents'], 'MacOS'))
    krita_app['resources'] = pathlib.Path(os.path.join(krita_app['contents'], 'Resources'))

    with krita_install_dir.joinpath('bin', 'LibrePaint.app', 'Contents', 'Info.plist').open('rb') as handle:
        bundle_info = plistlib.load(handle)
    bundle_version = bundle_info['CFBundleShortVersionString']
    source_app = krita_install_dir.joinpath('bin', 'LibrePaint.app')


    if krita_dmg.exists():
        print(f"Deleting previous LibrePaint.app run in {krita_dmg}")
        makeTreeOwnerWritable(krita_dmg)
        shutil.rmtree(krita_dmg)

    print(f"Preparing {krita_install_dir} for deployment")
    krita_dmg.mkdir(exist_ok=True)

    for key in krita_app:
        krita_app[key].mkdir(exist_ok=True, parents=True)

    print("copying LibrePaint.app...")
    copyDirSub(krita_install_dir.joinpath('bin', 'LibrePaint.app'), krita_app['root'], only_contents=True)
    makeTreeOwnerWritable(krita_app['root'])
    installBundleLauncher(
        krita_source_dir,
        krita_app['macos'],
        'LibrePaint',
    )
    runner_source = krita_install_dir.joinpath('bin', '.kritarunner-wrapped')
    if not runner_source.is_file():
        runner_source = krita_install_dir.joinpath('bin', 'kritarunner')
    installBundleLauncher(
        krita_source_dir,
        krita_app['macos'],
        'kritarunner',
        runner_source,
    )

    print("Copying share...")
    extra_args = [     '--delete'
                       ,'--exclude', 'LibrePaint.icns'
                       ,'--exclude', 'krita-krz.icns'
                       ,'--exclude', 'krita-kra.icns'
                       ,'--exclude', 'Assets.car'
                       ,'--exclude', 'aclocal'
                       ,'--exclude', 'doc'
                       ,'--exclude', 'ECM'
                       ,'--exclude', 'eigen3'
                       ,'--exclude', 'emacs'
                       ,'--exclude', 'gettext'
                       ,'--exclude', 'gettext-0.19.8'
                       ,'--exclude', 'info'
                       ,'--exclude', 'kf5'
                       ,'--exclude', 'kservices5'
                       ,'--exclude', 'man'
                       ,'--exclude', 'ocio'
                       ,'--exclude', 'pkgconfig'
                       ,'--exclude', 'mime'
                       ,'--exclude', 'translations'
                       ,'--exclude', 'qml'
                        ]
    runtime_share = krita_app['resources'].joinpath('share')
    runtime_share.mkdir()
    copyDirSub(krita_install_dir.joinpath('share'), runtime_share, extra_args=extra_args)
    installFontconfigConfiguration(krita_source_dir, krita_app['resources'])
    installQtConfiguration(krita_source_dir, krita_app['resources'])

    print("Copying Qt translations...")
    qt_translations = krita_install_dir.joinpath('translations')
    if qt_translations.is_dir():
        copyDirSub(qt_translations, krita_app['contents'], only_contents=False)
    else:
        print(f"Optional Qt translations are not installed at {qt_translations}")

    symlinks = [
        ('share', 'Resources/share'),
        ('lib', 'Frameworks'),
        ('Resources/kritaplugins', '../PlugIns/kritaplugins'),
    ]
    if qt_translations.is_dir():
        symlinks.append(('Resources/translations', '../translations'))
    for src,dst in symlinks:
        linkPath = krita_app['contents'].joinpath(src)
        if linkPath.is_symlink():
            linkPath.unlink()
        linkPath.symlink_to(dst)

    print("Copying mandatory libs...")
    pattern = ['libKF5*', 'libkrita*']
    mandatoryLibs = list()
    for pat in pattern:
        mandatoryLibs.extend(krita_install_dir.joinpath('lib').glob(pat))
    for file in mandatoryLibs:
        shutil.copy2(file,krita_app['frameworks'], follow_symlinks=False)

    print("Copying plugins...")
    # Reused install prefixes may still contain removed optional Finder integration artifacts.
    extra_args = [
        '--delete',
        '--delete-excluded',
        '--exclude', 'kritaspotlight.mdimporter',
        '--exclude', 'krita-thumbnailer.appex',
        '--exclude', 'krita-preview.appex',
    ]
    qt_plugins = krita_install_dir.joinpath('plugins')
    if qt_plugins.is_dir():
        copyDirSub(qt_plugins, krita_app['plugins'], extra_args=extra_args)
    else:
        print(f"Qt plugins will be copied from the runtime closure; {qt_plugins} is absent")

    print("Copying kritaplugins...")
    krita_plugins = krita_app['plugins'].joinpath('kritaplugins')
    krita_plugins.mkdir()
    copyDirSub(krita_install_dir.joinpath('lib', 'kritaplugins'), krita_plugins)

    qt_plugin_roots = copyNixQtPlugins(krita_install_dir, krita_app['plugins'])
    print(f"Copied {len(qt_plugin_roots)} Qt plugin roots")

    qml_roots = copyNixQmlRuntime(krita_install_dir, krita_app['resources'])
    print(f"Copied {len(qml_roots)} QML runtime roots")

    runtime_plugin_sources = {
        'mlt': source_app.joinpath('Contents', 'PlugIns', 'mlt'),
        'frei0r-1': source_app.joinpath('Contents', 'PlugIns', 'frei0r-1'),
    }
    for name, source_path in runtime_plugin_sources.items():
        destination = krita_app['plugins'].joinpath(name)
        _removePath(destination)
        destination.mkdir()
        # Resolve the Nix output's outer symlink while preserving links owned
        # by the runtime directory itself.
        copyDirSub(
            source_path.resolve(strict=True),
            destination,
            only_contents=True,
        )

    mlt_resources = krita_app['resources'].joinpath('mlt')
    _removePath(mlt_resources)
    mlt_resources.mkdir()
    copyDirSub(
        source_app.joinpath('Contents', 'Resources', 'mlt').resolve(strict=True),
        mlt_resources,
        only_contents=True,
    )

    for name in ['ffmpeg', 'ffprobe']:
        destination = krita_app['macos'].joinpath(name)
        _removePath(destination)
        shutil.copy2(
            source_app.joinpath('Contents', 'MacOS', name).resolve(strict=True),
            destination,
        )

    print("Copying python...")
    python_framework = krita_app['frameworks'].joinpath('Python.framework')
    python_framework.mkdir()
    copyDirSub(
        krita_install_dir.joinpath('lib', 'Python.framework').resolve(strict=True),
        python_framework,
        only_contents=True,
    )
    installPythonFrameworkInfo(python_framework)
    kritaCreatePyKrita(krita_install_dir, krita_app['frameworks'], bundle_version)

    makeTreeOwnerWritable(krita_app['root'])

    DeployCmd.achmod(krita_app['frameworks'].joinpath('Python.framework','Python'), stat.S_IWRITE)

    kritaStripPythonFramework(krita_app['frameworks'].joinpath('Python.framework'))
    print("precompiling all python files")
    cmd = [sys.executable, '-m', 'compileall', krita_app['contents']]
    cmdLog(cmd)
    subprocess.run(cmd, check=True)

    # Fix file permissions
    filesToFix = list()
    filesToFix.extend(krita_app['contents'].rglob('*.dylib'))
    filesToFix.extend(krita_app['contents'].rglob('*.so'))
    filesToFix.extend(krita_app['macos'].rglob('*'))
    for f in filesToFix:
        DeployCmd.achmod(f,0o111)
    for f in krita_app['resources'].joinpath('applications').rglob('*.desktop'):
        DeployCmd.xchmod(f,0o111)


    sdl3_runtime = installSdl3Runtime(
        krita_install_dir,
        krita_app['frameworks'],
    )
    print(f"Installed SDL3 runtime at {sdl3_runtime}")
    copied_nix = copyNixStoreDependencyClosure(krita_app['root'])
    print(f"Copied and relocated {len(copied_nix)} runtime dependencies")
    removed_payload = removeDeploymentOnlyPayload(krita_app['root'])
    print(f"Removed {len(removed_payload)} deployment-only paths")
    # Remove broken symlinks if any
    filesToFix = [f for f in krita_app['contents'].rglob('*') if f.is_symlink() and not f.exists()]
    for f in filesToFix:
        f.unlink()

    # Keep only relocatable RPATH entries that resolve within the final bundle.
    filesToFix =[f for f in krita_app['contents'].rglob('*') if
                  (f.is_file() and (stat.S_IMODE(f.stat().st_mode) & 0o111) and f.suffix != '.py')
                  or f.suffix == '.dylib'
                  or f.suffix == '.so'
                  ]
    filesToFix = [f for f in filesToFix if isBinary(f)]
    removed_rpaths = cleanInvalidBundleRpaths(krita_app['root'], filesToFix)
    print(f"Removed {len(removed_rpaths)} invalid RPATH entries")

    # delete .DS_Store if any
    for f in krita_app['contents'].rglob('*.DS_Store'):
        f.unlink()

    auditAppBundle(
        krita_app['root'],
        krita_source_dir,
        verify_signature=False,
    )
    signAppBundle(krita_app['root'], signing_identity)
    auditAppBundle(
        krita_app['root'],
        krita_source_dir,
        verify_signature=True,
    )

    print("## Finished preparing LibrePaint.app bundle!")

    return



if __name__ == '__main__':
    main()
