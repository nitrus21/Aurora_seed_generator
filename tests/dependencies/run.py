"""Host-only positive and negative checks for the P4 dependency SBOM."""

import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


root = Path(__file__).resolve().parents[2]
tool = root / "tools/generate_p4_sbom.py"
sbom = root / "tmp/sbom/aurora-p4-2.0.12.cdx.json"


def run(*arguments, expected=0):
    result = subprocess.run([sys.executable, str(tool), *map(str, arguments)],
                            capture_output=True, text=True)
    if result.returncode != expected:
        raise RuntimeError(result.stdout + result.stderr)
    return result


run("--output", sbom)
run("--check", "--output", sbom)
document = json.loads(sbom.read_text(encoding="utf-8"))
if document["bomFormat"] != "CycloneDX" or document["specVersion"] != "1.6":
    raise RuntimeError("unexpected SBOM format")
if document["metadata"]["component"]["version"] != "2.0.12":
    raise RuntimeError("SBOM firmware version is stale")
components = document["components"]
if len(components) != 27:
    raise RuntimeError(f"expected 24 locked plus 3 embedded/build components, got {len(components)}")
if len({item["bom-ref"] for item in components}) != len(components):
    raise RuntimeError("duplicate component references")
if len([item for item in components if item.get("hashes")]) != 23:
    raise RuntimeError("all 23 registry components must retain their lock hashes")
print("PASS: P4 CycloneDX inventory covers locked, embedded and build components")

with tempfile.TemporaryDirectory() as directory:
    stale = Path(directory) / "stale.cdx.json"
    stale.write_text(sbom.read_text(encoding="utf-8").replace('"version": "9.5.0"',
                                                               '"version": "9.5.1"', 1),
                     encoding="utf-8")
    result = run("--check", "--output", stale, expected=1)
    if "missing or stale" not in result.stderr:
        raise RuntimeError("stale SBOM failure did not explain the drift")
    print("PASS: stale SBOM is rejected")

    copied = Path(directory) / "repo"
    for relative in (
        "include/version.h",
        "targets/waveshare_p4/dependencies.lock",
        "targets/waveshare_p4/main/idf_component.yml",
        "targets/waveshare_p4/platformio.ini",
        "targets/waveshare_p4/build.ps1",
        "targets/waveshare_p4/tools/image_revision.py",
        "targets/waveshare_p4/main/CMakeLists.txt",
        "targets/waveshare_p4/sdkconfig.defaults",
        "targets/waveshare_p4/sdkconfig.rev1.defaults",
        "targets/waveshare_p4/cmake/crypto_hardening.cmake",
        "tools/patch_ubitcoin.py",
        "tools/patch_ubitcoin_p4.py",
        "tools/patch_ubitcoin_rng.py",
        "tools/prepare_p4_crypto_overlay.py",
        "tools/generate_p4_sbom.py",
        ".github/workflows/reproducible-p4-builds.yml",
    ):
        source = root / relative
        target = copied / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
    lock = copied / "targets/waveshare_p4/dependencies.lock"
    lock.write_text(lock.read_text(encoding="utf-8").replace("version: 9.5.0", "version: 9.5.1", 1),
                    encoding="utf-8")
    changed = Path(directory) / "changed.cdx.json"
    run("--repo-root", copied, "--output", changed)
    if changed.read_bytes() == sbom.read_bytes():
        raise RuntimeError("dependency lock drift did not change the SBOM")
    print("PASS: dependency lock drift changes the SBOM and requires review")

    lock.write_text(lock.read_text(encoding="utf-8").replace("\n", "\r\n"),
                    encoding="utf-8", newline="")
    crlf = Path(directory) / "crlf.cdx.json"
    run("--repo-root", copied, "--output", crlf)
    if crlf.read_bytes() != changed.read_bytes():
        raise RuntimeError("CRLF checkout changed the canonical SBOM")
    print("PASS: CRLF and LF checkouts produce the same SBOM")
