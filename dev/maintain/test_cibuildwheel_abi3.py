#!/usr/bin/env python3
"""Build a single abi3 wheel with cibuildwheel to sanity-check packaging.

This developer-only helper builds an abi3 wheel using cibuildwheel and
verifies that the resulting filename carries the expected ``cp38-abi3`` tag.
It defaults to building a single manylinux wheel for the host architecture to
keep turnaround times reasonable.
"""
from __future__ import annotations

import argparse
import os
import platform
import subprocess
import sys
import tempfile
from pathlib import Path


def _default_identifier() -> str:
    arch_map = {
        "x86_64": "x86_64",
        "aarch64": "aarch64",
    }
    machine = platform.machine()
    arch = arch_map.get(machine)
    if arch is None:
        raise SystemExit(f"Unsupported architecture for cibuildwheel smoke test: {machine}")
    return f"cp312-manylinux_{arch}"


def run(identifier: str, keep_wheels: bool, *, container_engine: str | None) -> Path:
    env = os.environ.copy()
    env.setdefault("CIBW_PLATFORM", "linux")
    if container_engine:
        env["CIBW_CONTAINER_ENGINE"] = container_engine
    else:
        env.setdefault("CIBW_CONTAINER_ENGINE", "podman")
    env.setdefault("CIBW_BUILD", identifier)
    env.setdefault("CIBW_BUILD_VERBOSITY", "1")
    env.setdefault("CIBW_ARCHS_LINUX", "native")
    env.setdefault("PYTHON_LIMITED_API", "cp38")

    output_dir_obj = None
    if keep_wheels:
        output_dir = Path.cwd() / "wheelhouse"
        output_dir.mkdir(exist_ok=True)
    else:
        output_dir_obj = tempfile.TemporaryDirectory()
        output_dir = Path(output_dir_obj.name)

    cmd = [
        sys.executable,
        "-m",
        "cibuildwheel",
        "--only",
        identifier,
        "--output-dir",
        str(output_dir),
    ]
    subprocess.run(cmd, env=env, check=True)

    wheels = sorted(output_dir.glob("*.whl"))
    if not wheels:
        raise SystemExit("cibuildwheel completed but produced no wheels")

    for wheel in wheels:
        if "cp38-abi3" not in wheel.name:
            raise SystemExit(f"Unexpected wheel tag (expected cp38-abi3): {wheel.name}")

    print("Built wheels:\n" + "\n".join(f"  - {wheel.name}" for wheel in wheels))

    if output_dir_obj is not None:
        output_dir_obj.cleanup()
        cleaned = "(deleted temporary wheelhouse)"
    else:
        cleaned = "(kept in wheelhouse/)"
    print(f"Wheelhouse location: {output_dir} {cleaned}")
    return output_dir


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--container-engine",
        default=None,
        help=(
            "Container engine to use, e.g. 'podman' (default), 'docker', "
            "or a cibuildwheel container-engine config string"
        ),
    )
    parser.add_argument(
        "--identifier",
        default=None,
        help="Optional cibuildwheel identifier, e.g. cp312-manylinux_x86_64",
    )
    parser.add_argument(
        "--keep-wheels",
        action="store_true",
        help="Keep the wheelhouse directory instead of deleting it afterwards",
    )
    args = parser.parse_args()

    identifier = args.identifier or _default_identifier()
    run(
        identifier,
        keep_wheels=args.keep_wheels,
        container_engine=args.container_engine,
    )


if __name__ == "__main__":
    main()
