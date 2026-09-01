#!/usr/bin/env python3
"""Package the reusable OTA sample as one Arduino library, without local keys."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import tempfile
import zipfile

PROJECT_ROOT = Path(__file__).resolve().parents[1]
LIBRARY = "WioOtaAgent"
SKETCH_FILES = ("CellularStatusOta.ino", "ota_sketch_config.h",
                "ota_manifest_public_key.h.example")


def package_library(project: Path, output: Path) -> Path:
    output.mkdir(parents=True, exist_ok=True)
    files = [project / "library.properties", project / "LICENSE.txt"]
    files += sorted(path for path in (project / "src").rglob("*")
                    if path.is_file() and path.suffix in (".h", ".cpp", ".c", ".S"))
    sketch = project / "examples/CellularStatusOta"
    files += [sketch / filename for filename in SKETCH_FILES]
    for path in files:
        # Reject both file symlinks and symlinked source directories.
        candidate = project
        for component in path.relative_to(project).parts:
            candidate = candidate / component
            if candidate.is_symlink():
                raise ValueError(f"refusing to package symlink: {candidate}")
    archive = output / f"{LIBRARY}.zip"
    descriptor, temporary_name = tempfile.mkstemp(
        prefix=f".{archive.name}.", dir=output
    )
    os.close(descriptor)
    temporary_archive = Path(temporary_name)
    try:
        with zipfile.ZipFile(
            temporary_archive, "w", zipfile.ZIP_DEFLATED
        ) as package:
            for path in files:
                package.write(
                    path, f"{LIBRARY}/{path.relative_to(project).as_posix()}"
                )
        os.replace(temporary_archive, archive)
    finally:
        temporary_archive.unlink(missing_ok=True)
    return archive


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=PROJECT_ROOT / "dist/arduino")
    args = parser.parse_args()
    print(package_library(PROJECT_ROOT, args.output))


if __name__ == "__main__":
    main()
