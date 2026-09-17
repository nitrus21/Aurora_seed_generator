"""Read-only verification of both P4 images and their factory-image mappings."""
from pathlib import Path
import argparse
import hashlib
import struct

root = Path(__file__).resolve().parents[2]

def verify_image(data, expected):
    assert data[0] == 0xE9 and data[3] >> 4 == 5, "Not a 32MB ESP image"
    fields = struct.unpack_from("<BBBBHBHHBBBBB", data, 8)
    assert fields[4] == 18, "Not an ESP32-P4 image"
    assert tuple(fields[6:8]) == expected, "Wrong silicon revision constraints"
    assert fields[-1] == 1, "Missing validation digest"
    position, checksum = 24, 0xEF
    for _ in range(data[1]):
        _, length = struct.unpack_from("<II", data, position)
        position += 8
        assert position + length <= len(data)
        for byte in data[position:position + length]:
            checksum ^= byte
        position += length
    position = (position // 16) * 16 + 15
    assert data[position] == checksum, "Invalid image checksum"
    assert data[position + 1:position + 33] == hashlib.sha256(data[:position + 1]).digest(), "Invalid image SHA-256"
    return position + 33

profiles = {"waveshare-p4": (300, 399), "waveshare-p4-rev1": (100, 199)}
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--profile", choices=profiles)
parser.add_argument("--factory", type=Path,
                    help="Verify a published merged factory image instead of a local build directory")
parser.add_argument("--version", help="Expected application version for --factory")
arguments = parser.parse_args()
if arguments.factory:
    assert arguments.profile and arguments.version, "--factory requires --profile and --version"
    revisions = profiles[arguments.profile]
    factory = arguments.factory.read_bytes()
    verify_image(factory[0x2000:0x8000], revisions)
    assert factory[0x8000:0x8002] == b"\xaa\x50", "Missing partition table"
    application = factory[0x10000:]
    assert verify_image(application, revisions) == len(application), "Unexpected factory image tail"
    assert struct.unpack_from("<I", application, 0x20)[0] == 0xABCD5432, "Missing ESP-IDF app descriptor"
    actual_version = application[0x30:0x50].split(b"\0")[0].decode()
    assert actual_version == arguments.version, (actual_version, arguments.version)
    print(f"PASS: published {arguments.factory.name}, chip/32MB/revisions {revisions}, checksums, version and factory offsets")
    raise SystemExit(0)
for profile, revisions in profiles.items():
    if arguments.profile and profile != arguments.profile:
        continue
    folder = root / "targets/waveshare_p4/.pio/build" / profile
    factory = (folder / "firmware.factory.bin").read_bytes()
    for name, offset in (("bootloader.bin", 0x2000), ("partitions.bin", 0x8000), ("firmware.bin", 0x10000)):
        data = (folder / name).read_bytes()
        assert factory[offset:offset + len(data)] == data, f"{profile}: factory mismatch for {name}"
        if name != "partitions.bin":
            verify_image(data, revisions)
    assert len(factory) == 0x10000 + (folder / "firmware.bin").stat().st_size
    print(f"PASS: {profile}, chip/32MB/revisions {revisions}, checksums, SHA-256 and factory offsets")
