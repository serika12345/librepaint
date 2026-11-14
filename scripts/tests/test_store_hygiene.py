import importlib.util
import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from types import SimpleNamespace
from unittest import mock

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GIB = 1024**3


def load(module_name, filename):
    path = os.path.join(SCRIPTS, filename)
    spec = importlib.util.spec_from_file_location(module_name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


ensure_links = load("ensure_state_links", "ensure-state-links.py")
hygiene = load("check_store_hygiene", "check-store-hygiene.py")
store_guard = load("store_guard", "store-guard.py")


class EnsureStateLinksTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix="state-links-")
        self.addCleanup(shutil.rmtree, self.tmp)
        self.root = os.path.join(self.tmp, "repo")
        os.makedirs(self.root)
        self.env = {"LIBREPAINT_STATE_ROOT": os.path.join(self.tmp, "state")}

    def ensure(self, name, migrate=False, proc_entries=lambda: []):
        ensure_links.ensure_name(
            self.root,
            name,
            migrate=migrate,
            proc_entries=(lambda: []) if proc_entries is None else proc_entries,
            quiet=True,
            environ=self.env,
        )

    def test_default_creates_absolute_links(self):
        for name in ensure_links.STATE_NAMES:
            self.ensure(name)
            link = os.path.join(self.root, name)
            self.assertTrue(os.path.islink(link))
            target = os.readlink(link)
            self.assertTrue(os.path.isabs(target))
            self.assertTrue(os.path.isdir(target))

    def test_real_directory_is_rejected_without_migrate(self):
        build = os.path.join(self.root, "build")
        os.makedirs(build)
        with self.assertRaises(SystemExit) as raised:
            self.ensure("build")
        self.assertEqual(raised.exception.code, 1)
        self.assertTrue(os.path.isdir(build))
        self.assertFalse(os.path.islink(build))

    def test_migrate_moves_and_is_idempotent(self):
        build = os.path.join(self.root, "build")
        os.makedirs(build)
        with open(os.path.join(build, "kept"), "w", encoding="utf-8") as handle:
            handle.write("data")
        self.ensure("build", migrate=True)
        self.assertTrue(os.path.islink(build))
        target = os.readlink(build)
        self.assertTrue(os.path.isfile(os.path.join(target, "kept")))
        self.ensure("build", migrate=True)
        self.assertEqual(os.readlink(build), target)

    def test_migrate_refuses_non_empty_destination(self):
        build = os.path.join(self.root, "build")
        os.makedirs(build)
        target = ensure_links.state_path(self.root, "build", self.env)
        os.makedirs(target)
        with open(os.path.join(target, "existing"), "w", encoding="utf-8") as handle:
            handle.write("data")
        with self.assertRaises(SystemExit) as raised:
            self.ensure("build", migrate=True)
        self.assertEqual(raised.exception.code, 1)
        self.assertFalse(os.path.islink(build))

    def test_migrate_refuses_when_cwd_is_below(self):
        build = os.path.join(self.root, "build")
        os.makedirs(os.path.join(build, "sub"))
        with self.assertRaises(SystemExit) as raised:
            self.ensure("build", migrate=True, proc_entries=lambda: [(4242, os.path.join(build, "sub"))])
        self.assertEqual(raised.exception.code, 1)
        self.assertFalse(os.path.islink(build))

    def test_relative_state_root_is_rejected(self):
        with self.assertRaises(ensure_links.ConfigurationError):
            ensure_links.state_root({"LIBREPAINT_STATE_ROOT": "relative/path"})
        with mock.patch.dict(os.environ, {"LIBREPAINT_STATE_ROOT": "relative/path"}):
            self.assertEqual(ensure_links.main([]), 2)

    def test_dangling_link_is_repaired(self):
        target = ensure_links.state_path(self.root, "build", self.env)
        os.symlink(target, os.path.join(self.root, "build"))
        self.ensure("build")
        self.assertTrue(os.path.isdir(target))

    def test_link_to_regular_file_is_rejected(self):
        elsewhere = os.path.join(self.tmp, "file")
        with open(elsewhere, "w", encoding="utf-8") as handle:
            handle.write("data")
        os.symlink(elsewhere, os.path.join(self.root, "build"))
        with self.assertRaises(SystemExit) as raised:
            self.ensure("build")
        self.assertEqual(raised.exception.code, 1)

    def test_check_finds_claude_worktrees(self):
        worktree = os.path.join(self.root, ".claude", "worktrees", "one")
        os.makedirs(os.path.join(worktree, "cache"))
        os.makedirs(os.path.join(worktree, ".cache"))
        violations = ensure_links.check_roots(self.root)
        self.assertIn(os.path.join(worktree, ".cache"), violations)

    def test_check_passes_for_links(self):
        for name in ensure_links.STATE_NAMES:
            self.ensure(name)
        self.assertEqual(ensure_links.check_roots(self.root), [])


class CheckStoreHygieneTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix="hygiene-")
        self.addCleanup(shutil.rmtree, self.tmp)
        self.root = os.path.join(self.tmp, "repo")
        os.makedirs(self.root)

    def test_links_pass(self):
        for name in ensure_links.STATE_NAMES:
            target = os.path.join(self.tmp, "state", name)
            os.makedirs(target, exist_ok=True)
            os.symlink(target, os.path.join(self.root, name))
        self.assertEqual(hygiene.check(self.root, skip_free=True), [])

    def test_real_directory_is_a_violation(self):
        os.makedirs(os.path.join(self.root, "build"))
        problems = hygiene.check(self.root, skip_free=True)
        self.assertTrue(any("real state directory" in message for message, _ in problems))

    def test_skip_free_does_not_read_statvfs(self):
        with mock.patch.object(hygiene, "free_gib", side_effect=AssertionError("called")):
            self.assertEqual(hygiene.check(self.root, skip_free=True), [])

    def test_large_tree_reports_five_largest_directories(self):
        for index in range(6):
            directory = os.path.join(self.root, "area%d" % index)
            os.makedirs(directory)
            sparse = os.path.join(directory, "sparse")
            with open(sparse, "wb") as handle:
                handle.truncate((index + 1) * GIB)
        problems = hygiene.check(self.root, skip_free=True)
        messages = "\n".join(message for message, _ in problems)
        self.assertIn("largest directories", messages)
        listed = [line for line in messages.splitlines() if line.startswith("  ")]
        self.assertEqual(len(listed), 5)
        self.assertNotIn("area0", messages)
        for index in range(1, 6):
            self.assertIn("area%d" % index, messages)


def fake_result(returncode=0, stdout="", stderr=""):
    return SimpleNamespace(returncode=returncode, stdout=stdout, stderr=stderr)


class StoreGuardTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix="store-guard-")
        self.addCleanup(shutil.rmtree, self.tmp)
        self.state = os.path.join(self.tmp, "state")
        self.store = os.path.join(self.tmp, "store")
        os.makedirs(self.store)
        self.delete_calls = []
        self.events = []
        self.available = 500.0

        def delete(path):
            self.delete_calls.append(path)
            return fake_result(0)

        patches = [
            mock.patch.object(store_guard, "state_root", return_value=self.state),
            mock.patch.object(store_guard, "STORE", self.store),
            mock.patch.object(store_guard, "repository_root", return_value=self.tmp),
            mock.patch.object(store_guard, "process_table", return_value=""),
            mock.patch.object(store_guard, "query_referrers", return_value=fake_result(1)),
            mock.patch.object(store_guard, "free_gib", side_effect=lambda: self.available),
            mock.patch.object(store_guard, "send_event", side_effect=lambda message: self.events.append(message) or True),
            mock.patch.object(store_guard, "delete_store_path", side_effect=delete),
        ]
        for patch in patches:
            patch.start()
            self.addCleanup(patch.stop)

    def add_source(self, name="one"):
        path = os.path.join(self.store, "%s-source" % name)
        os.makedirs(path)
        return path

    def info_for(self, path, size=2 * GIB):
        return {path: {"narSize": size, "registrationTime": 1000}}

    def test_small_paths_are_ignored(self):
        path = self.add_source()
        with mock.patch.object(store_guard, "query_path_info", return_value=self.info_for(path, 100)):
            self.assertEqual(store_guard.run(), 0)
        self.assertEqual(store_guard.load_state()["known"], {})
        self.assertFalse(os.path.exists(store_guard.log_file()))

    def test_first_seen_path_is_logged_once(self):
        path = self.add_source()
        with mock.patch.object(store_guard, "query_path_info", return_value=self.info_for(path)):
            self.assertEqual(store_guard.run(), 0)
            self.assertEqual(store_guard.run(), 0)
        with open(store_guard.log_file(), "r", encoding="utf-8") as handle:
            records = [json.loads(line) for line in handle if line.strip()]
        first_seen = [record for record in records if record.get("path") == path]
        self.assertEqual(len(first_seen), 1)
        self.assertIn(path, store_guard.load_state()["known"])

    def test_copy_without_repository_markers_is_not_deleted(self):
        path = self.add_source()
        with open(os.path.join(path, "flake.nix"), "w", encoding="utf-8") as handle:
            handle.write("{}")
        with mock.patch.object(store_guard, "query_path_info", return_value=self.info_for(path)):
            self.assertEqual(store_guard.run(), 0)
        self.assertEqual(self.delete_calls, [])

    def test_repository_copy_is_deleted(self):
        path = self.add_source()
        os.makedirs(os.path.join(path, ".git"))
        os.makedirs(os.path.join(path, "build"))
        with open(os.path.join(path, "flake.nix"), "w", encoding="utf-8") as handle:
            handle.write("{}")
        with mock.patch.object(store_guard, "query_path_info", return_value=self.info_for(path)):
            self.assertEqual(store_guard.run(), 0)
        self.assertEqual(self.delete_calls, [path])

    def test_delete_refusal_is_logged_and_not_raised(self):
        path = self.add_source()
        os.makedirs(os.path.join(path, ".git"))
        os.makedirs(os.path.join(path, ".cache"))
        with open(os.path.join(path, "flake.nix"), "w", encoding="utf-8") as handle:
            handle.write("{}")
        with mock.patch.object(store_guard, "query_path_info", return_value=self.info_for(path)), mock.patch.object(
            store_guard, "delete_store_path", return_value=fake_result(1, stderr="path is still alive")
        ):
            self.assertEqual(store_guard.run(), 0)
        with open(store_guard.log_file(), "r", encoding="utf-8") as handle:
            records = [json.loads(line) for line in handle if line.strip()]
        self.assertTrue(any(record.get("refused") == path for record in records))

    def test_permission_denied_disables_later_deletion(self):
        path = self.add_source()
        os.makedirs(os.path.join(path, ".git"))
        os.makedirs(os.path.join(path, "po"))
        with open(os.path.join(path, "flake.nix"), "w", encoding="utf-8") as handle:
            handle.write("{}")
        refused = fake_result(1, stderr="error: Permission denied")
        with mock.patch.object(store_guard, "query_path_info", return_value=self.info_for(path)), mock.patch.object(
            store_guard, "delete_store_path", return_value=refused
        ) as delete:
            self.assertEqual(store_guard.run(), 0)
            self.assertEqual(delete.call_count, 1)
            self.assertEqual(store_guard.run(), 0)
            self.assertEqual(delete.call_count, 1)
        self.assertTrue(store_guard.load_state()["delete_forbidden"])

    def test_path_info_failure_returns_one(self):
        self.add_source()
        with mock.patch.object(store_guard, "query_path_info", side_effect=store_guard.StoreError("boom")):
            self.assertEqual(store_guard.run(), 1)
        self.assertTrue(os.path.exists(store_guard.log_file()))

    def test_dry_run_writes_nothing(self):
        path = self.add_source()
        with mock.patch.object(store_guard, "query_path_info", return_value=self.info_for(path)):
            with mock.patch("builtins.print") as printed:
                self.assertEqual(store_guard.run(dry_run=True), 0)
        self.assertFalse(os.path.exists(store_guard.state_file()))
        self.assertFalse(os.path.exists(store_guard.log_file()))
        self.assertEqual(self.delete_calls, [])
        self.assertTrue(printed.called)

    def test_low_space_warns_once_per_hour(self):
        self.available = 50.0
        self.add_source()
        with mock.patch.object(store_guard, "query_path_info", return_value={}):
            self.assertEqual(store_guard.run(), 0)
            self.assertEqual(store_guard.run(), 0)
        self.assertEqual(len(self.events), 1)
        self.assertIn("100GiB", self.events[0])


class NixGuardTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix="nix-guard-")
        self.addCleanup(shutil.rmtree, self.tmp)
        self.repo = os.path.join(self.tmp, "repo")
        guard_dir = os.path.join(self.repo, "scripts", "nix-guard")
        os.makedirs(guard_dir)
        self.guard = os.path.join(guard_dir, "nix")
        shutil.copy2(os.path.join(SCRIPTS, "nix-guard", "nix"), self.guard)
        os.chmod(self.guard, 0o755)
        self.fake_bin = os.path.join(self.tmp, "fake-bin")
        os.makedirs(self.fake_bin)
        self.calls = os.path.join(self.tmp, "fake-calls")
        self.fake_nix = os.path.join(self.fake_bin, "nix")
        with open(self.fake_nix, "w", encoding="utf-8") as handle:
            handle.write('#!/bin/sh\nprintf \'%s\\n\' "$*" >> "$FAKE_NIX_CALLS"\n')
        os.chmod(self.fake_nix, 0o755)
        self.state = os.path.join(self.tmp, "state")
        for command in (["git", "init", "-q", "."], ["git", "add", "-A"]):
            subprocess.run(command, cwd=self.repo, check=True)
        subprocess.run(
            ["git", "-c", "user.email=a@b", "-c", "user.name=a", "commit", "-qm", "init"],
            cwd=self.repo,
            check=True,
        )
        self.env = dict(os.environ)
        self.env["LIBREPAINT_STATE_ROOT"] = self.state
        self.env["FAKE_NIX_CALLS"] = self.calls
        self.env["PATH"] = guard_dir + os.pathsep + self.fake_bin + os.pathsep + os.environ.get("PATH", "")

    def run_nix(self, arguments, cwd=None, extra_env=None):
        environment = dict(self.env)
        if extra_env:
            environment.update(extra_env)
        return subprocess.run(
            ["nix"] + arguments,
            cwd=cwd or self.repo,
            env=environment,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=False,
        )

    def logged_lines(self):
        with open(os.path.join(self.state, "nix-calls.log"), "r", encoding="utf-8") as handle:
            return [line for line in handle if line.strip()]

    def test_version_passes_to_fake_nix(self):
        result = self.run_nix(["--version"])
        self.assertEqual(result.returncode, 0, result.stderr)
        with open(self.calls, "r", encoding="utf-8") as handle:
            self.assertIn("--version", handle.read())

    def test_path_flake_of_repository_is_rejected(self):
        result = self.run_nix(["build", "path:%s#x" % self.repo])
        self.assertEqual(result.returncode, 2)
        self.assertIn("path:", result.stderr)

    def test_relative_path_flake_of_repository_is_rejected(self):
        result = self.run_nix(["build", "path:.#x"])
        self.assertEqual(result.returncode, 2)

    def test_dot_reference_inside_worktree_passes(self):
        result = self.run_nix(["build", ".#x"])
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_dot_reference_without_git_is_rejected(self):
        directory = os.path.join(self.tmp, "nogit")
        os.makedirs(directory)
        result = self.run_nix(["build", ".#x"], cwd=directory, extra_env={"LIBREPAINT_REPO_ROOT": directory})
        self.assertEqual(result.returncode, 2)
        passed = self.run_nix(["--version"], cwd=directory, extra_env={"LIBREPAINT_REPO_ROOT": directory})
        self.assertEqual(passed.returncode, 0, passed.stderr)

    def test_every_call_appends_one_log_line(self):
        self.run_nix(["--version"])
        self.run_nix(["build", "path:%s#x" % self.repo])
        self.assertEqual(len(self.logged_lines()), 2)

    def assert_rejected(self, expression):
        result = self.run_nix(["eval", "--impure", "--expr", expression])
        self.assertEqual(result.returncode, 2, result.stderr)
        self.assertIn("getFlake", result.stderr)
        self.assertFalse(os.path.exists(self.calls))

    def assert_passed(self, expression):
        result = self.run_nix(["eval", "--impure", "--expr", expression])
        self.assertEqual(result.returncode, 0, result.stderr)
        with open(self.calls, "r", encoding="utf-8") as handle:
            self.assertIn(expression, handle.read())

    def test_getflake_with_local_path_is_rejected(self):
        self.assert_rejected("(builtins.getFlake (toString ./.)).outPath")

    def test_getflake_with_path_reference_is_rejected(self):
        self.assert_rejected('(builtins.getFlake "path:/x").outPath')

    def test_getflake_with_absolute_string_is_rejected(self):
        self.assert_rejected('(builtins.getFlake "/x").outPath')

    def test_getflake_with_remote_references_passes(self):
        self.assert_passed('(builtins.getFlake "git+file:///x").outPath')
        os.remove(self.calls)
        self.assert_passed('(builtins.getFlake "github:NixOS/nixpkgs/0000000000000000000000000000000000000000").outPath')

    def test_expression_without_getflake_passes(self):
        self.assert_passed("1 + 1")


if __name__ == "__main__":
    unittest.main()
