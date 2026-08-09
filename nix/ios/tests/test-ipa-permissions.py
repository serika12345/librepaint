#!/usr/bin/env python3

import copy
import os
import plistlib
import shutil
import stat
import subprocess
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path


HELPER = Path(sys.argv.pop(1)).resolve()
ZIP = Path(sys.argv.pop(1)).resolve()


class IPAPermissionsTest(unittest.TestCase):
    def run_helper(self, *arguments, succeeds=True):
        result = subprocess.run(
            [sys.executable, str(HELPER), *map(str, arguments)],
            check=False,
            capture_output=True,
            text=True,
        )
        if succeeds:
            self.assertEqual(result.returncode, 0, result.stderr)
        else:
            self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
            self.assertNotIn("Traceback", result.stderr)
        return result

    def make_stage(
        self, root, app_name="krita.app", executable_name="krita"
    ):
        payload = root / "Payload"
        app = payload / app_name
        data_dir = app / "share" / "krita"
        data_dir.mkdir(parents=True)
        with (app / "Info.plist").open("wb") as handle:
            plistlib.dump({"CFBundleExecutable": executable_name}, handle)
        executable = app / executable_name
        executable.write_bytes(b"executable\n")
        (data_dir / "data.txt").write_bytes(b"data\n")
        (data_dir / "literal*?[name].txt").write_bytes(b"literal wildcard\n")

        for directory in (data_dir, data_dir.parent, app, payload):
            directory.chmod(0o555)
        (app / "Info.plist").chmod(0o444)
        executable.chmod(0o555)
        (data_dir / "data.txt").chmod(0o444)
        (data_dir / "literal*?[name].txt").chmod(0o444)
        return payload, app

    def archive_stage(self, root, payload, app, succeeds=True):
        entries = []
        for directory, directory_names, file_names in os.walk(payload):
            directory_names.sort()
            file_names.sort()
            relative_directory = Path(directory).relative_to(root).as_posix()
            entries.append(relative_directory + "/")
            entries.extend(
                (Path(directory) / name).relative_to(root).as_posix()
                for name in file_names
            )
        entries.sort()

        ipa = root / "fixture.ipa"
        environment = os.environ.copy()
        environment["ZIPOPT"] = ""
        environment["ZIP"] = ""
        result = subprocess.run(
            [str(ZIP), "-nw", "-MM", "-X", "-9", "-q", str(ipa), "-@"],
            cwd=root,
            env=environment,
            input="".join(f"{entry}\n" for entry in entries),
            check=False,
            capture_output=True,
            text=True,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.run_helper(
            "check-ipa", ipa, "--staged-app", app, succeeds=succeeds
        )
        return ipa

    def rewrite_archive(self, source, destination, mutation):
        with zipfile.ZipFile(source) as input_archive, zipfile.ZipFile(
            destination, "w"
        ) as output_archive:
            for source_entry in input_archive.infolist():
                entry = copy.copy(source_entry)
                data = input_archive.read(source_entry)
                if mutation(entry):
                    continue
                output_archive.writestr(entry, data)

    def test_read_only_stage_normalizes_and_archives(self):
        bundle_names = (
            ("krita.app", "krita"),
            ("LibrePaint.app", "LibrePaint"),
        )
        for app_name, executable_name in bundle_names:
            with self.subTest(app=app_name, executable=executable_name):
                with tempfile.TemporaryDirectory() as temporary:
                    root = Path(temporary)
                    payload, app = self.make_stage(
                        root, app_name, executable_name
                    )
                    self.run_helper("normalize-app", app)
                    payload.chmod(0o755)
                    ipa = self.archive_stage(root, payload, app)
                    self.run_helper("check-ipa", ipa)

                    executable_entry = (
                        f"Payload/{app_name}/{executable_name}"
                    )
                    with zipfile.ZipFile(ipa) as archive:
                        for entry in archive.infolist():
                            mode = entry.external_attr >> 16
                            expected = (
                                0o755
                                if entry.is_dir()
                                or entry.filename == executable_entry
                                else 0o644
                            )
                            self.assertEqual(stat.S_IMODE(mode), expected)
                            self.assertEqual(entry.external_attr & 1, 0)
                            self.assertEqual(entry.extra, b"")

    def test_archive_requires_one_regular_app_directory(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            payload, app = self.make_stage(root)
            self.run_helper("normalize-app", app)
            payload.chmod(0o755)
            source = self.archive_stage(root, payload, app)
            app_entry_name = "Payload/krita.app/"

            no_app = root / "no-app.ipa"
            self.rewrite_archive(
                source,
                no_app,
                lambda entry: entry.filename.startswith("Payload/krita.app"),
            )
            result = self.run_helper("check-ipa", no_app, succeeds=False)
            self.assertIn("found 0", result.stderr)

            multiple_apps = root / "multiple-apps.ipa"
            with zipfile.ZipFile(source) as input_archive, zipfile.ZipFile(
                multiple_apps, "w"
            ) as output_archive:
                for source_entry in input_archive.infolist():
                    entry = copy.copy(source_entry)
                    data = input_archive.read(source_entry)
                    output_archive.writestr(entry, data)
                    if entry.filename == app_entry_name:
                        second_app = copy.copy(entry)
                        second_app.filename = "Payload/LibrePaint.app/"
                        output_archive.writestr(second_app, b"")
            result = self.run_helper(
                "check-ipa", multiple_apps, succeeds=False
            )
            self.assertIn("found 2", result.stderr)

            symlink_app = root / "symlink-app.ipa"

            def make_app_symlink(entry):
                if entry.filename == app_entry_name:
                    entry.external_attr = (stat.S_IFLNK | 0o777) << 16
                return False

            self.rewrite_archive(source, symlink_app, make_app_symlink)
            result = self.run_helper("check-ipa", symlink_app, succeeds=False)
            self.assertIn("app bundle entry is not a regular directory", result.stderr)

    def test_plist_executable_must_match_a_regular_file(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            payload, app = self.make_stage(
                root, "LibrePaint.app", "LibrePaint"
            )
            self.run_helper("normalize-app", app)
            payload.chmod(0o755)
            app.chmod(0o755)
            info_path = app / "Info.plist"
            info_path.chmod(0o644)
            with info_path.open("wb") as handle:
                plistlib.dump({"CFBundleExecutable": "MissingExecutable"}, handle)
            info_path.chmod(0o644)
            app.chmod(0o755)

            ipa = self.archive_stage(
                root, payload, app, succeeds=False
            )
            result = self.run_helper("check-ipa", ipa, succeeds=False)
            self.assertIn(
                "main executable entry is missing: "
                "Payload/LibrePaint.app/MissingExecutable",
                result.stderr,
            )

    def test_stage_preflight_rejects_unsafe_entries_before_chmod(self):
        anomaly_names = ("line\nbreak", "back\\slash", ".DS_Store")
        for anomaly_name in anomaly_names:
            with self.subTest(anomaly=repr(anomaly_name)):
                with tempfile.TemporaryDirectory() as temporary:
                    root = Path(temporary)
                    _payload, app = self.make_stage(root)
                    app.chmod(0o755)
                    (app / anomaly_name).write_bytes(b"bad\n")
                    app.chmod(0o555)
                    self.run_helper("normalize-app", app, succeeds=False)
                    self.assertEqual(stat.S_IMODE(app.stat().st_mode), 0o555)
                    self.assertEqual(
                        stat.S_IMODE((app / "Info.plist").stat().st_mode), 0o444
                    )

    def test_stage_preflight_rejects_links_special_files_and_extra_executables(self):
        for anomaly in ("symlink", "fifo", "executable"):
            with self.subTest(anomaly=anomaly):
                with tempfile.TemporaryDirectory() as temporary:
                    root = Path(temporary)
                    _payload, app = self.make_stage(root)
                    app.chmod(0o755)
                    if anomaly == "symlink":
                        (app / "link").symlink_to("Info.plist")
                    elif anomaly == "fifo":
                        os.mkfifo(app / "fifo")
                    else:
                        (app / "extra-tool").write_bytes(b"tool\n")
                        (app / "extra-tool").chmod(0o555)
                    app.chmod(0o555)
                    self.run_helper("normalize-app", app, succeeds=False)
                    self.assertEqual(stat.S_IMODE(app.stat().st_mode), 0o555)
                    self.assertEqual(
                        stat.S_IMODE((app / "Info.plist").stat().st_mode), 0o444
                    )

    def test_invalid_plist_is_reported_without_traceback(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            _payload, app = self.make_stage(root)
            app.chmod(0o755)
            (app / "Info.plist").chmod(0o644)
            with (app / "Info.plist").open("wb") as handle:
                plistlib.dump(["not", "a", "dictionary"], handle)
            (app / "Info.plist").chmod(0o444)
            app.chmod(0o555)
            self.run_helper("normalize-app", app, succeeds=False)

    def test_zip_requires_every_literal_input_to_match(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            ipa = root / "must-match.ipa"
            environment = os.environ.copy()
            environment["ZIPOPT"] = ""
            environment["ZIP"] = ""
            result = subprocess.run(
                [str(ZIP), "-nw", "-MM", "-X", "-q", str(ipa), "-@"],
                cwd=root,
                env=environment,
                input="Payload/missing\n",
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertFalse(ipa.exists())

    def test_archive_checker_rejects_metadata_types_and_inventory_drift(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            payload, app = self.make_stage(root)
            self.run_helper("normalize-app", app)
            payload.chmod(0o755)
            source = self.archive_stage(root, payload, app)

            data_name = "Payload/krita.app/share/krita/data.txt"

            def dos_read_only(entry):
                if entry.filename == data_name:
                    entry.external_attr |= 1
                return False

            def extra_metadata(entry):
                if entry.filename == data_name:
                    entry.extra = b"\xfe\xca\x00\x00"
                return False

            def entry_comment(entry):
                if entry.filename == data_name:
                    entry.comment = b"comment"
                return False

            def symlink_type(entry):
                if entry.filename == data_name:
                    entry.external_attr = (stat.S_IFLNK | 0o777) << 16
                return False

            def missing_stage_entry(entry):
                return entry.filename == data_name

            cases = (
                ("dos-read-only", dos_read_only, False),
                ("extra-metadata", extra_metadata, False),
                ("entry-comment", entry_comment, False),
                ("symlink-type", symlink_type, False),
                ("inventory-drift", missing_stage_entry, True),
            )
            for name, mutation, compare_stage in cases:
                with self.subTest(anomaly=name):
                    destination = root / f"{name}.ipa"
                    self.rewrite_archive(source, destination, mutation)
                    arguments = ["check-ipa", destination]
                    if compare_stage:
                        arguments.extend(("--staged-app", app))
                    self.run_helper(*arguments, succeeds=False)

    def test_cleanup_does_not_follow_symlinks(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            stage = root / "stage"
            nested = stage / "nested"
            outside = root / "outside"
            nested.mkdir(parents=True)
            outside.write_bytes(b"outside\n")
            outside.chmod(0o400)
            (nested / "link").symlink_to(outside)
            nested.chmod(0o555)
            stage.chmod(0o555)

            self.run_helper("make-tree-removable", stage)
            self.assertTrue(stage.stat().st_mode & stat.S_IWUSR)
            self.assertTrue(nested.stat().st_mode & stat.S_IWUSR)
            self.assertEqual(stat.S_IMODE(outside.stat().st_mode), 0o400)
            shutil.rmtree(stage)


if __name__ == "__main__":
    unittest.main()
