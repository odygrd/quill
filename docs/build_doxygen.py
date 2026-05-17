#!/usr/bin/env python3
from __future__ import annotations

import os
import shutil
import subprocess
import tarfile
import urllib.request
from pathlib import Path

DOXYGEN_VERSION = "1.16.0"
DOXYGEN_ARCHIVE_NAME = f"doxygen-{DOXYGEN_VERSION}.linux.bin.tar.gz"
DOXYGEN_ARCHIVE_URL = f"https://www.doxygen.nl/files/{DOXYGEN_ARCHIVE_NAME}"

REPO_ROOT = Path(__file__).resolve().parents[1]
DOCS_DIR = REPO_ROOT / "docs"
TOOLS_DIR = DOCS_DIR / ".tools"
CONFIG_TEMPLATE = DOCS_DIR / "Doxyfile.in"
CONFIG_PATH = DOCS_DIR / "Doxyfile.rtd"
OUTPUT_DIR = DOCS_DIR / "build"
INPUT_DIR = REPO_ROOT / "include"


def _download_and_extract_doxygen() -> Path:
    TOOLS_DIR.mkdir(parents=True, exist_ok=True)

    archive_path = TOOLS_DIR / DOXYGEN_ARCHIVE_NAME
    extracted_dir = TOOLS_DIR / f"doxygen-{DOXYGEN_VERSION}"
    doxygen_bin = extracted_dir / "bin" / "doxygen"

    if doxygen_bin.exists():
        return doxygen_bin

    if not archive_path.exists():
        print(f"Downloading Doxygen {DOXYGEN_VERSION} from {DOXYGEN_ARCHIVE_URL}")
        urllib.request.urlretrieve(DOXYGEN_ARCHIVE_URL, archive_path)

    if extracted_dir.exists():
        shutil.rmtree(extracted_dir)

    with tarfile.open(archive_path, "r:gz") as archive:
        archive.extractall(TOOLS_DIR, filter="data")

    if not doxygen_bin.exists():
        raise FileNotFoundError(f"Expected Doxygen binary at {doxygen_bin}")

    return doxygen_bin


def _resolve_doxygen() -> Path:
    configured_doxygen = os.environ.get("DOXYGEN_EXECUTABLE")
    if configured_doxygen:
        doxygen_bin = Path(configured_doxygen).expanduser().resolve()
        if not doxygen_bin.exists():
            raise FileNotFoundError(f"DOXYGEN_EXECUTABLE does not exist: {doxygen_bin}")
        return doxygen_bin

    return _download_and_extract_doxygen()


def _write_config() -> None:
    config = CONFIG_TEMPLATE.read_text(encoding="utf-8")
    config = config.replace("@DOXYGEN_OUTPUT_DIR@", str(OUTPUT_DIR.resolve()))
    config = config.replace("@DOXYGEN_INPUT_DIR@", str(INPUT_DIR.resolve()))
    CONFIG_PATH.write_text(config, encoding="utf-8")


def main() -> None:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    doxygen_bin = _resolve_doxygen()
    _write_config()

    subprocess.run(
        [str(doxygen_bin), str(CONFIG_PATH)],
        check=True,
        cwd=REPO_ROOT,
    )


if __name__ == "__main__":
    main()
