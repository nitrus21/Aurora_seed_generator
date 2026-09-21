#!/usr/bin/env python3
"""Prepare the private, deterministic AURORA P4 2.0.11 release candidate.

This tool never writes to ``webflasher`` and never publishes anything.  The
expected digests pin the two production builds that completed the local test
campaign; a rebuild with different bytes must be reviewed as a new candidate.
"""

from __future__ import annotations

import hashlib
from pathlib import Path
import shutil
import zipfile


ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / "targets" / "waveshare_p4"
OUT = ROOT / "tmp" / "release-candidates" / "2.0.11"
VERSION = "2.0.11"
ZIP_TIMESTAMP = (2026, 9, 21, 0, 0, 0)

IMAGES = {
    "aurora-2.0.11-esp32-p4-rev1.factory.bin": {
        "source": PROJECT / ".pio" / "build" / "waveshare-p4-rev1" / "firmware.factory.bin",
        "sha256": "962A04E1FF65D0CE38FD13E3700F74E15852315999C031E972761497D3E0994E",
        "silicon": "ESP32-P4 revision 1.x (100-199)",
        "status": "software and physical validation on P4 rev1.3",
    },
    "aurora-2.0.11-esp32-p4-rev3.factory.bin": {
        "source": PROJECT / ".pio" / "build" / "waveshare-p4" / "firmware.factory.bin",
        "sha256": "7B6F2C69988862233341413677AD866131AABEC82F319A7DBF3F891962F2B0A7",
        "silicon": "ESP32-P4 revision 3.x (300-399)",
        "status": "software validation only; no rev3 device was available",
    },
}


def digest(path: Path) -> str:
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest().upper()


def write_zip(path: Path, entries: list[tuple[str, bytes]]) -> None:
    with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED) as bundle:
        for name, data in entries:
            item = zipfile.ZipInfo(name, ZIP_TIMESTAMP)
            item.compress_type = zipfile.ZIP_DEFLATED
            item.external_attr = 0o644 << 16
            bundle.writestr(item, data)


def main() -> int:
    OUT.mkdir(parents=True, exist_ok=True)
    copied: list[Path] = []
    for name, metadata in IMAGES.items():
        source = metadata["source"]
        if not source.is_file():
            raise RuntimeError(f"missing production image: {source}")
        actual = digest(source)
        if actual != metadata["sha256"]:
            raise RuntimeError(
                f"candidate drift for {name}: expected {metadata['sha256']}, got {actual}"
            )
        target = OUT / name
        shutil.copyfile(source, target)
        copied.append(target)

    internal_sums = "".join(f"{digest(path)}  {path.name}\n" for path in copied)
    notes = [
        f"AURORA P4 {VERSION} - private release candidate",
        "",
        "This directory is not a publication and is not served by the Web Flasher.",
        "Select only the image matching the silicon revision shown by the device.",
        "",
    ]
    for name, metadata in IMAGES.items():
        notes.extend((
            name,
            f"  Target: {metadata['silicon']}",
            f"  Validation: {metadata['status']}",
            f"  SHA-256: {metadata['sha256']}",
            "",
        ))
    notes.extend((
        "The firmware is offline by design and creates no application log file on the P4.",
        "The rev3 image must not be presented as physically qualified.",
        "Verify SHA256SUMS-v2.0.11.txt and its Ed25519 signature before publication.",
        "",
    ))
    readme = "\n".join(notes).encode("utf-8")

    archive = OUT / "AURORA-v2.0.11-P4.zip"
    zip_entries = [(path.name, path.read_bytes()) for path in copied]
    zip_entries.extend((
        ("SHA256SUMS.txt", internal_sums.encode("ascii")),
        ("README.txt", readme),
    ))
    write_zip(archive, zip_entries)

    manifest = OUT / "SHA256SUMS-v2.0.11.txt"
    manifest.write_text(
        internal_sums + f"{digest(archive)}  {archive.name}\n",
        encoding="ascii",
        newline="\n",
    )
    (OUT / "README.txt").write_bytes(readme)

    print(f"PRIVATE CANDIDATE: {OUT}")
    for path in (*copied, archive, manifest):
        print(f"{digest(path)}  {path.name} ({path.stat().st_size} bytes)")
    print("NOT SIGNED, NOT PUBLISHED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
