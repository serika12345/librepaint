#!/usr/bin/env python3

import argparse
import os
import plistlib
import stat
import sys
import zipfile


PAYLOAD_ARCHIVE_ROOT = "Payload"
MAX_REPORTED_ERRORS = 20
FORBIDDEN_METADATA_COMPONENTS = {
    "__MACOSX",
    "_CodeSignature",
    ".DS_Store",
    "embedded.mobileprovision",
}


class ValidationError(Exception):
    pass


def format_errors(summary, errors):
    shown = errors[:MAX_REPORTED_ERRORS]
    details = "\n".join(f"  {error}" for error in shown)
    if len(errors) > len(shown):
        details += f"\n  ... {len(errors) - len(shown)} more errors"
    return f"{summary}:\n{details}"


def contains_control_character(value):
    return any(ord(character) < 0x20 or ord(character) == 0x7F for character in value)


def validate_executable_name(value, context):
    if (
        not isinstance(value, str)
        or not value
        or value in {".", ".."}
        or "/" in value
        or "\\" in value
        or contains_control_character(value)
    ):
        raise ValidationError(f"invalid {context} CFBundleExecutable: {value!r}")
    return value


def validate_app_bundle_name(value, context):
    if (
        not isinstance(value, str)
        or value in {"", ".", "..", ".app"}
        or not value.endswith(".app")
        or "/" in value
        or "\\" in value
        or contains_control_character(value)
    ):
        raise ValidationError(f"invalid {context} app bundle name: {value!r}")
    return value


def validate_staged_component(path):
    component = os.path.basename(path)
    if "\\" in component or contains_control_character(component):
        raise ValidationError(f"unsafe IPA path component: {component!r}")
    if component in FORBIDDEN_METADATA_COMPONENTS or component.startswith("._"):
        raise ValidationError(f"signing or Finder metadata in IPA stage: {path}")


def inventory_app(bundle):
    directories = []
    regular_files = {}

    def inventory(path):
        validate_staged_component(path)
        try:
            mode = os.lstat(path).st_mode
        except OSError as error:
            raise ValidationError(f"cannot inspect IPA path {path}: {error}") from error
        if stat.S_ISDIR(mode):
            directories.append(path)
            try:
                with os.scandir(path) as entries:
                    children = sorted((entry.path for entry in entries), key=os.fsencode)
            except OSError as error:
                raise ValidationError(f"cannot inspect IPA directory {path}: {error}") from error
            for child in children:
                inventory(child)
        elif stat.S_ISREG(mode):
            regular_files[path] = mode
        else:
            raise ValidationError(f"unsupported IPA entry type: {path}")

    inventory(bundle)
    return directories, regular_files


def read_bundle_executable(bundle):
    info_path = os.path.join(bundle, "Info.plist")
    try:
        with open(info_path, "rb") as handle:
            info = plistlib.load(handle)
    except (OSError, plistlib.InvalidFileException) as error:
        raise ValidationError(f"cannot read staged Info.plist: {error}") from error
    if not isinstance(info, dict):
        raise ValidationError("staged Info.plist root is not a dictionary")
    return validate_executable_name(info.get("CFBundleExecutable"), "staged")


def normalize_app(bundle):
    bundle = os.path.abspath(bundle)
    directories, regular_files = inventory_app(bundle)
    executable = read_bundle_executable(bundle)
    executable_path = os.path.join(bundle, executable)
    if executable_path not in regular_files:
        raise ValidationError(f"main executable is not a regular file: {executable_path}")

    executable_files = {
        path for path, mode in regular_files.items() if stat.S_IMODE(mode) & 0o111
    }
    unexpected_executables = sorted(
        executable_files.difference({executable_path}), key=os.fsencode
    )
    if unexpected_executables:
        raise ValidationError(
            format_errors("unexpected executable files in IPA stage", unexpected_executables)
        )

    # Apply no changes until the complete tree and executable inventory pass.
    try:
        for path in directories:
            os.chmod(path, 0o755)
        for path in regular_files:
            os.chmod(path, 0o755 if path == executable_path else 0o644)
    except OSError as error:
        raise ValidationError(f"cannot normalize IPA stage permissions: {error}") from error

    errors = []
    try:
        for path in directories:
            mode = stat.S_IMODE(os.lstat(path).st_mode)
            if mode != 0o755:
                errors.append(f"{path}: mode {mode:#05o}; expected 0o755")
        for path in regular_files:
            mode = stat.S_IMODE(os.lstat(path).st_mode)
            expected = 0o755 if path == executable_path else 0o644
            if mode != expected:
                errors.append(f"{path}: mode {mode:#05o}; expected {expected:#05o}")
    except OSError as error:
        raise ValidationError(f"cannot verify IPA stage permissions: {error}") from error
    if errors:
        raise ValidationError(format_errors("failed to normalize IPA stage", errors))

    print(
        "normalized IPA stage permissions: "
        f"{len(directories)} directories, "
        f"{len(regular_files) - 1} data files, 1 executable file"
    )


def validate_archive_name(name):
    stripped_name = name[:-1] if name.endswith("/") else name
    components = stripped_name.split("/")
    return not (
        name.startswith("/")
        or "\\" in name
        or contains_control_character(name)
        or any(component in {"", ".", ".."} for component in components)
    )


def expected_archive_names(staged_app):
    staged_app = os.path.abspath(staged_app)
    bundle_name = validate_app_bundle_name(
        os.path.basename(staged_app), "staged"
    )
    app_archive_root = f"{PAYLOAD_ARCHIVE_ROOT}/{bundle_name}"
    directories, regular_files = inventory_app(staged_app)
    expected = {f"{PAYLOAD_ARCHIVE_ROOT}/"}
    for path in directories:
        relative = os.path.relpath(path, staged_app)
        archive_path = app_archive_root
        if relative != ".":
            archive_path += "/" + relative.replace(os.sep, "/")
        expected.add(archive_path + "/")
    for path in regular_files:
        relative = os.path.relpath(path, staged_app).replace(os.sep, "/")
        expected.add(f"{app_archive_root}/{relative}")
    return expected


def detect_archive_app_root(entries, errors):
    candidates = []
    for entry in entries:
        stripped_name = (
            entry.filename[:-1]
            if entry.filename.endswith("/")
            else entry.filename
        )
        components = stripped_name.split("/")
        if (
            len(components) == 2
            and components[0] == PAYLOAD_ARCHIVE_ROOT
            and components[1].endswith(".app")
        ):
            candidates.append(entry)

    if len(candidates) != 1:
        errors.append(
            "expected exactly one app bundle directory directly under Payload; "
            f"found {len(candidates)}"
        )
        return None

    app_entry = candidates[0]
    app_archive_root = app_entry.filename.rstrip("/")
    bundle_name = app_archive_root.split("/", 1)[1]
    try:
        validate_app_bundle_name(bundle_name, "archived")
    except ValidationError as error:
        errors.append(str(error))

    archive_mode = app_entry.external_attr >> 16
    if not app_entry.filename.endswith("/") or not stat.S_ISDIR(archive_mode):
        errors.append(
            "app bundle entry is not a regular directory: "
            f"{app_entry.filename}"
        )
    return app_archive_root


def check_ipa(ipa, staged_app=None):
    try:
        archive = zipfile.ZipFile(ipa)
    except (OSError, zipfile.BadZipFile) as error:
        raise ValidationError(f"cannot open IPA {ipa}: {error}") from error

    with archive:
        entries = archive.infolist()
        names = [entry.filename for entry in entries]
        errors = []

        if not names:
            errors.append("archive is empty")
        if len(names) != len(set(names)):
            errors.append("archive contains duplicate entry names")
        invalid_names = [name for name in names if not validate_archive_name(name)]
        errors.extend(f"unsafe entry name: {name!r}" for name in invalid_names)
        if names != sorted(names):
            errors.append("central directory is not byte-order sorted")
        if archive.comment:
            errors.append("archive comment is present")

        app_archive_root = detect_archive_app_root(entries, errors)

        if staged_app is not None:
            expected_names = expected_archive_names(staged_app)
            missing_names = sorted(expected_names.difference(names))
            unexpected_names = sorted(set(names).difference(expected_names))
            errors.extend(
                f"staged entry is missing from archive: {name}"
                for name in missing_names
            )
            errors.extend(
                f"archive entry is absent from stage: {name}"
                for name in unexpected_names
            )

        allowed_roots = {f"{PAYLOAD_ARCHIVE_ROOT}/"}
        if app_archive_root is not None:
            allowed_roots.add(f"{app_archive_root}/")
        for name in names:
            if name not in allowed_roots and (
                app_archive_root is None
                or not name.startswith(f"{app_archive_root}/")
            ):
                errors.append(f"entry is outside the single app bundle: {name}")
        for required_directory in allowed_roots:
            if required_directory not in names:
                errors.append(f"explicit directory entry is missing: {required_directory}")

        name_set = set(names)
        for name in names:
            stripped_name = name[:-1] if name.endswith("/") else name
            components = stripped_name.split("/")
            for index in range(1, len(components)):
                parent = "/".join(components[:index]) + "/"
                if parent not in name_set:
                    errors.append(f"explicit parent directory is missing for {name}: {parent}")
                    break

            basename = components[-1]
            if (
                any(
                    component in FORBIDDEN_METADATA_COMPONENTS
                    for component in components
                )
                or basename.startswith("._")
            ):
                errors.append(f"signing or Finder metadata is present: {name}")

        executable_entry = None
        if app_archive_root is not None:
            info_plist_entry = f"{app_archive_root}/Info.plist"
            try:
                info = plistlib.loads(archive.read(info_plist_entry))
                if not isinstance(info, dict):
                    raise ValidationError(
                        "archived Info.plist root is not a dictionary"
                    )
                executable = validate_executable_name(
                    info.get("CFBundleExecutable"), "archived"
                )
                executable_entry = f"{app_archive_root}/{executable}"
            except (
                KeyError,
                OSError,
                RuntimeError,
                plistlib.InvalidFileException,
                zipfile.BadZipFile,
            ) as error:
                errors.append(f"cannot read archived Info.plist: {error}")
            except ValidationError as error:
                errors.append(str(error))

        if executable_entry is not None and executable_entry not in name_set:
            errors.append(f"main executable entry is missing: {executable_entry}")

        for entry in entries:
            archive_mode = entry.external_attr >> 16
            permission = stat.S_IMODE(archive_mode)
            if entry.create_system != 3:
                errors.append(
                    f"{entry.filename}: ZIP creator is not Unix ({entry.create_system})"
                )
            if entry.is_dir():
                expected_permission = 0o755
                if not stat.S_ISDIR(archive_mode):
                    errors.append(
                        f"{entry.filename}: missing directory type in ZIP mode "
                        f"{archive_mode:#07o}"
                    )
            else:
                expected_permission = (
                    0o755 if entry.filename == executable_entry else 0o644
                )
                if not stat.S_ISREG(archive_mode):
                    errors.append(
                        f"{entry.filename}: unsupported ZIP file type {archive_mode:#07o}"
                    )
            if permission != expected_permission:
                errors.append(
                    f"{entry.filename}: mode {permission:#05o}; "
                    f"expected {expected_permission:#05o}"
                )
            if entry.external_attr & 0x1:
                errors.append(f"{entry.filename}: ZIP DOS read-only attribute is set")
            if entry.extra:
                errors.append(f"{entry.filename}: ZIP extra metadata is present")
            if entry.comment:
                errors.append(f"{entry.filename}: ZIP entry comment is present")
            if entry.flag_bits & 0x1:
                errors.append(f"{entry.filename}: encrypted ZIP entry is not allowed")

        if errors:
            raise ValidationError(format_errors("IPA validation failed", errors))

        directory_count = sum(entry.is_dir() for entry in entries)
        file_count = len(entries) - directory_count
        print(
            "IPA permissions verified: "
            f"{directory_count} directories at 0o755, "
            f"{file_count - 1} data files at 0o644, "
            "1 executable file at 0o755, DOS read-only clear"
        )


def make_tree_removable(root):
    """Make only real directories below a known temporary root owner-writable."""
    root = os.path.abspath(root)
    try:
        root_mode = os.lstat(root).st_mode
    except FileNotFoundError:
        return
    except OSError as error:
        raise ValidationError(f"cannot inspect cleanup root {root}: {error}") from error
    if not stat.S_ISDIR(root_mode):
        raise ValidationError(f"cleanup root is not a directory: {root}")

    pending = [root]
    while pending:
        directory = pending.pop()
        try:
            mode = os.lstat(directory).st_mode
            os.chmod(directory, stat.S_IMODE(mode) | 0o700)
            with os.scandir(directory) as entries:
                for entry in entries:
                    entry_mode = entry.stat(follow_symlinks=False).st_mode
                    if stat.S_ISDIR(entry_mode):
                        pending.append(entry.path)
        except OSError as error:
            raise ValidationError(
                f"cannot make temporary IPA stage removable at {directory}: {error}"
            ) from error


def parse_args():
    parser = argparse.ArgumentParser(
        description="Normalize and verify Krita iPadOS IPA permissions"
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    normalize_parser = subparsers.add_parser(
        "normalize-app", help="normalize a staged .app tree"
    )
    normalize_parser.add_argument("app_path")

    check_parser = subparsers.add_parser(
        "check-ipa", help="verify a completed IPA archive"
    )
    check_parser.add_argument("ipa_path")

    check_parser.add_argument(
        "--staged-app",
        help="require the archive inventory to exactly match this staged .app",
    )

    cleanup_parser = subparsers.add_parser(
        "make-tree-removable",
        help="make directories in a private staging tree owner-writable for cleanup",
    )
    cleanup_parser.add_argument("root_path")
    return parser.parse_args()


def main():
    args = parse_args()
    try:
        if args.command == "normalize-app":
            normalize_app(args.app_path)
        elif args.command == "check-ipa":
            check_ipa(args.ipa_path, args.staged_app)
        else:
            make_tree_removable(args.root_path)
    except ValidationError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
