"""Host-only positive and negative tests for the reproducibility comparator."""

from copy import deepcopy
import json
from pathlib import Path
import subprocess
import sys
import tempfile


root = Path(__file__).resolve().parents[2]
tool = root / "tools" / "reproducible_build.py"
image_revision = root / "targets" / "waveshare_p4" / "tools" / "image_revision.py"
reproducible_link = root / "targets" / "waveshare_p4" / "tools" / "reproducible_link.py"
reproducible_elf = root / "targets" / "waveshare_p4" / "tools" / "reproducible_elf.py"
attributes = root / ".gitattributes"

image_revision_text = image_revision.read_text(encoding="utf-8")
if 'env.subst("$PYTHONEXE")' not in image_revision_text or "sys.executable" in image_revision_text:
    raise RuntimeError("image regeneration must use PlatformIO's tool Python")
print("PASS: image regeneration uses PlatformIO tool Python")

link_text = reproducible_link.read_text(encoding="utf-8")
if "sorted(libraries, key=stable_library_key)" not in link_text:
    raise RuntimeError("application libraries must use a stable link order")
print("PASS: application libraries use a stable link order")

attribute_text = attributes.read_text(encoding="utf-8")
for pattern in ("*.c text eol=lf", "*.cpp text eol=lf", "*.h text eol=lf",
                "*.cmake text eol=lf", "*.ini text eol=lf", "*.defaults text eol=lf",
                "*.csv text eol=lf"):
    if pattern not in attribute_text:
        raise RuntimeError(f"missing reproducible line-ending policy: {pattern}")
print("PASS: firmware inputs use host-independent LF line endings")

elf_text = reproducible_elf.read_text(encoding="utf-8")
if 'subprocess.run([str(objcopy), "--strip-debug", str(elf)]' not in elf_text:
    raise RuntimeError("reproducible builds must normalize host-specific DWARF sections")
print("PASS: reproducible ELF hash excludes host-specific DWARF sections")


def manifest(profile, runner):
    return {
        "schema": 1,
        "source_revision": "a" * 40,
        "profile": profile,
        "runner": runner,
        "python_runtime": "3.11.9",
        "platformio_core": "PlatformIO Core, version 6.1.19",
        "inputs": {"dependencies.lock": "b" * 64},
        "artifacts": {
            "factory": {"filename": "firmware.factory.bin", "bytes": 3, "sha256": "c" * 64}
        },
    }


with tempfile.TemporaryDirectory() as directory:
    temporary = Path(directory)
    paths = []
    for profile in ("waveshare-p4-rev1", "waveshare-p4"):
        for runner in ("windows-latest", "ubuntu-latest"):
            path = temporary / f"reproducibility-{profile}-{runner}.json"
            path.write_text(json.dumps(manifest(profile, runner)), encoding="utf-8")
            paths.append(path)

    command = [sys.executable, str(tool), "compare-tree", "--root", str(temporary),
               "--expected-runner", "windows-latest", "--expected-runner", "ubuntu-latest"]
    subprocess.run(command, check=True)
    print("PASS: identical manifests accepted")

    altered = deepcopy(manifest("waveshare-p4", "ubuntu-latest"))
    altered["artifacts"]["factory"]["sha256"] = "d" * 64
    target = temporary / "reproducibility-waveshare-p4-ubuntu-latest.json"
    target.write_text(json.dumps(altered), encoding="utf-8")
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode == 0 or "reproducibility mismatch" not in result.stderr:
        raise RuntimeError("altered artifact manifest was not rejected")
    print("PASS: altered artifact manifest rejected")

    target.write_text(json.dumps(manifest("waveshare-p4", "ubuntu-latest")), encoding="utf-8")
    (temporary / "reproducibility-waveshare-p4-rev1-ubuntu-latest.json").unlink()
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode == 0 or "missing independent runners" not in result.stderr:
        raise RuntimeError("missing independent runner was not rejected")
    print("PASS: missing independent runner rejected")
