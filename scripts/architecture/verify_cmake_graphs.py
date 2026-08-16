#!/usr/bin/env python3

from __future__ import annotations

import argparse
import concurrent.futures
import platform
import shlex
import subprocess
import sys
from pathlib import Path


SCRIPT_DIRECTORY = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIRECTORY.parents[1]
REGENERATOR = SCRIPT_DIRECTORY / "regenerate_cmake_graph.py"
MATRIX_REGENERATOR = SCRIPT_DIRECTORY / "regenerate_cmake_graph_matrix.py"
PLATFORM_HOSTS = {
    "macos": "local",
    "linux": "remote",
    "ios": "local",
    "android": "remote",
    "windows": "remote",
}


class VerificationError(RuntimeError):
    """Raised when the cross-host platform verification cannot proceed."""


def build_jobs(
    *, remote_host: str, remote_repository: Path
) -> dict[str, list[str]]:
    jobs: dict[str, list[str]] = {}
    remote_directory = shlex.quote(str(remote_repository))
    for target_platform, target_host in PLATFORM_HOSTS.items():
        if target_host == "local":
            jobs[target_platform] = [
                sys.executable,
                str(REGENERATOR),
                target_platform,
                "--check",
            ]
        else:
            remote_command = (
                f"cd -- {remote_directory} && "
                "export NIX_CONFIG='eval-cache = false' && "
                "nix develop .#test --command "
                "./scripts/architecture/regenerate_cmake_graph.py "
                f"{target_platform} --check"
            )
            jobs[target_platform] = ["ssh", remote_host, remote_command]
    return jobs


def revision_diagnostic(
    *,
    local_revision: str,
    remote_revision: str,
    local_status: str,
    remote_status: str,
) -> str | None:
    if local_revision != remote_revision:
        return (
            "local and remote repositories are on different commits: "
            f"{local_revision} != {remote_revision}"
        )
    if local_status:
        return "local worktree is dirty; commit the verification inputs first"
    if remote_status:
        return "remote worktree is dirty; use a clean verification worktree"
    return None


def _run(command: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command,
        cwd=REPO_ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )


def _require_success(
    result: subprocess.CompletedProcess[str], description: str
) -> str:
    if result.returncode != 0:
        diagnostic = result.stderr.strip() or result.stdout.strip()
        raise VerificationError(f"{description} failed: {diagnostic}")
    return result.stdout.strip()


def verify_preconditions(*, remote_host: str, remote_repository: Path) -> None:
    if platform.system() != "Darwin":
        raise VerificationError("the local macOS and iOS verification host must be Darwin")

    local_revision = _require_success(
        _run(["git", "rev-parse", "HEAD"]), "local revision query"
    )
    local_status = _require_success(
        _run(["git", "status", "--porcelain", "--untracked-files=all"]),
        "local status query",
    )
    quoted_repository = shlex.quote(str(remote_repository))
    remote_revision = _require_success(
        _run(
            [
                "ssh",
                remote_host,
                f"git -C {quoted_repository} rev-parse HEAD",
            ]
        ),
        "remote revision query",
    )
    remote_status = _require_success(
        _run(
            [
                "ssh",
                remote_host,
                "git -C "
                f"{quoted_repository} status --porcelain --untracked-files=all",
            ]
        ),
        "remote status query",
    )
    remote_system = _require_success(
        _run(["ssh", remote_host, "printf '%s:%s' \"$(uname -s)\" \"$(uname -m)\""]),
        "remote host query",
    )
    if remote_system != "Linux:x86_64":
        raise VerificationError(
            f"the Linux, Android, and Windows verification host must be Linux:x86_64; got {remote_system}"
        )

    diagnostic = revision_diagnostic(
        local_revision=local_revision,
        remote_revision=remote_revision,
        local_status=local_status,
        remote_status=remote_status,
    )
    if diagnostic is not None:
        raise VerificationError(diagnostic)


def verify_platforms(*, remote_host: str, remote_repository: Path) -> None:
    jobs = build_jobs(
        remote_host=remote_host,
        remote_repository=remote_repository,
    )
    failures: list[str] = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=len(jobs)) as executor:
        futures = {
            executor.submit(_run, command): target_platform
            for target_platform, command in jobs.items()
        }
        for future in concurrent.futures.as_completed(futures):
            target_platform = futures[future]
            result = future.result()
            if result.returncode == 0:
                print(f"[{target_platform}] verified")
                continue
            diagnostic = result.stderr.strip() or result.stdout.strip()
            failures.append(f"[{target_platform}] {diagnostic}")

    if failures:
        raise VerificationError("platform verification failed:\n" + "\n".join(failures))

    matrix_result = _run([sys.executable, str(MATRIX_REGENERATOR), "--check"])
    _require_success(matrix_result, "joint target matrix verification")
    print("[matrix] verified")


def _argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Verify the recorded CMake graphs concurrently on Darwin and an "
            "x86_64 Linux build host."
        )
    )
    parser.add_argument("--remote-host", required=True)
    parser.add_argument("--remote-repository", type=Path, required=True)
    return parser


def main(arguments: list[str] | None = None) -> int:
    options = _argument_parser().parse_args(arguments)
    try:
        verify_preconditions(
            remote_host=options.remote_host,
            remote_repository=options.remote_repository,
        )
        verify_platforms(
            remote_host=options.remote_host,
            remote_repository=options.remote_repository,
        )
    except (OSError, VerificationError) as error:
        print(f"verify-cmake-graphs: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
