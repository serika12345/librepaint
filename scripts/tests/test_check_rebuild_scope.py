import importlib.util
import os
import shutil
import subprocess
import tempfile
import unittest

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPO_ROOT = os.path.dirname(SCRIPTS)
DOCUMENT = os.path.join(REPO_ROOT, "docs", "architecture", "DEVELOPMENT.md")
BEGIN = "<!-- rebuild-scope:begin -->"
END = "<!-- rebuild-scope:end -->"


def load(filename):
    path = os.path.join(SCRIPTS, filename)
    spec = importlib.util.spec_from_file_location("check_rebuild_scope", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


scope = load("check-rebuild-scope.py")

MATRIX = {
    "labels": {
        "linux/compile": {"attrs": ["a"]},
        "linux/package": {"attrs": ["b"]},
        "macos/compile": {"attrs": ["c"]},
        "governance": {"alwaysChanges": True, "attrs": ["d"]},
    },
    "probes": [],
}


class MatrixTests(unittest.TestCase):
    def test_repository_matrix_is_valid(self):
        self.assertEqual(scope.validate_matrix(scope.load_matrix()), [])

    def test_probe_files_match_their_edit_kind(self):
        for probe in scope.load_matrix()["probes"]:
            if scope.is_control(probe):
                continue
            exists = os.path.exists(os.path.join(REPO_ROOT, probe["path"]))
            if "create" in probe["edit"]:
                self.assertFalse(exists, f"{probe['name']}: {probe['path']} exists")
            else:
                self.assertTrue(exists, f"{probe['name']}: {probe['path']} is missing")

    def test_wildcards_expand_and_always_changing_labels_are_included(self):
        probe = {"name": "p", "changes": ["*/compile"]}
        self.assertEqual(
            scope.expected_changes(MATRIX, probe),
            {"linux/compile", "macos/compile", "governance"},
        )

    def test_a_pattern_without_a_label_is_rejected(self):
        with self.assertRaises(ValueError):
            scope.expected_changes(MATRIX, {"name": "p", "changes": ["windows/*"]})

    def test_changed_labels_compare_every_derivation_of_a_label(self):
        base = {"linux/compile": ["x"], "linux/package": ["y", "z"]}
        probe = {"linux/compile": ["x"], "linux/package": ["y", "w"]}
        self.assertEqual(scope.changed_labels(base, probe), {"linux/package"})


class ControlTests(unittest.TestCase):
    def test_a_control_probe_keeps_even_the_always_changing_labels_unchanged(self):
        probe = {"name": "c", "edit": {"control": True}, "changes": []}
        self.assertEqual(scope.expected_changes(MATRIX, probe), set())

    def test_the_repository_matrix_has_a_positive_and_a_negative_control(self):
        matrix = scope.load_matrix()
        self.assertTrue(any(scope.is_control(probe) for probe in matrix["probes"]))
        compile_labels = {name for name in matrix["labels"] if name.endswith("/compile")}
        self.assertTrue(
            any(compile_labels <= scope.expected_changes(matrix, probe) for probe in matrix["probes"])
        )


class DescribeTests(unittest.TestCase):
    def test_complete_categories_name_all_platforms(self):
        labels = {f"{platform}/compile" for platform in scope.PLATFORM_NAMES}
        self.assertEqual(scope.describe(labels, set()), "全プラットフォームのコンパイル")

    def test_partial_categories_name_each_platform(self):
        text = scope.describe({"linux/compile", "linux/package", "ios/package"}, set())
        self.assertEqual(text, "Linuxのコンパイル、Linuxのパッケージ化、iOSのパッケージ化")

    def test_empty_set_reads_as_nothing(self):
        self.assertEqual(scope.describe({"governance"}, {"governance"}), "なし")


class EditTests(unittest.TestCase):
    def test_append_requires_an_existing_file(self):
        self.assertEqual(scope.apply_edit(b"a\n", {"append": "b\n"}, "f"), b"a\nb\n")
        with self.assertRaises(ValueError):
            scope.apply_edit(None, {"append": "b"}, "f")

    def test_create_requires_a_new_file(self):
        self.assertEqual(scope.apply_edit(None, {"create": "x\n"}, "f"), b"x\n")
        with self.assertRaises(ValueError):
            scope.apply_edit(b"a", {"create": "x"}, "f")

    def test_insert_after_keeps_the_indentation_of_the_matched_line(self):
        content = b"''\n    first\n    second\n''\n"
        edited = scope.apply_edit(content, {"insert_after": "first", "text": "# probe"}, "f")
        self.assertEqual(edited, b"''\n    first\n    # probe\n    second\n''\n")

    def test_insert_after_fails_when_no_line_matches(self):
        with self.assertRaises(ValueError):
            scope.apply_edit(b"a\n", {"insert_after": "zzz", "text": "t"}, "f")


@unittest.skipUnless(shutil.which("git"), "git is required")
class ScratchCommitTests(unittest.TestCase):
    def test_the_commit_carries_the_edit_and_nothing_else_changes(self):
        with tempfile.TemporaryDirectory() as root:
            def git(*arguments):
                return subprocess.run(
                    ["git", *arguments], cwd=root, check=True, capture_output=True, text=True
                ).stdout.strip()

            git("init", "-q")
            git("config", "user.email", "t@localhost")
            git("config", "user.name", "t")
            with open(os.path.join(root, "a.txt"), "w", encoding="utf-8") as handle:
                handle.write("one\n")
            git("add", "a.txt")
            git("commit", "-q", "-m", "base")
            base = git("rev-parse", "HEAD")
            refs = git("for-each-ref")

            commit = scope.scratch_commit(root, base, {"path": "a.txt", "append": "two\n"})

            self.assertEqual(git("show", f"{commit}:a.txt"), "one\ntwo")
            self.assertEqual(git("rev-parse", f"{commit}^"), base)
            self.assertEqual(git("for-each-ref"), refs)
            self.assertEqual(git("status", "--porcelain"), "")
            self.assertEqual(git("rev-parse", "HEAD"), base)

            control = scope.scratch_commit(root, base, {"control": True})
            self.assertNotEqual(control, base)
            self.assertEqual(git("rev-parse", f"{control}^{{tree}}"), git("rev-parse", f"{base}^{{tree}}"))


class DocumentTests(unittest.TestCase):
    def test_the_manual_table_is_the_rendered_matrix(self):
        with open(DOCUMENT, encoding="utf-8") as handle:
            text = handle.read()
        self.assertIn(BEGIN, text)
        self.assertIn(END, text)
        table = text.split(BEGIN, 1)[1].split(END, 1)[0].strip()
        self.assertEqual(
            table,
            scope.render_table(scope.load_matrix()),
            "run: python3 scripts/check-rebuild-scope.py --render-table",
        )


if __name__ == "__main__":
    unittest.main()
