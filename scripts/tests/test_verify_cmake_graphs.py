#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
import sys
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT_PATH = REPO_ROOT / "scripts/architecture/verify_cmake_graphs.py"
SPEC = importlib.util.spec_from_file_location("verify_cmake_graphs", SCRIPT_PATH)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"cannot import {SCRIPT_PATH}")
verify_cmake_graphs = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = verify_cmake_graphs
SPEC.loader.exec_module(verify_cmake_graphs)


class VerifyCMakeGraphsTests(unittest.TestCase):
    def test_jobs_cover_all_platforms_on_their_build_hosts(self) -> None:
        jobs = verify_cmake_graphs.build_jobs(
            remote_host="nixos",
            remote_repository=Path("/home/masato/worktrees/librepaint-r1-g1-verify"),
        )

        self.assertEqual(
            list(jobs), ["macos", "linux", "ios", "android", "windows"]
        )
        self.assertNotEqual(jobs["macos"][0], "ssh")
        self.assertNotEqual(jobs["ios"][0], "ssh")
        for platform in ("linux", "android", "windows"):
            with self.subTest(platform=platform):
                self.assertEqual(jobs[platform][:2], ["ssh", "nixos"])
                self.assertIn(
                    f"regenerate_cmake_graph.py {platform} --check",
                    jobs[platform][-1],
                )

    def test_revision_check_requires_identical_clean_commits(self) -> None:
        self.assertEqual(
            verify_cmake_graphs.revision_diagnostic(
                local_revision="abc",
                remote_revision="abc",
                local_status="",
                remote_status="",
            ),
            None,
        )
        self.assertIn(
            "different commits",
            verify_cmake_graphs.revision_diagnostic(
                local_revision="abc",
                remote_revision="def",
                local_status="",
                remote_status="",
            ),
        )
        self.assertIn(
            "worktree is dirty",
            verify_cmake_graphs.revision_diagnostic(
                local_revision="abc",
                remote_revision="abc",
                local_status=" M tracked-file",
                remote_status="",
            ),
        )


if __name__ == "__main__":
    unittest.main()
