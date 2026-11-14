import importlib.util
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
import unittest
from unittest import mock

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ROOT = os.path.dirname(SCRIPTS)


def load(module_name, filename):
    path = os.path.join(SCRIPTS, filename)
    spec = importlib.util.spec_from_file_location(module_name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


new_worktree = load("new_worktree", "new-worktree.py")
worktree_gc = load("worktree_gc", "worktree-gc.py")
ensure_links = load("ensure_state_links", "ensure-state-links.py")
hygiene = load("check_store_hygiene", "check-store-hygiene.py")
store_guard = load("store_guard", "store-guard.py")

STATE_NAMES = ensure_links.STATE_NAMES


class RepositoryCase(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix="worktrees-")
        self.addCleanup(shutil.rmtree, self.tmp)
        self.origin = os.path.join(self.tmp, "origin.git")
        self.repo = os.path.join(self.tmp, "repo")
        self.state = os.path.join(self.tmp, "state")
        self.wtroot = os.path.join(self.tmp, "wts")

        self.env = dict(os.environ)
        self.env.update(
            {
                "LIBREPAINT_STATE_ROOT": self.state,
                "LIBREPAINT_WORKTREE_ROOT": self.wtroot,
                "GIT_CONFIG_GLOBAL": os.devnull,
                "GIT_CONFIG_SYSTEM": os.devnull,
                "GIT_TERMINAL_PROMPT": "0",
            }
        )

        self.git(self.tmp, "init", "--quiet", "--bare", self.origin)
        self.git(self.tmp, "init", "--quiet", "-b", "main", self.repo)
        self.git(self.repo, "config", "user.email", "agent@example.invalid")
        self.git(self.repo, "config", "user.name", "Agent")
        self.copy_scripts()
        with open(os.path.join(self.repo, ".gitignore"), "w", encoding="utf-8") as handle:
            handle.write("\n".join(STATE_NAMES) + "\n")
        with open(os.path.join(self.repo, "README"), "w", encoding="utf-8") as handle:
            handle.write("repository\n")
        self.git(self.repo, "add", "-A")
        self.git(self.repo, "commit", "--quiet", "-m", "init")
        self.git(self.repo, "remote", "add", "origin", self.origin)
        self.git(self.repo, "push", "--quiet", "-u", "origin", "main")

    def copy_scripts(self):
        destination = os.path.join(self.repo, "scripts")
        os.makedirs(destination)
        for name in (
            "new-worktree.py",
            "worktree-gc.py",
            "ensure-state-links.py",
            "check-store-hygiene.py",
            "store-guard.py",
        ):
            shutil.copy2(os.path.join(SCRIPTS, name), destination)

    def git(self, cwd, *arguments, env=None, check=True):
        environment = dict(self.env)
        if env:
            environment.update(env)
        result = subprocess.run(
            ["git", "-C", cwd] + list(arguments),
            env=environment,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=False,
        )
        if check and result.returncode != 0:
            raise AssertionError("git %s failed: %s" % (" ".join(arguments), result.stderr))
        return result

    def run_script(self, name, *arguments, env=None):
        environment = dict(self.env)
        if env:
            environment.update(env)
        return subprocess.run(
            [sys.executable, os.path.join(self.repo, "scripts", name)] + list(arguments),
            cwd=self.repo,
            env=environment,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=False,
        )

    def create(self, name, *arguments):
        result = self.run_script("new-worktree.py", name, "--purpose", "test", *arguments)
        self.assertEqual(result.returncode, 0, result.stderr)
        return result.stdout.strip()

    def commit_old(self, path, days=30):
        stamp = time.strftime("%Y-%m-%dT%H:%M:%S", time.gmtime(time.time() - days * 86400))
        self.git(
            path,
            "commit",
            "--quiet",
            "--allow-empty",
            "-m",
            "old",
            env={"GIT_AUTHOR_DATE": stamp, "GIT_COMMITTER_DATE": stamp},
        )

    def ready_worktree(self, name, push=True, days=30, dirty=False):
        path = self.create(name, "--new-branch", name)
        if days is not None:
            self.commit_old(path, days)
        if push:
            self.git(path, "push", "--quiet", "origin", name)
        if dirty:
            with open(os.path.join(path, "README"), "a", encoding="utf-8") as handle:
                handle.write("changed\n")
        return path

    def ledger(self):
        path = os.path.join(self.state, "worktrees.json")
        if not os.path.exists(path):
            return {}
        with open(path, "r", encoding="utf-8") as handle:
            return json.load(handle)

    def classify(self):
        return worktree_gc.classify(
            root=self.repo,
            environ=self.env,
            proc_entries=lambda: [],
        )

    def categories(self):
        return {entry["path"]: entry for entry in self.classify()["entries"]}

    def add_unregistered(self, name):
        path = os.path.join(self.wtroot, name)
        self.git(self.repo, "worktree", "add", "--quiet", "-b", name, path)
        return path

    def add_inside_repo(self, name):
        path = os.path.join(self.repo, name)
        self.git(self.repo, "worktree", "add", "--quiet", "-b", name, path)
        result = self.run_script("new-worktree.py", "--adopt", path, "--purpose", "inside")
        self.assertEqual(result.returncode, 0, result.stderr)
        return path


class NewWorktreeTests(RepositoryCase):
    def test_create_registers_ledger_and_state_links(self):
        path = self.create("one")
        self.assertTrue(os.path.isdir(path))
        self.assertEqual(os.path.dirname(path), self.wtroot)

        ledger = self.ledger()
        self.assertIn(path, ledger)
        record = ledger[path]
        self.assertEqual(record["name"], "one")
        self.assertEqual(record["purpose"], "test")
        self.assertEqual(record["owner"], "user")
        self.assertEqual(record["branch_or_ref"], "one")
        self.assertTrue(record["created"])

        for name in STATE_NAMES:
            link = os.path.join(path, name)
            self.assertTrue(os.path.islink(link), link)
            self.assertTrue(os.path.isdir(os.readlink(link)))

    def test_creation_inside_repository_is_rejected(self):
        inside = os.path.join(self.repo, "wts")
        result = self.run_script(
            "new-worktree.py",
            "one",
            "--purpose",
            "test",
            env={"LIBREPAINT_WORKTREE_ROOT": inside},
        )
        self.assertEqual(result.returncode, 2)
        self.assertIn("outside the repository", result.stderr)

    def test_relative_worktree_root_is_rejected(self):
        result = self.run_script(
            "new-worktree.py",
            "one",
            "--purpose",
            "test",
            env={"LIBREPAINT_WORKTREE_ROOT": "relative/wts"},
        )
        self.assertEqual(result.returncode, 2)
        self.assertIn("absolute path", result.stderr)

    def test_purpose_is_required(self):
        missing = self.run_script("new-worktree.py", "one")
        self.assertEqual(missing.returncode, 2)
        empty = self.run_script("new-worktree.py", "one", "--purpose", " ")
        self.assertEqual(empty.returncode, 2)

    def test_name_cannot_contain_parent_directory(self):
        result = self.run_script("new-worktree.py", "a..b", "--purpose", "test")
        self.assertEqual(result.returncode, 2)

    def test_existing_path_is_rejected(self):
        os.makedirs(os.path.join(self.wtroot, "one"))
        result = self.run_script("new-worktree.py", "one", "--purpose", "test")
        self.assertEqual(result.returncode, 1)

    def test_adopt_registers_an_existing_worktree_without_moving_it(self):
        path = os.path.join(self.wtroot, "adopted")
        self.git(self.repo, "worktree", "add", "--quiet", "-b", "adopted", path)
        result = self.run_script("new-worktree.py", "--adopt", path, "--purpose", "adopted")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.strip(), os.path.realpath(path))
        self.assertTrue(os.path.isdir(path))
        record = self.ledger()[os.path.realpath(path)]
        self.assertEqual(record["name"], "adopted")
        self.assertEqual(record["purpose"], "adopted")

    def test_adopt_rejects_a_path_outside_the_worktree_list(self):
        directory = os.path.join(self.tmp, "plain")
        os.makedirs(directory)
        result = self.run_script("new-worktree.py", "--adopt", directory, "--purpose", "x")
        self.assertEqual(result.returncode, 2)


class WorktreeGcTests(RepositoryCase):
    def test_classification(self):
        eligible = self.ready_worktree("eligible")
        modified = self.ready_worktree("modified", dirty=True)
        unpushed = self.ready_worktree("unpushed", push=False)
        recent = self.ready_worktree("recent", days=None)
        unregistered = self.add_unregistered("unregistered")
        inside = self.add_inside_repo("inside")
        prunable = self.create("prunable")
        shutil.rmtree(prunable)

        entries = self.categories()
        self.assertEqual(entries[eligible]["category"], "eligible")
        self.assertEqual(entries[modified]["category"], "needs-review")
        self.assertIn("変更あり", entries[modified]["reasons"])
        self.assertEqual(entries[unpushed]["category"], "needs-review")
        self.assertIn("push されていない", entries[unpushed]["reasons"])
        self.assertEqual(entries[recent]["category"], "needs-review")
        self.assertIn("最終コミットが 7 日以内", entries[recent]["reasons"])
        self.assertEqual(entries[unregistered]["category"], "unregistered")
        self.assertIn("--adopt", entries[unregistered]["hint"])
        self.assertEqual(entries[inside]["category"], "inside-repo")
        self.assertEqual(entries[prunable]["category"], "prunable")

    def test_apply_removes_only_eligible_and_prunable(self):
        eligible = self.ready_worktree("eligible")
        modified = self.ready_worktree("modified", dirty=True)
        unregistered = self.add_unregistered("unregistered")
        prunable = self.create("prunable")
        shutil.rmtree(prunable)

        state_dir = ensure_links.worktree_state_dir(eligible, self.env)
        self.assertTrue(os.path.isdir(state_dir))

        result = self.run_script("worktree-gc.py", "--apply")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("removed: %s" % eligible, result.stdout)
        self.assertIn("pruned: %s" % prunable, result.stdout)

        self.assertFalse(os.path.exists(eligible))
        self.assertFalse(os.path.exists(state_dir))
        self.assertNotIn(eligible, self.ledger())

        self.assertTrue(os.path.isdir(modified))
        self.assertTrue(os.path.isdir(unregistered))
        self.assertIn(modified, self.ledger())

        listed = self.git(self.repo, "worktree", "list", "--porcelain").stdout
        self.assertNotIn(prunable, listed)
        self.assertIn(modified, listed)

    def test_apply_keeps_unregistered_and_inside_repo(self):
        unregistered = self.add_unregistered("unregistered")
        inside = self.add_inside_repo("inside")

        result = self.run_script("worktree-gc.py", "--apply")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue(os.path.isdir(unregistered))
        self.assertTrue(os.path.isdir(inside))


class StoreHygieneWorktreeTests(RepositoryCase):
    def test_prunable_is_a_violation(self):
        path = self.create("gone")
        shutil.rmtree(path)
        with mock.patch.dict(os.environ, self.env):
            problems = hygiene.worktree_problems(self.repo)
        self.assertTrue(any("worktree prunable" in message for message, _ in problems))

    def test_unregistered_is_a_violation(self):
        self.add_unregistered("loose")
        with mock.patch.dict(os.environ, self.env):
            problems = hygiene.worktree_problems(self.repo)
        self.assertTrue(any("worktree unregistered" in message for message, _ in problems))

    def test_inside_repo_is_a_violation(self):
        self.add_inside_repo("inner")
        with mock.patch.dict(os.environ, self.env):
            problems = hygiene.worktree_problems(self.repo)
        self.assertTrue(any("worktree inside-repo" in message for message, _ in problems))

    def test_eligible_is_only_a_warning(self):
        self.ready_worktree("eligible")
        with mock.patch.dict(os.environ, self.env):
            self.assertEqual(hygiene.check(self.repo, skip_free=True), [])
            warnings = hygiene.worktree_warnings(self.repo)
        self.assertTrue(any("eligible for removal" in message for message, _ in warnings))

    def test_ledger_over_four_is_a_warning(self):
        for index in range(4):
            self.create("wt%d" % index)

        def add_main(data):
            data[self.repo] = {
                "name": "repo",
                "purpose": "main",
                "owner": "user",
                "branch_or_ref": "main",
                "created": new_worktree.timestamp(),
            }

        new_worktree.update_ledger(self.env, add_main)
        with mock.patch.dict(os.environ, self.env):
            warnings = hygiene.worktree_warnings(self.repo)
        self.assertTrue(any("over 4" in message for message, _ in warnings))


class StoreGuardWorktreeTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix="store-guard-worktrees-")
        self.addCleanup(shutil.rmtree, self.tmp)
        self.state = os.path.join(self.tmp, "state")
        self.store = os.path.join(self.tmp, "store")
        os.makedirs(self.store)
        self.events = []
        self.listing = {"entries": [], "ledger": [], "main": None}
        self.deliver = True

        patches = [
            mock.patch.object(store_guard, "state_root", return_value=self.state),
            mock.patch.object(store_guard, "STORE", self.store),
            mock.patch.object(store_guard, "repository_root", return_value=self.tmp),
            mock.patch.object(store_guard, "process_table", return_value=""),
            mock.patch.object(store_guard, "query_referrers", return_value=subprocess.CompletedProcess([], 1, "", "")),
            mock.patch.object(store_guard, "free_gib", return_value=500.0),
            mock.patch.object(store_guard, "query_path_info", return_value={}),
            mock.patch.object(
                store_guard,
                "send_event",
                side_effect=lambda message: self.events.append(message) or self.deliver,
            ),
            mock.patch.object(
                store_guard, "worktree_listing", side_effect=lambda root: self.listing
            ),
        ]
        for patch in patches:
            patch.start()
            self.addCleanup(patch.stop)

    def log_records(self):
        path = os.path.join(self.state, "store-guard.log")
        if not os.path.exists(path):
            return []
        with open(path, "r", encoding="utf-8") as handle:
            return [json.loads(line) for line in handle if line.strip()]

    def test_notification_is_sent_once_per_day(self):
        self.listing = {
            "main": None,
            "ledger": [],
            "entries": [
                {"category": "prunable", "path": "/wts/old", "reasons": []},
                {"category": "needs-review", "path": "/wts/busy", "reasons": ["変更あり"]},
            ],
        }
        self.assertEqual(store_guard.run(), 0)
        self.assertEqual(store_guard.run(), 0)
        self.assertEqual(len(self.events), 1)
        self.assertIn("放置の作業ツリー", self.events[0])
        self.assertIn("/wts/old", self.events[0])
        self.assertNotIn("/wts/busy", self.events[0])

    def test_listing_is_written_to_the_log(self):
        self.listing = {
            "main": None,
            "ledger": [],
            "entries": [{"category": "eligible", "path": "/wts/done", "reasons": []}],
        }
        self.assertEqual(store_guard.run(), 0)
        records = [record["worktrees"] for record in self.log_records() if "worktrees" in record]
        self.assertEqual(len(records), 1)
        self.assertEqual(records[0]["eligible"], ["/wts/done"])

    def test_failed_delivery_is_logged(self):
        self.deliver = False
        self.listing = {
            "main": None,
            "ledger": [],
            "entries": [{"category": "prunable", "path": "/wts/old", "reasons": []}],
        }
        self.assertEqual(store_guard.run(), 0)
        self.assertTrue(any(record.get("delivery") == "failed" for record in self.log_records()))

    def test_no_event_without_actionable_worktrees(self):
        self.listing = {
            "main": None,
            "ledger": [],
            "entries": [{"category": "needs-review", "path": "/wts/busy", "reasons": ["変更あり"]}],
        }
        self.assertEqual(store_guard.run(), 0)
        self.assertEqual(self.events, [])
        self.assertFalse(any("worktrees" in record for record in self.log_records()))


if __name__ == "__main__":
    unittest.main()
