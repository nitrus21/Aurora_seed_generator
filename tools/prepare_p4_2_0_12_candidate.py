#!/usr/bin/env python3
"""Prepare the private, deterministic AURORA P4 2.0.12 release candidate.

Only artifacts downloaded from the independent Windows/Linux CI matrix are
accepted.  The two runners must be byte-for-byte reproducible, and their
digests must match the exact candidate qualified on the P4 rev1.3.  This tool
never writes to ``webflasher`` and never publishes anything.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import zipfile

from reproducible_build import compare_manifests


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "tmp" / "release-candidates" / "2.0.12"
VERSION = "2.0.12"
SOURCE_REVISION = "c670ecbba43aa46f728adbd2f1c774fe76dc4b83"
ZIP_TIMESTAMP = (2026, 9, 21, 0, 0, 0)

PROFILES = {
    "waveshare-p4-rev1": {
        "release_name": "aurora-2.0.12-esp32-p4-rev1.factory.bin",
        "application_sha256": "3CC0D7B887C8B14E18528A8F5CA3900086AE7AAA0AC68097016D2A40E004E5B8",
        "factory_sha256": "5C27560D7E021CA38BA9C8FABB59EF217E5E2536F22AF5AB9B4171ABBDB6F90A",
        "silicon": "ESP32-P4 revision 1.x (100-199)",
        "status": "software and physical validation on P4 rev1.3",
    },
    "waveshare-p4": {
        "release_name": "aurora-2.0.12-esp32-p4-rev3.factory.bin",
        "application_sha256": "58E80A40B9D550A30A092A48BBC22AEE9F5909C6FF0D063B14D47F2F944981AC",
        "factory_sha256": "0CD58BA0C18A50A7073BEAB871154AFE622EEB8206F27C7BE5571DC57732F98E",
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


def locate_manifests(ci_root: Path, profile: str) -> dict[str, tuple[Path, dict]]:
    found: dict[str, tuple[Path, dict]] = {}
    for path in ci_root.rglob(f"reproducibility-{profile}-*.json"):
        document = json.loads(path.read_text(encoding="utf-8"))
        if document.get("profile") != profile:
            continue
        runner = document.get("runner")
        if runner in found:
            raise RuntimeError(f"duplicate CI manifest for {profile}/{runner}")
        found[runner] = (path, document)
    expected = {"ubuntu-latest", "windows-latest"}
    if set(found) != expected:
        raise RuntimeError(
            f"incomplete CI evidence for {profile}: got {sorted(found)}, "
            f"expected {sorted(expected)}"
        )
    return found


def artifact_path(manifest_path: Path, document: dict, logical_name: str) -> Path:
    artifact = document["artifacts"][logical_name]
    path = (
        manifest_path.parent
        / document["profile"]
        / document["runner"]
        / artifact["filename"]
    )
    if not path.is_file():
        raise RuntimeError(f"missing CI artifact: {path}")
    actual = digest(path)
    expected = artifact["sha256"].upper()
    if actual != expected:
        raise RuntimeError(f"CI artifact differs from its manifest: {path}")
    return path


def prepare(ci_root: Path, output: Path) -> None:
    selected: list[tuple[Path, dict]] = []
    for profile, expected in PROFILES.items():
        manifests = locate_manifests(ci_root, profile)
        compare_manifests(
            [entry[0] for entry in manifests.values()],
            {"ubuntu-latest", "windows-latest"},
        )
        for manifest_path, document in manifests.values():
            if document.get("source_revision") != SOURCE_REVISION:
                raise RuntimeError(
                    f"unexpected source revision in {manifest_path}: "
                    f"{document.get('source_revision')}"
                )
            app = document["artifacts"]["application"]["sha256"].upper()
            factory = document["artifacts"]["factory"]["sha256"].upper()
            if app != expected["application_sha256"]:
                raise RuntimeError(f"qualified application drift for {profile}: {app}")
            if factory != expected["factory_sha256"]:
                raise RuntimeError(f"qualified factory drift for {profile}: {factory}")
            artifact_path(manifest_path, document, "application")
            artifact_path(manifest_path, document, "factory")
        selected.append(manifests["ubuntu-latest"])

    output.mkdir(parents=True, exist_ok=True)
    copied: list[Path] = []
    for manifest_path, document in selected:
        metadata = PROFILES[document["profile"]]
        source = artifact_path(manifest_path, document, "factory")
        target = output / metadata["release_name"]
        shutil.copyfile(source, target)
        copied.append(target)

    internal_sums = "".join(f"{digest(path)}  {path.name}\n" for path in copied)
    notes = [
        f"AURORA P4 {VERSION} - private release candidate",
        "",
        f"Canonical CI source revision: {SOURCE_REVISION}",
        "This directory is not a publication and is not served by the Web Flasher.",
        "Select only the image matching the silicon revision shown by the device.",
        "",
    ]
    for profile, metadata in PROFILES.items():
        notes.extend((
            metadata["release_name"],
            f"  Profile: {profile}",
            f"  Target: {metadata['silicon']}",
            f"  Validation: {metadata['status']}",
            f"  Application SHA-256: {metadata['application_sha256']}",
            f"  Factory SHA-256: {metadata['factory_sha256']}",
            "",
        ))
    notes.extend((
        "The firmware is offline by design and creates no application log file on the P4.",
        "The rev3 image must not be presented as physically qualified.",
        "Verify SHA256SUMS-v2.0.12.txt and its Ed25519 signature before publication.",
        "",
    ))
    readme = "\n".join(notes).encode("utf-8")

    archive = output / "AURORA-v2.0.12-P4.zip"
    zip_entries = [(path.name, path.read_bytes()) for path in copied]
    zip_entries.extend((
        ("SHA256SUMS.txt", internal_sums.encode("ascii")),
        ("README.txt", readme),
    ))
    write_zip(archive, zip_entries)

    manifest = output / "SHA256SUMS-v2.0.12.txt"
    manifest.write_text(
        internal_sums + f"{digest(archive)}  {archive.name}\n",
        encoding="ascii",
        newline="\n",
    )
    (output / "README.txt").write_bytes(readme)

    print(f"PRIVATE CANDIDATE: {output}")
    for path in (*copied, archive, manifest):
        print(f"{digest(path)}  {path.name} ({path.stat().st_size} bytes)")
    print("NOT SIGNED, NOT PUBLISHED")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ci-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, default=OUT)
    args = parser.parse_args()
    prepare(args.ci_root.resolve(), args.output.resolve())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
