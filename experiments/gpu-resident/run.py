#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Build and measure the standalone document-residency comparison."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import statistics
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
SOURCE = Path(__file__).resolve().parent
BUILD = ROOT / "build/gpu-resident/ninja"


def run(command, **kwargs):
    return subprocess.run(command, check=True, text=True, **kwargs)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def prepare():
    # Read the pinned remote input; evaluating this expression does not import
    # the application source into the Nix store or reevaluate its development shell.
    revision = json.loads((ROOT / "flake.lock").read_text())["nodes"]["nixpkgs"]["locked"]["rev"]
    package = f"github:NixOS/nixpkgs/{revision}#wgpu-native"
    paths = run(["nix", "build", "--no-link", "--print-out-paths", package, package + ".dev"],
                capture_output=True).stdout.splitlines()
    include = next(Path(p) / "include" for p in paths if (Path(p) / "include/webgpu/wgpu.h").exists())
    library = next(p for root in paths for p in (Path(root) / "lib").glob("libwgpu_native.*")
                   if p.suffix in (".dylib", ".so"))
    run(["cmake", "-S", str(SOURCE), "-B", str(BUILD), "-G", "Ninja",
         "-DCMAKE_BUILD_TYPE=Release", f"-DWGPU_INCLUDE_DIR={include}", f"-DWGPU_LIBRARY={library}"])


def summarize(report):
    groups = {}
    for item in report["runs"]:
        key = (item["case"], item["in_flight"], item["mode"])
        groups.setdefault(key, []).append(item)
    result = []
    for (case, depth, mode), rows in groups.items():
        values = [v for row in rows for v in row["samples_ms"]]
        cpus = [v for row in rows for v in row["cpu_samples_ms"]]
        frames = [v for row in rows for v in row["frame_completion_ms"]]
        tails = [v for row in rows for v in row["last_input_tail_ms"]]
        entry = dict(case=case, in_flight=depth, mode=mode, samples=len(values),
                     median_ms=statistics.median(values), p95_ms=sorted(values)[math.ceil(.95 * len(values)) - 1],
                     median_cpu_ms=statistics.median(cpus), median_tail_ms=statistics.median(tails),
                     process_medians_ms=[statistics.median(row["samples_ms"]) for row in rows],
                     pixel_upload_mib=rows[0]["pixel_upload_bytes_per_sample"] / 2**20,
                     command_upload_kib=rows[0]["command_upload_bytes_per_sample"] / 1024,
                     pixel_readback_bytes=rows[0]["pixel_readback_bytes_per_sample"],
                     gpu_allocated_mib=rows[0]["gpu_allocated_bytes"] / 2**20)
        if frames:
            entry.update(frame_median_ms=statistics.median(frames),
                         frame_p95_ms=sorted(frames)[math.ceil(.95 * len(frames)) - 1])
        result.append(entry)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prepare", action="store_true", help="resolve pinned Nix dependency and configure")
    parser.add_argument("--build-only", action="store_true")
    parser.add_argument("--suite", choices=("smoke", "full", "profile", "paced"), default="smoke")
    parser.add_argument("--rounds", type=int, default=3)
    parser.add_argument("--samples", type=int, default=8)
    parser.add_argument("--workers", type=int, default=4)
    parser.add_argument("--cases", help="comma-separated workload filter")
    parser.add_argument("--modes", default="cpu,hybrid,gpu")
    parser.add_argument("--depths", default="1,3")
    parser.add_argument("--output", type=Path, default=ROOT / "build/gpu-resident/results.json")
    args = parser.parse_args()
    if args.rounds < 1 or args.samples < 1:
        parser.error("rounds and samples must be positive")
    if args.prepare or not (BUILD / "CMakeCache.txt").exists():
        prepare()
    run(["cmake", "--build", str(BUILD)])
    if args.build_only:
        return
    run(["ctest", "--test-dir", str(BUILD), "--output-on-failure"])
    binary = BUILD / "resident_poc"
    cases = {
        "light": ["--size", "1024", "--layers", "1", "--diameter", "32"],
        "heavy": ["--size", "4096", "--layers", "24", "--diameter", "512"],
        "varying": ["--size", "4096", "--layers", "24", "--diameter", "512", "--varying"],
        "shape": ["--size", "4096", "--layers", "24", "--diameter", "96", "--shape"],
    }
    if args.suite == "smoke":
        cases = {"smoke": ["--size", "256", "--layers", "3", "--diameter", "96"]}
    if args.cases:
        requested = args.cases.split(",")
        if set(requested) - cases.keys():
            parser.error("unknown case")
        cases = {key: cases[key] for key in requested}
    modes = args.modes.split(",")
    if set(modes) - {"cpu", "hybrid", "gpu"}:
        parser.error("unknown mode")
    depths = [int(x) for x in args.depths.split(",")]
    if args.suite in ("profile", "smoke", "paced"):
        depths = [1]
    if set(depths) - {1, 2, 3}:
        parser.error("in-flight depth must be 1, 2 or 3")
    report = {
        "scope": "standalone RGBA8 CPU vs GPU document; common wgpu display; offscreen GPU completion, not physical presentation",
        "timing": "binning + paint + composition + upload + render + completion; initialization/oracle/readback excluded",
        "source_sha256": {p.name: digest(p) for p in sorted(SOURCE.iterdir())
                          if p.suffix in (".cpp", ".h", ".wgsl", ".py") or p.name == "CMakeLists.txt"},
        "binary_sha256": digest(binary),
        "base_commit": run(["git", "rev-parse", "HEAD"], capture_output=True).stdout.strip(),
        "suite": args.suite, "runs": [],
    }
    cache = (BUILD / "CMakeCache.txt").read_text()
    library = Path(re.search(r"^WGPU_LIBRARY:FILEPATH=(.+)$", cache, re.M)[1])
    report["wgpu_library_sha256"] = digest(library)
    report["compiler"] = run(["clang++", "--version"], capture_output=True).stdout.splitlines()[0]
    report["nixpkgs_revision"] = json.loads((ROOT / "flake.lock").read_text())["nodes"]["nixpkgs"]["locked"]["rev"]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    logs = args.output.parent / (args.output.stem + "-logs")
    logs.mkdir(exist_ok=True)
    start = time.monotonic()
    for round_index in range(args.rounds):
        order = modes[round_index % len(modes):] + modes[:round_index % len(modes)]
        for case, flags in cases.items():
            for depth in depths:
                for mode in order:
                    label = f"r{round_index + 1}-{case}-q{depth}-{mode}"
                    print(label, flush=True)
                    command = [str(binary), "--mode", mode, *flags, "--in-flight", str(depth),
                               "--samples", str(args.samples), "--warmup", "2", "--workers", str(args.workers)]
                    if args.suite == "profile":
                        command += ["--timestamps"]
                    if args.suite == "paced":
                        command += ["--interval-ms", "4"]
                    completed = subprocess.run(command, capture_output=True, text=True, timeout=180)
                    (logs / (label + ".log")).write_text(completed.stdout + completed.stderr)
                    completed.check_returncode()
                    row = json.loads(completed.stdout)
                    row.update(case=case, round=round_index + 1)
                    if not row["verified"]:
                        raise RuntimeError(f"{label}: verification failed")
                    for other in report["runs"]:
                        if other["case"] == case and (other["document_hash"], other["projection_hash"]) != (row["document_hash"], row["projection_hash"]):
                            raise RuntimeError(f"{label}: cross-mode pixel mismatch")
                    report["runs"].append(row)
                    report["summary"] = summarize(report)
                    report["elapsed_seconds"] = time.monotonic() - start
                    args.output.write_text(json.dumps(report, indent=2) + "\n")
                    print(f"  {row['median_ms']:.3f} ms, CPU {row['median_cpu_ms']:.3f} ms, pixels {row['pixel_upload_bytes_per_sample'] / 2**20:.3f} MiB", flush=True)
    print(args.output)


if __name__ == "__main__":
    main()
