import importlib.util
import io
import os
import shutil
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def load(filename):
    path = os.path.join(SCRIPTS, filename)
    spec = importlib.util.spec_from_file_location("check_flake_hygiene", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


hygiene = load("check-flake-hygiene.py")


class FlakeHygieneTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix="flake-hygiene-")
        self.addCleanup(shutil.rmtree, self.tmp)
        self.root = os.path.join(self.tmp, "repo")
        os.makedirs(os.path.join(self.root, "nix"))

    def write(self, relative_path, text):
        path = os.path.join(self.root, relative_path)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8") as handle:
            handle.write(text)

    def violations(self):
        _files, violations = hygiene.check(self.root)
        return [(os.path.relpath(path, self.root), line, rule) for path, line, rule in violations]

    def test_clean_expressions_have_no_violations(self):
        self.write("flake.nix", "{ outputs = _: { }; }\n")
        self.write("nix/package.nix", "{ lib }: { buildInputs = [ lib ]; }\n")
        self.assertEqual(self.violations(), [])

    def test_reports_each_build_option(self):
        cases = {
            "ninjaFlags = [ \"-k\" \"0\" ];\n": "buildOptions: -k flag",
            "NIX_DEBUG = 1;\n": "NIX_DEBUG",
            "enableDebugging = true;\n": "enableDebugging",
            "separateDebugInfo = true;\n": "separateDebugInfo",
        }
        for line, rule in cases.items():
            with self.subTest(line=line):
                self.write("flake.nix", line)
                self.assertEqual(self.violations(), [("flake.nix", 1, rule)])

    def test_reports_k_flag_in_make_and_build_flags(self):
        for attribute in ("makeFlags", "buildFlags"):
            with self.subTest(attribute=attribute):
                self.write("flake.nix", "%s = [ \"-k\" ];\n" % attribute)
                self.assertEqual(self.violations(), [("flake.nix", 1, "buildOptions: -k flag")])

    def test_reports_marker_comments(self):
        for marker in hygiene.MARKERS:
            with self.subTest(marker=marker):
                self.write("nix/package.nix", "# %s note\n{ }\n" % marker)
                self.assertEqual(
                    self.violations(),
                    [("nix/package.nix", 1, "markers: %s" % marker)],
                )

    def test_reason_comment_exempts_build_option(self):
        self.write(
            "nix/package.nix",
            "{\n"
            "  # remove when nixpkgs carries the fix\n"
            "  separateDebugInfo = true;\n"
            "  NIX_DEBUG = 1; # Issue #108\n"
            "}\n",
        )
        self.assertEqual(self.violations(), [])

    def test_reason_comment_exempts_k_flag_in_multiline_value(self):
        self.write(
            "flake.nix",
            "{\n"
            "  # removal condition: drop when the upstream fix lands\n"
            "  ninjaFlags = [\n"
            "    \"-k\" # drop when the upstream fix lands\n"
            "  ];\n"
            "}\n",
        )
        self.assertEqual(self.violations(), [])

    def test_reason_marker_must_be_in_a_comment(self):
        self.write("flake.nix", "{ name = \"remove when\"; }\n")
        self.write("nix/package.nix", "{ NIX_DEBUG = 1; }\n")
        self.assertEqual(
            self.violations(),
            [("nix/package.nix", 1, "NIX_DEBUG")],
        )

    def test_list_reports_files_and_rules(self):
        self.write("flake.nix", "{ }\n")
        self.write("nix/package.nix", "{ }\n")
        stdout = io.StringIO()
        stderr = io.StringIO()
        with redirect_stdout(stdout), redirect_stderr(stderr):
            status = hygiene.main(["--root", self.root, "--list"])
        self.assertEqual(status, 0)
        output = stdout.getvalue()
        self.assertIn("2 files", output)
        self.assertIn("rule: buildOptions: NIX_DEBUG", output)
        self.assertIn("reason markers: nixpkgs, Issue #, remove when, drop when, removal", output)

    def test_main_returns_one_for_violation(self):
        self.write("flake.nix", "enableDebugging = true;\n")
        stdout = io.StringIO()
        stderr = io.StringIO()
        with redirect_stdout(stdout), redirect_stderr(stderr):
            status = hygiene.main(["--root", self.root])
        self.assertEqual(status, 1)
        self.assertIn("flake.nix:1: enableDebugging", stderr.getvalue())


if __name__ == "__main__":
    unittest.main()
