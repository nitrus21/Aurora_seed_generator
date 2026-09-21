"""Generate the deterministic CycloneDX inventory for AURORA P4.

The generator intentionally uses only Python's standard library and inputs
tracked by Git.  It does not inspect a developer's PlatformIO cache: a clean CI
checkout must produce the same document byte for byte.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys
import uuid


DEFAULT_OUTPUT = Path("tmp/sbom/aurora-p4-2.0.12.cdx.json")
INPUTS = (
    Path("include/version.h"),
    Path("targets/waveshare_p4/dependencies.lock"),
    Path("targets/waveshare_p4/main/idf_component.yml"),
    Path("targets/waveshare_p4/platformio.ini"),
    Path("targets/waveshare_p4/build.ps1"),
    Path("targets/waveshare_p4/tools/image_revision.py"),
    Path("targets/waveshare_p4/main/CMakeLists.txt"),
    Path("targets/waveshare_p4/sdkconfig.defaults"),
    Path("targets/waveshare_p4/sdkconfig.rev1.defaults"),
    Path("targets/waveshare_p4/cmake/crypto_hardening.cmake"),
    Path("tools/patch_ubitcoin.py"),
    Path("tools/patch_ubitcoin_p4.py"),
    Path("tools/patch_ubitcoin_rng.py"),
    Path("tools/prepare_p4_crypto_overlay.py"),
    Path("tools/generate_p4_sbom.py"),
    Path(".github/workflows/reproducible-p4-builds.yml"),
)


def sha256(path: Path) -> str:
    # Git may materialize text as CRLF or LF depending on the checkout.  The
    # SBOM describes source content, so hash the canonical UTF-8/LF form.
    text = path.read_text(encoding="utf-8")
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def scalar(value: str) -> str:
    value = value.strip()
    if len(value) >= 2 and value[0] == value[-1] and value[0] in "\"'":
        return value[1:-1]
    return value


def parse_lock(path: Path) -> tuple[dict[str, dict[str, object]], list[str]]:
    """Parse only the stable fields used from ESP-IDF's dependency lock."""
    components: dict[str, dict[str, object]] = {}
    direct: list[str] = []
    current: str | None = None
    in_components = False
    in_direct = False
    in_child_dependencies = False

    for line in path.read_text(encoding="utf-8").splitlines():
        if line == "dependencies:":
            in_components = True
            in_direct = False
            continue
        if line == "direct_dependencies:":
            in_components = False
            in_direct = True
            current = None
            continue
        if line and not line.startswith(" ") and line.endswith(":"):
            in_components = False
            in_direct = False
            current = None

        if in_direct:
            match = re.fullmatch(r"- (.+)", line)
            if match:
                direct.append(scalar(match.group(1)))
            continue
        if not in_components:
            continue

        match = re.fullmatch(r"  (\S[^:]*):", line)
        if match:
            current = match.group(1)
            components[current] = {"dependencies": []}
            in_child_dependencies = False
            continue
        if current is None:
            continue
        if line == "    dependencies:":
            in_child_dependencies = True
            continue
        match = re.fullmatch(r"    (version|component_hash): (.+)", line)
        if match:
            components[current][match.group(1)] = scalar(match.group(2))
            continue
        if in_child_dependencies:
            match = re.fullmatch(r"    - name: (.+)", line)
            if match:
                components[current]["dependencies"].append(scalar(match.group(1)))

    if len(components) != 24:
        raise ValueError(f"expected 24 resolved P4 components, found {len(components)}")
    if len([item for item in components.values() if "component_hash" in item]) != 23:
        raise ValueError("expected hashes for the 23 registry components")
    if not direct or any(name not in components for name in direct):
        raise ValueError("invalid direct dependency list")
    for name, item in components.items():
        if "version" not in item:
            raise ValueError(f"missing resolved version for {name}")
    return components, direct


def extract(pattern: str, path: Path, description: str) -> str:
    match = re.search(pattern, path.read_text(encoding="utf-8"), re.MULTILINE)
    if not match:
        raise ValueError(f"cannot find {description} in {path}")
    return match.group(1)


def generic_ref(name: str, version: str) -> str:
    return f"pkg:generic/{name}@{version}"


def component(name: str, version: str, ref: str, *, group: str = "",
              kind: str = "library", digest: str | None = None,
              properties: list[dict[str, str]] | None = None,
              source_url: str | None = None) -> dict[str, object]:
    result: dict[str, object] = {
        "type": kind,
        "bom-ref": ref,
        "name": name,
        "version": version,
    }
    if group:
        result["group"] = group
    if digest:
        result["hashes"] = [{"alg": "SHA-256", "content": digest}]
    if properties:
        result["properties"] = properties
    if source_url:
        result["externalReferences"] = [{"type": "vcs", "url": source_url}]
    return result


def generate(repo: Path) -> dict[str, object]:
    paths = {str(relative).replace("\\", "/"): repo / relative for relative in INPUTS}
    for relative, path in paths.items():
        if not path.is_file():
            raise FileNotFoundError(f"missing SBOM input: {relative}")

    version_path = repo / "include/version.h"
    lock_path = repo / "targets/waveshare_p4/dependencies.lock"
    pio_path = repo / "targets/waveshare_p4/platformio.ini"
    cmake_path = repo / "targets/waveshare_p4/main/CMakeLists.txt"
    overlay_path = repo / "tools/prepare_p4_crypto_overlay.py"
    repro_workflow = repo / ".github/workflows/reproducible-p4-builds.yml"

    aurora_version = extract(
        r'^#define AURORA_P4_FIRMWARE_VERSION "([0-9]+\.[0-9]+\.[0-9]+)"$',
        version_path, "P4 firmware version")
    platform_version = extract(
        r"platform-espressif32/releases/download/([^/]+)/platform-espressif32\.zip",
        pio_path, "pioarduino platform version")
    platformio_core = extract(r"platformio==([0-9.]+)", repro_workflow,
                              "PlatformIO Core version")
    python_runtime = extract(r'python-version: "([0-9.]+)"', repro_workflow,
                             "CI Python version")
    ubitcoin_commit = extract(r"GIT_TAG ([0-9a-f]{40})", cmake_path, "uBitcoin commit")
    overlay_versions = re.search(
        r"ESP-IDF ([0-9.]+) / Mbed TLS ([0-9.]+) inputs are whole-file pinned",
        overlay_path.read_text(encoding="utf-8"))
    if not overlay_versions:
        raise ValueError("cannot find pinned ESP-IDF/Mbed TLS overlay versions")
    overlay_idf_version, mbedtls_version = overlay_versions.groups()

    locked, direct = parse_lock(lock_path)
    idf_version = str(locked["idf"]["version"])
    if idf_version != overlay_idf_version:
        raise ValueError("ESP-IDF lock and crypto overlay versions disagree")

    input_hashes = {name: sha256(path) for name, path in sorted(paths.items())}
    identity = "\n".join(f"{name}:{digest}" for name, digest in input_hashes.items())
    serial = uuid.uuid5(uuid.NAMESPACE_URL, f"aurora-p4:{aurora_version}\n{identity}")
    root_ref = f"pkg:generic/nitrus21/aurora@{aurora_version}?target=esp32p4"
    idf_ref = f"pkg:github/espressif/esp-idf@v{idf_version}"
    platform_ref = f"pkg:github/pioarduino/platform-espressif32@{platform_version}"
    ubitcoin_ref = f"pkg:github/micro-bitcoin/uBitcoin@{ubitcoin_commit}"
    mbedtls_ref = f"pkg:github/Mbed-TLS/mbedtls@mbedtls-{mbedtls_version}"

    refs: dict[str, str] = {}
    bom_components: list[dict[str, object]] = []
    for locked_name in sorted(locked):
        item = locked[locked_name]
        version = str(item["version"])
        group, _, name = locked_name.partition("/")
        if locked_name == "idf":
            refs[locked_name] = idf_ref
            bom_components.append(component(
                "esp-idf", version, idf_ref, group="espressif", kind="framework",
                source_url=f"https://github.com/espressif/esp-idf/tree/v{version}"))
        else:
            ref = generic_ref(locked_name, version)
            refs[locked_name] = ref
            bom_components.append(component(
                name, version, ref, group=group, digest=str(item["component_hash"]),
                properties=[{"name": "aurora:source", "value": "ESP-IDF component registry"}],
                source_url=f"https://components.espressif.com/components/{locked_name}"))

    patch_properties = []
    for name in ("tools/patch_ubitcoin.py", "tools/patch_ubitcoin_p4.py",
                 "tools/patch_ubitcoin_rng.py"):
        patch_properties.append({"name": f"aurora:patch-sha256:{name}",
                                 "value": input_hashes[name]})
    bom_components.extend([
        component("platform-espressif32", platform_version, platform_ref,
                  group="pioarduino", kind="framework",
                  properties=[{"name": "aurora:role", "value": "build platform"}],
                  source_url=("https://github.com/pioarduino/platform-espressif32/"
                              f"releases/tag/{platform_version}")),
        component("uBitcoin", ubitcoin_commit, ubitcoin_ref, group="micro-bitcoin",
                  properties=patch_properties,
                  source_url=f"https://github.com/micro-bitcoin/uBitcoin/tree/{ubitcoin_commit}"),
        component("mbedtls", mbedtls_version, mbedtls_ref, group="Mbed-TLS",
                  properties=[
                      {"name": "aurora:source", "value": "embedded in ESP-IDF"},
                      {"name": "aurora:overlay-sha256",
                       "value": input_hashes["tools/prepare_p4_crypto_overlay.py"]},
                  ],
                  source_url=f"https://github.com/Mbed-TLS/mbedtls/tree/mbedtls-{mbedtls_version}"),
    ])
    bom_components.sort(key=lambda item: str(item["bom-ref"]))

    dependency_map: dict[str, set[str]] = {str(item["bom-ref"]): set()
                                           for item in bom_components}
    root_dependencies = {refs[name] for name in direct}
    root_dependencies.update((platform_ref, ubitcoin_ref))
    for locked_name, item in locked.items():
        parent_ref = refs[locked_name]
        for child in item["dependencies"]:
            if child in refs:
                dependency_map[parent_ref].add(refs[child])
    dependency_map[idf_ref].add(mbedtls_ref)
    dependency_map[platform_ref].add(idf_ref)

    metadata_properties = [
        {"name": "aurora:profiles", "value": "waveshare-p4,waveshare-p4-rev1"},
        {"name": "aurora:scope", "value": "P4 firmware and build inputs; Web Flasher excluded"},
        {"name": "aurora:build-tool:platformio-core", "value": platformio_core},
        {"name": "aurora:build-tool:python", "value": python_runtime},
    ]
    metadata_properties.extend(
        {"name": f"aurora:input-sha256:{name}", "value": digest}
        for name, digest in input_hashes.items())

    dependencies = [{"ref": root_ref, "dependsOn": sorted(root_dependencies)}]
    dependencies.extend(
        {"ref": ref, "dependsOn": sorted(children)}
        for ref, children in sorted(dependency_map.items()))

    return {
        "bomFormat": "CycloneDX",
        "specVersion": "1.6",
        "serialNumber": f"urn:uuid:{serial}",
        "version": 1,
        "metadata": {
            "component": {
                "type": "firmware",
                "bom-ref": root_ref,
                "group": "nitrus21",
                "name": "AURORA P4",
                "version": aurora_version,
            },
            "properties": metadata_properties,
        },
        "components": bom_components,
        "dependencies": dependencies,
    }


def serialized(document: dict[str, object]) -> str:
    return json.dumps(document, indent=2, ensure_ascii=False) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path,
                        default=Path(__file__).resolve().parents[1])
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--check", action="store_true",
                        help="fail if the committed SBOM differs from current inputs")
    args = parser.parse_args()

    repo = args.repo_root.resolve()
    output = args.output if args.output.is_absolute() else repo / args.output
    content = serialized(generate(repo))
    if args.check:
        if not output.is_file() or output.read_text(encoding="utf-8") != content:
            print(f"P4 SBOM is missing or stale: {output}", file=sys.stderr)
            return 1
        print(f"PASS: deterministic P4 SBOM is current ({output})")
        return 0
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(content, encoding="utf-8", newline="\n")
    print(f"Generated {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
