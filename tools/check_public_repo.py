"""Fail when the public AURORA tree contains private or non-release material."""

from __future__ import annotations

from pathlib import Path, PurePosixPath
import hashlib
import re
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
FIRMWARE = PurePosixPath("webflasher/firmware")
FORBIDDEN_DIRS = {
    ".agents",
    ".codex",
    ".pio",
    ".vscode",
    "managed_components",
    "private",
    "sources",
    "tmp",
}
FORBIDDEN_EXACT = {"AGENTS.md"}
FORBIDDEN_SUFFIXES = {".aurora", ".elf", ".map", ".o", ".obj"}
WITHDRAWN_RELEASES = {
    "1.7.6",
    "1.9.2",
    "1.9.3",
    "1.9.4",
    "1.9.5",
    "1.9.6",
    "1.9.7",
    "1.9.8",
    "2.0.0",
    "2.0.1",
    "2.0.2",
}

# These two frozen CYD images predate the public-tree policy and contain a
# compiler source path. They remain byte-for-byte identical to the published
# files; no future binary receives this exception.
LEGACY_BINARY_EXCEPTIONS = {
    "webflasher/firmware/aurora-1.7.5-esp32-2432s028r.factory.bin":
        "469a8912cd2a7ba3ef919467d84d6857cf60e726a27865157bd1a04653510218",
    "webflasher/firmware/aurora-1.9.9-esp32-2432s028r.factory.bin":
        "34a7928985aa88d0e79f2d48eb5143a839e69106b2322db78d2404eab03dad1e",
}
CONTENT_SCAN_EXCEPTIONS = {"tools/check_public_repo.py"}

LOCAL_PATH = re.compile(
    rb"(?:[A-Za-z]:[\\/]+Users[\\/]+[^\\/\x00\r\n]+|/home/[^/\x00\r\n]+|/Users/[^/\x00\r\n]+)",
    re.IGNORECASE,
)
PERSONAL_EMAIL = re.compile(rb"[A-Z0-9._%+-]+@(?:gmail|outlook|hotmail|proton)\.[A-Z]{2,}", re.IGNORECASE)


def tracked_files() -> list[str]:
    result = subprocess.run(
        ["git", "ls-files", "-z"], cwd=ROOT, check=True, capture_output=True
    )
    return [item.decode("utf-8") for item in result.stdout.split(b"\0") if item]


def main() -> int:
    errors: list[str] = []
    paths = tracked_files()
    for name in paths:
        path = PurePosixPath(name)
        file = ROOT / name
        # A local cleanup may remove an indexed file before the commit. The
        # committed tree checked in CI never has this transient state.
        if not file.exists():
            continue
        lower = name.lower()
        parts = set(path.parts)
        basename = path.name.lower()

        if name in FORBIDDEN_EXACT or parts & FORBIDDEN_DIRS:
            errors.append(f"private/generated path is tracked: {name}")
        if basename in {"security.md", "validation.md"} or (
            basename.startswith(("audit-", "audit_"))
            and path.suffix.lower() in {".md", ".txt", ".json", ".pdf", ".log"}
        ):
            errors.append(f"private report is tracked: {name}")
        if path.suffix.lower() in FORBIDDEN_SUFFIXES:
            errors.append(f"secret or build output is tracked: {name}")
        if basename in {"sdkconfig", "sdkconfig.old"}:
            errors.append(f"generated SDK configuration is tracked: {name}")

        if path.parent == FIRMWARE:
            if basename != "sha256sums.txt" and not basename.endswith(".factory.bin"):
                errors.append(f"raw Web Flasher artifact is tracked: {name}")
            if any(version in lower for version in WITHDRAWN_RELEASES):
                errors.append(f"withdrawn firmware is tracked: {name}")

        data = file.read_bytes()
        if name in CONTENT_SCAN_EXCEPTIONS:
            continue
        if name in LEGACY_BINARY_EXCEPTIONS:
            actual = hashlib.sha256(data).hexdigest()
            if actual != LEGACY_BINARY_EXCEPTIONS[name]:
                errors.append(f"frozen legacy image changed: {name}")
            continue
        if LOCAL_PATH.search(data):
            errors.append(f"local account path found: {name}")
        if PERSONAL_EMAIL.search(data):
            errors.append(f"personal email address found: {name}")

    if errors:
        for error in sorted(set(errors)):
            print(f"ERROR: {error}")
        return 1
    print(f"PASS: {len(paths)} tracked files satisfy the public repository policy")
    return 0


if __name__ == "__main__":
    sys.exit(main())
