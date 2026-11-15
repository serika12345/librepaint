import importlib.util
import os
import shutil
import tempfile
import unittest

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def load(filename):
    path = os.path.join(SCRIPTS, filename)
    spec = importlib.util.spec_from_file_location("check_test_gating", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


gating = load("check-test-gating.py")


def rules(text):
    return [(line, rule) for line, rule, _message in gating.check_file("CMakeLists.txt", text)]


class CheckFileTests(unittest.TestCase):
    def test_gated_subdirectory_is_accepted(self):
        self.assertEqual(rules("if(BUILD_TESTING)\n    add_subdirectory(tests)\nendif()\n"), [])

    def test_ungated_tests_subdirectory_is_rejected(self):
        self.assertEqual(rules("add_subdirectory( tests )\n"), [(1, "gate")])

    def test_nested_test_component_is_rejected(self):
        self.assertEqual(rules("add_subdirectory(brush/tests)\n"), [(1, "gate")])

    def test_benchmarks_are_rejected(self):
        self.assertEqual(rules("add_subdirectory(benchmarks EXCLUDE_FROM_ALL)\n"), [(1, "gate")])

    def test_other_subdirectories_are_accepted(self):
        self.assertEqual(rules("add_subdirectory(testsupport)\nadd_subdirectory(styles)\n"), [])

    def test_guard_may_be_one_of_several_conditions(self):
        text = "if(ANDROID AND BUILD_TESTING)\nadd_subdirectory(tests)\nendif()\n"
        self.assertEqual(rules(text), [])

    def test_negated_guard_is_rejected(self):
        text = "if(NOT BUILD_TESTING)\nadd_subdirectory(tests)\nendif()\n"
        self.assertEqual(rules(text), [(2, "gate")])

    def test_or_condition_is_rejected(self):
        text = "if(ANDROID OR BUILD_TESTING)\nadd_subdirectory(tests)\nendif()\n"
        self.assertEqual(rules(text), [(2, "gate")])

    def test_else_branch_is_not_guarded(self):
        text = "if(BUILD_TESTING)\nelse()\nadd_subdirectory(tests)\nendif()\n"
        self.assertEqual(rules(text), [(3, "gate")])

    def test_elseif_branch_can_be_guarded(self):
        text = "if(WIN32)\nelseif(BUILD_TESTING)\nadd_subdirectory(tests)\nendif()\n"
        self.assertEqual(rules(text), [])

    def test_guard_ends_with_endif(self):
        text = "if(BUILD_TESTING)\nendif()\nadd_subdirectory(tests)\n"
        self.assertEqual(rules(text), [(3, "gate")])

    def test_commented_call_is_ignored(self):
        self.assertEqual(rules("#add_subdirectory(tests)\n"), [])

    def test_test_source_in_a_library_is_rejected(self):
        text = "set(lib_SRCS\n    a.cpp\n    tests/MockShapes.cpp\n)\n"
        self.assertEqual(rules(text), [(3, "source")])

    def test_test_source_behind_the_guard_is_accepted(self):
        text = "if(BUILD_TESTING)\n    list(APPEND lib_SRCS tests/MockShapes.cpp)\nendif()\n"
        self.assertEqual(rules(text), [])

    def test_ungated_test_macro_is_rejected(self):
        self.assertEqual(rules("kis_add_test(Foo.cpp)\n"), [(1, "macro")])

    def test_gated_test_macro_is_accepted(self):
        self.assertEqual(rules("if(BUILD_TESTING)\nkis_add_test(Foo.cpp)\nendif()\n"), [])


class TreeTests(unittest.TestCase):
    def setUp(self):
        self.root = tempfile.mkdtemp(prefix="test-gating-")
        self.addCleanup(shutil.rmtree, self.root)

    def write(self, relative_path, text):
        path = os.path.join(self.root, relative_path)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8") as handle:
            handle.write(text)

    def test_files_below_test_directories_and_cmake_modules_are_exempt(self):
        self.write("libs/a/tests/CMakeLists.txt", "kis_add_test(Foo.cpp)\n")
        self.write("cmake/modules/Add.cmake", "kis_add_test(Foo.cpp)\n")
        self.write("libs/a/CMakeLists.txt", "if(BUILD_TESTING)\nadd_subdirectory(tests)\nendif()\n")
        self.assertEqual(gating.check(self.root), [])

    def test_violation_names_the_file_and_line(self):
        self.write("libs/a/CMakeLists.txt", "project(a)\nadd_subdirectory(tests)\n")
        self.assertEqual(
            [(path, line, rule) for path, line, rule, _message in gating.check(self.root)],
            [(os.path.join("libs", "a", "CMakeLists.txt"), 2, "gate")],
        )


if __name__ == "__main__":
    unittest.main()
