#!/usr/bin/env python3

"""Check type information used by LibrePaint across Android shared libraries."""

import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path


# These types are constructed and inspected (or thrown and caught) on opposite
# sides of a shared-library boundary. Their RTTI must come from their owner.
# Other duplicate project RTTI serves virtual dispatch or same-library casts;
# KisShared is nonpolymorphic.
OWNERS = {
    "_ZTI13KisAnnotation": "libkritaimage.so",
    "_ZTI16PSDResourceBlock": "libkritapsd.so",
    "_ZTI22PSDInterpretedResource": "libkritapsd.so",
    "_ZTIN22FreehandStrokeStrategy4DataE": "libkritapainting.so",
    "_ZTIN33KisAsynchronousStrokeUpdateHelper10UpdateDataE": "libkritapainting.so",
    "_ZTIN23KisFilterStrokeStrategy13FilterJobDataE": "libkritapainting.so",
    "_ZTIN23KisFilterStrokeStrategy15IdleBarrierDataE": "libkritapainting.so",
    "_ZTIN17KisAslReaderUtils17ASLParseExceptionE": "libkritapsdutils.so",
    "_ZTIN17KisAslWriterUtils17ASLWriteExceptionE": "libkritapsdutils.so",
}


def main() -> int:
    if len(sys.argv) != 4:
        print("usage: audit-android-cross-library-rtti.py <apk-or-aab> <abi> <llvm-readelf>", file=sys.stderr)
        return 2

    package, abi, readelf = sys.argv[1:]
    prefix = f"{'base/' if package.endswith('.aab') else ''}lib/{abi}/"
    definitions = {symbol: [] for symbol in OWNERS}

    with zipfile.ZipFile(package) as archive, tempfile.TemporaryDirectory() as temp_dir:
        extracted = Path(temp_dir) / "library.so"
        for member in archive.infolist():
            if not member.filename.startswith(prefix) or not member.filename.endswith(".so"):
                continue
            with archive.open(member) as source, extracted.open("wb") as destination:
                while chunk := source.read(1024 * 1024):
                    destination.write(chunk)
            result = subprocess.run(
                [readelf, "--dyn-syms", "--wide", str(extracted)],
                capture_output=True,
                text=True,
                check=True,
            )
            for line in result.stdout.splitlines():
                fields = line.split()
                if len(fields) < 8 or fields[3] != "OBJECT" or fields[6] == "UND":
                    continue
                symbol = fields[7].split("@", 1)[0]
                if symbol in definitions:
                    definitions[symbol].append(Path(member.filename).name)

    failures = []
    for symbol, owner in OWNERS.items():
        if definitions[symbol] != [owner]:
            failures.append(f"{symbol}: expected [{owner}], found {definitions[symbol]}")
    if failures:
        print("Android cross-library RTTI audit failed:\n" + "\n".join(failures), file=sys.stderr)
        return 1
    print(f"Android cross-library RTTI audit passed: {package} ({abi})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
