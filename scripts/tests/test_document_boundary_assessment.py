#!/usr/bin/env python3

from __future__ import annotations

import copy
import importlib.util
import sys
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT_PATH = REPO_ROOT / "scripts/architecture/check_document_boundary_assessment.py"
ASSESSMENT_PATH = REPO_ROOT / "docs/architecture/document-boundary-assessment.json"
SPEC = importlib.util.spec_from_file_location(
    "check_document_boundary_assessment", SCRIPT_PATH
)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"cannot import {SCRIPT_PATH}")
check_document_boundary_assessment = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = check_document_boundary_assessment
SPEC.loader.exec_module(check_document_boundary_assessment)


class DocumentBoundaryAssessmentTests(unittest.TestCase):
    def load_assessment(self):
        return check_document_boundary_assessment.load_assessment(ASSESSMENT_PATH)

    def validate(self, assessment) -> None:
        check_document_boundary_assessment.validate_assessment(
            assessment,
            repository_root=REPO_ROOT,
        )

    def test_complete_assessment_is_valid(self) -> None:
        assessment = self.load_assessment()

        self.validate(assessment)

        self.assertEqual(
            sum(
                len(group["classes"])
                for group in assessment["classGroups"]
            ),
            22,
        )
        self.assertEqual(
            sum(
                len(group["methods"])
                for group in assessment["kisDocumentMethodGroups"]
            ),
            129,
        )
        self.assertEqual(assessment["nextAction"]["gate"], "R1-G6f")

    def test_missing_document_class_is_rejected(self) -> None:
        assessment = copy.deepcopy(self.load_assessment())
        assessment["classGroups"][0]["classes"].pop()

        with self.assertRaisesRegex(
            check_document_boundary_assessment.DocumentBoundaryError,
            "document class assessments do not match",
        ):
            self.validate(assessment)

    def test_unknown_document_class_is_rejected(self) -> None:
        assessment = copy.deepcopy(self.load_assessment())
        assessment["classGroups"][0]["classes"][0] = "MissingClass"
        assessment["classGroups"][0]["classes"].sort()

        with self.assertRaisesRegex(
            check_document_boundary_assessment.DocumentBoundaryError,
            "document class assessments do not match",
        ):
            self.validate(assessment)

    def test_missing_kis_document_method_is_rejected(self) -> None:
        assessment = copy.deepcopy(self.load_assessment())
        assessment["kisDocumentMethodGroups"][0]["methods"].pop()

        with self.assertRaisesRegex(
            check_document_boundary_assessment.DocumentBoundaryError,
            "KisDocument method assessments do not match",
        ):
            self.validate(assessment)

    def test_duplicate_kis_document_method_is_rejected(self) -> None:
        assessment = copy.deepcopy(self.load_assessment())
        method = assessment["kisDocumentMethodGroups"][0]["methods"][0]
        assessment["kisDocumentMethodGroups"][1]["methods"].append(method)
        assessment["kisDocumentMethodGroups"][1]["methods"].sort()

        with self.assertRaisesRegex(
            check_document_boundary_assessment.DocumentBoundaryError,
            "KisDocument methods must be assigned exactly once",
        ):
            self.validate(assessment)

    def test_unjustified_abstraction_is_rejected(self) -> None:
        assessment = copy.deepcopy(self.load_assessment())
        assessment["abstractionAssessment"][0]["decision"] = "introduce"
        assessment["abstractionAssessment"][0]["currentRequirement"] = False

        with self.assertRaisesRegex(
            check_document_boundary_assessment.DocumentBoundaryError,
            "introduced abstraction requires a current requirement",
        ):
            self.validate(assessment)


if __name__ == "__main__":
    unittest.main()
