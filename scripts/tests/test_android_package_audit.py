#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

import os
from pathlib import Path
import subprocess
import tempfile
import textwrap
import unittest
import zipfile


SCRIPT = Path(__file__).resolve().parents[1] / "platform/audit-android-package"


class AndroidPackageAuditTest(unittest.TestCase):
    def inspect(self, abi="arm64-v8a", mode="complete"):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            package = root / "contract.apk"
            with zipfile.ZipFile(package, "w") as archive:
                archive.writestr("classes.dex", b"dex\n035\0")
                for name in (
                    f"libQt6Core_{abi}.so",
                    f"libplugins_platforms_qtforandroid_{abi}.so",
                    "libc++_shared.so",
                ):
                    archive.writestr(f"lib/{abi}/{name}", b"ELF inspection fixture")

            inspector = root / "readelf"
            inspector.write_text(textwrap.dedent(r"""
                #!/usr/bin/env python3
                import os
                from pathlib import Path
                import sys

                mode = os.environ["LIBREPAINT_AUDIT_TEST_MODE"]
                abi = os.environ["LIBREPAINT_AUDIT_TEST_ABI"]
                machine = "AArch64" if abi == "arm64-v8a" else "Advanced Micro Devices X86-64"
                primary = Path(sys.argv[-1]).name.startswith("libQt6Core_")

                def write_all(data):
                    while data:
                        data = data[os.write(1, data):]

                try:
                    if "-h" in sys.argv or "--file-header" in sys.argv:
                        if primary and mode == "wrong-machine":
                            machine = "Advanced Micro Devices X86-64" if abi == "arm64-v8a" else "AArch64"
                        write_all(("ELF Header:\n  Class: ELF64\n  Type: DYN (Shared object file)\n"
                                   "  Machine: " + machine + "\n").encode())
                        if primary:
                            # Exceed pipe capacity to exercise completion without timing assumptions.
                            write_all(b"  Additional header detail\n" * 131072)
                            if mode == "failure":
                                print("fixture inspector failed after header", file=sys.stderr)
                                sys.exit(23)
                    elif "-lW" in sys.argv or "--program-headers" in sys.argv:
                        write_all(b"LOAD 0x0 0x0 0x0 0x1000 0x1000 R E 0x4000\n")
                    elif "-dW" not in sys.argv and "--dynamic" not in sys.argv:
                        sys.exit(2)
                except BrokenPipeError:
                    sys.exit(74)
                """).lstrip("\n"))
            inspector.chmod(0o755)
            aapt = root / "aapt2"
            aapt.write_text(
                "#!/usr/bin/env python3\n"
                "print(\"minSdkVersion:'28'\\ntargetSdkVersion:'35'\")\n"
            )
            aapt.chmod(0o755)
            signer = root / "apksigner"
            signer.write_text("#!/bin/sh\nexit 0\n")
            signer.chmod(0o755)
            return subprocess.run(
                ["bash", str(SCRIPT), str(package), abi, str(inspector), str(aapt), "test"],
                env=os.environ | {
                    "LIBREPAINT_AUDIT_TEST_MODE": mode,
                    "LIBREPAINT_AUDIT_TEST_ABI": abi,
                },
                capture_output=True,
                text=True,
                timeout=30,
                check=False,
            )

    def test_accepts_matching_architecture_after_complete_inspector_output(self):
        for abi in ("arm64-v8a", "x86_64"):
            with self.subTest(abi=abi):
                result = self.inspect(abi)
                self.assertEqual(result.returncode, 0, result.stderr)

    def test_rejects_another_architecture(self):
        result = self.inspect(mode="wrong-machine")
        self.assertNotEqual(result.returncode, 0)

    def test_rejects_inspector_failure_after_matching_header(self):
        result = self.inspect(mode="failure")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("fixture inspector failed after header", result.stderr)


if __name__ == "__main__":
    unittest.main()
