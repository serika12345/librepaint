#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT_PATH = REPO_ROOT / "scripts/architecture/measure_change_impact.py"
SPEC = importlib.util.spec_from_file_location("measure_change_impact", SCRIPT_PATH)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"cannot import {SCRIPT_PATH}")
measure_change_impact = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = measure_change_impact
SPEC.loader.exec_module(measure_change_impact)


class MeasureChangeImpactTests(unittest.TestCase):
    def test_dependency_records_keep_valid_header_consumers_only(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            build_directory = Path(temporary_directory) / "build"
            header = Path(temporary_directory) / "repo/libs/global/KoID.h"
            lines = [
                "libs/global/CMakeFiles/global.dir/KoID.cpp.o: #deps 2, deps mtime 1 (VALID)\n",
                f"    {header}\n",
                "    /usr/include/utility\n",
                "libs/old/CMakeFiles/old.dir/old.cpp.o: #deps 1, deps mtime 1 (STALE)\n",
                f"    {header}\n",
                "libs/paint/CMakeFiles/paint.dir/use.cpp.o: #deps 1, deps mtime 1 (VALID)\n",
                f"    {Path('..') / 'other/libs/global/KoID.h'}\n",
            ]

            result = measure_change_impact.parse_dependency_records(
                lines,
                build_directory=build_directory,
                header_path=header,
            )

            self.assertEqual(
                result.objects,
                {"libs/global/CMakeFiles/global.dir/KoID.cpp.o"},
            )
            self.assertEqual(result.stale_records, 1)

    def test_direct_includers_are_split_between_product_and_test_files(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            repository_root = Path(temporary_directory)
            header = repository_root / "libs/global/KoID.h"
            product = repository_root / "libs/paint/use_id.cpp"
            test = repository_root / "libs/paint/tests/use_id_test.cpp"
            unrelated = repository_root / "libs/paint/unrelated.cpp"
            for path in (header, product, test, unrelated):
                path.parent.mkdir(parents=True, exist_ok=True)
            header.write_text("#pragma once\n", encoding="utf-8")
            product.write_text('#include <KoID.h>\n', encoding="utf-8")
            test.write_text('#include "global/KoID.h"\n', encoding="utf-8")
            unrelated.write_text('#include <QString>\n', encoding="utf-8")

            result = measure_change_impact.discover_direct_includers(
                repository_root,
                Path("libs/global/KoID.h"),
            )

            self.assertEqual(result.product_files, ["libs/paint/use_id.cpp"])
            self.assertEqual(
                result.test_files,
                ["libs/paint/tests/use_id_test.cpp"],
            )

    def test_object_target_name_is_read_from_ninja_output_path(self) -> None:
        self.assertEqual(
            measure_change_impact.target_name_from_object(
                "libs/global/CMakeFiles/kritaglobalidobjects.dir/KoID.cpp.o"
            ),
            "kritaglobalidobjects",
        )
        self.assertIsNone(
            measure_change_impact.target_name_from_object("generated/unknown.o")
        )

    def test_benchmark_targets_are_excluded_from_product_counts(self) -> None:
        targets = {
            "paint": {"sourceDirectory": "libs/paint"},
            "paint_test": {"sourceDirectory": "libs/paint/tests"},
            "paint_benchmark": {"sourceDirectory": "libs/paint/benchmarks"},
        }

        product, test, unknown = measure_change_impact._split_targets(
            set(targets), targets
        )

        self.assertEqual(product, {"paint"})
        self.assertEqual(test, {"paint_test", "paint_benchmark"})
        self.assertEqual(unknown, set())

    def test_reverse_dependency_closure_includes_transitive_consumers(self) -> None:
        targets = {
            "foundation": {"dependencies": []},
            "paint": {"dependencies": ["foundation"]},
            "plugin": {"dependencies": ["paint"]},
            "unrelated": {"dependencies": []},
        }

        closure = measure_change_impact._reverse_dependency_closure(
            {"foundation"}, targets
        )

        self.assertEqual(closure, {"foundation", "paint", "plugin"})


if __name__ == "__main__":
    unittest.main()
