#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

"""Verify the identity and signature of a distributable Android APK."""

import argparse
from pathlib import Path
import re
import subprocess
import sys


APPLICATION_ID = "io.github.serika12345.librepaint"
APPLICATION_LABEL = "LibrePaint"
MIN_SDK = "28"
TARGET_SDK = "35"


class VerificationError(Exception):
    pass


def run_tool(*command: str) -> str:
    result = subprocess.run(command, capture_output=True, text=True, check=False)
    if result.returncode:
        raise VerificationError(
            f"{' '.join(command[:2])} failed ({result.returncode}): {result.stderr.strip()}"
        )
    return result.stdout


def require_field(text: str, pattern: str, expected: str, field: str) -> None:
    match = re.search(pattern, text, re.MULTILINE)
    actual = match.group(1) if match else "missing"
    if actual != expected:
        raise VerificationError(f"{field}: expected {expected}, found {actual}")


def verify_metadata(apk: Path, aapt2: str, version_name: str, version_code: int) -> None:
    badging = run_tool(aapt2, "dump", "badging", str(apk))
    require_field(badging, r"^package: name='([^']+)'", APPLICATION_ID, "application ID")
    require_field(badging, r"^package:.* versionCode='([^']+)'", str(version_code), "versionCode")
    require_field(badging, r"^package:.* versionName='([^']+)'", version_name, "versionName")
    require_field(badging, r"^minSdkVersion:'([^']+)'", MIN_SDK, "minSdkVersion")
    require_field(badging, r"^targetSdkVersion:'([^']+)'", TARGET_SDK, "targetSdkVersion")
    require_field(badging, r"^application-label:'([^']+)'", APPLICATION_LABEL, "application label")


def verify_signature(apk: Path, apksigner: str, certificate_sha256: str) -> None:
    output = run_tool(
        apksigner,
        "verify",
        "--verbose",
        "--print-certs",
        "--min-sdk-version",
        MIN_SDK,
        str(apk),
    )
    match = re.search(r"^Signer #1 certificate SHA-256 digest: ([0-9a-fA-F:]+)$", output, re.MULTILINE)
    actual = match.group(1).replace(":", "").lower() if match else "missing"
    expected = certificate_sha256.replace(":", "").lower()
    if not re.fullmatch(r"[0-9a-f]{64}", expected):
        raise VerificationError("expected certificate SHA-256 must be 64 hexadecimal characters")
    if actual != expected:
        raise VerificationError(f"signing certificate: expected {expected}, found {actual}")
    if re.search(r"^Number of signers: ([^\n]+)$", output, re.MULTILINE):
        require_field(output, r"^Number of signers: ([^\n]+)$", "1", "signer count")


def verify_apk(
    apk: Path,
    aapt2: str,
    zipalign: str,
    version_name: str,
    version_code: int,
    apksigner: str | None = None,
    certificate_sha256: str | None = None,
) -> None:
    if not apk.is_file() or apk.stat().st_size == 0:
        raise VerificationError(f"APK is missing or empty: {apk}")
    run_tool(zipalign, "-c", "-P", "16", "-v", "4", str(apk))
    verify_metadata(apk, aapt2, version_name, version_code)
    if apksigner is not None:
        if certificate_sha256 is None:
            raise VerificationError("expected signing certificate is required")
        verify_signature(apk, apksigner, certificate_sha256)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("apk", type=Path)
    parser.add_argument("--aapt2", required=True)
    parser.add_argument("--zipalign", required=True)
    parser.add_argument("--version-name", required=True)
    parser.add_argument("--version-code", required=True, type=int)
    parser.add_argument("--apksigner")
    parser.add_argument("--certificate-sha256")
    args = parser.parse_args()
    try:
        verify_apk(
            args.apk,
            args.aapt2,
            args.zipalign,
            args.version_name,
            args.version_code,
            args.apksigner,
            args.certificate_sha256,
        )
    except VerificationError as error:
        print(f"Android release APK verification failed: {error}", file=sys.stderr)
        return 1
    print(f"Android release APK verified: {args.apk}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
