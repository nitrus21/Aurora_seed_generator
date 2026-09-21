#!/usr/bin/env python3
"""Collect and compare AURORA P4 release-image reproducibility evidence."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys


SCHEMA = 1
ARTIFACTS = {
    "bootloader": "bootloader.bin",
    "partitions": "partitions.bin",
    "application": "firmware.bin",
    "factory": "firmware.factory.bin",
}
INPUTS = (
    "include/version.h",
    "targets/waveshare_p4/platformio.ini",
    "targets/waveshare_p4/dependencies.lock",
    "targets/waveshare_p4/sdkconfig.defaults",
    "targets/waveshare_p4/sdkconfig.rev1.defaults",
    "targets/waveshare_p4/partitions.csv",
)
COMPARE_FIELDS = (
    "schema", "source_revision", "profile", "python_runtime", "platformio_core",
    "inputs", "artifacts",
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def platformio_version() -> str:
    result = subprocess.run(
        ["pio", "--version"], check=True, capture_output=True, text=True
    )
    return result.stdout.strip()


def git_revision(repo_root: Path) -> str:
    result = subprocess.run(
        ["git", "rev-parse", "HEAD"], cwd=repo_root, check=True,
        capture_output=True, text=True,
    )
    return result.stdout.strip()


def collect(args: argparse.Namespace) -> int:
    repo_root = Path(args.repo_root).resolve()
    project_dir = Path(args.project_dir).resolve()
    build_dir = project_dir / ".pio" / "build" / args.profile
    config = build_dir / "config" / "sdkconfig.h"
    if not config.is_file():
        raise RuntimeError(f"missing effective configuration: {config}")
    config_text = config.read_text(encoding="utf-8")
    if "#define CONFIG_APP_REPRODUCIBLE_BUILD 1" not in config_text:
        raise RuntimeError("effective build does not enable CONFIG_APP_REPRODUCIBLE_BUILD")
    if "#define CONFIG_APP_COMPILE_TIME_DATE 1" in config_text:
        raise RuntimeError("effective build still embeds the compile timestamp")

    revision = args.source_revision or git_revision(repo_root)
    input_hashes = {}
    for relative in INPUTS:
        path = repo_root / relative
        if not path.is_file():
            raise RuntimeError(f"missing reproducibility input: {path}")
        input_hashes[relative] = sha256(path)

    artifact_data = {}
    output_dir = Path(args.output_dir).resolve()
    artifact_output = output_dir / args.profile / args.runner
    artifact_output.mkdir(parents=True, exist_ok=True)
    for logical_name, filename in ARTIFACTS.items():
        source = build_dir / filename
        if not source.is_file():
            raise RuntimeError(f"missing build artifact: {source}")
        artifact_data[logical_name] = {
            "filename": filename,
            "bytes": source.stat().st_size,
            "sha256": sha256(source),
        }
        shutil.copyfile(source, artifact_output / filename)

    manifest = {
        "schema": SCHEMA,
        "source_revision": revision,
        "profile": args.profile,
        "runner": args.runner,
        "python_runtime": sys.version.split()[0],
        "platformio_core": platformio_version(),
        "inputs": input_hashes,
        "artifacts": artifact_data,
    }
    manifest_path = output_dir / f"reproducibility-{args.profile}-{args.runner}.json"
    manifest_path.write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(f"WROTE {manifest_path}")
    for name, data in artifact_data.items():
        print(f"{args.profile} {name}: {data['sha256']} ({data['bytes']} bytes)")
    return 0


def load_manifest(path: Path) -> dict:
    data = json.loads(path.read_text(encoding="utf-8"))
    if data.get("schema") != SCHEMA:
        raise RuntimeError(f"unsupported manifest schema in {path}")
    return data


def compare_manifests(paths: list[Path], expected_runners: set[str] | None = None) -> None:
    loaded = [(path, load_manifest(path)) for path in paths]
    if expected_runners:
        runners = {manifest.get("runner") for _, manifest in loaded}
        missing = expected_runners - runners
        if missing:
            raise RuntimeError(f"missing independent runners: {sorted(missing)}")
    if len(paths) < 2:
        raise RuntimeError("at least two manifests are required")
    reference_path, reference = loaded[0]
    differences = []
    for path, manifest in loaded[1:]:
        for field in COMPARE_FIELDS:
            if manifest.get(field) != reference.get(field):
                differences.append(f"{path}: {field} differs from {reference_path}")
    if differences:
        raise RuntimeError("reproducibility mismatch:\n" + "\n".join(differences))
    profile = reference["profile"]
    runners = ", ".join(sorted(manifest["runner"] for _, manifest in loaded))
    print(f"PASS: reproducible {profile} across {runners}")


def compare(args: argparse.Namespace) -> int:
    compare_manifests(
        [Path(path).resolve() for path in args.manifest],
        set(args.expected_runner or []),
    )
    return 0


def compare_tree(args: argparse.Namespace) -> int:
    root = Path(args.root).resolve()
    paths = sorted(root.rglob("reproducibility-*.json"))
    if not paths:
        raise RuntimeError(f"no reproducibility manifests below {root}")
    groups: dict[str, list[Path]] = {}
    for path in paths:
        profile = load_manifest(path).get("profile")
        groups.setdefault(profile, []).append(path)
    expected_profiles = {"waveshare-p4-rev1", "waveshare-p4"}
    if set(groups) != expected_profiles:
        raise RuntimeError(f"unexpected profile set: {sorted(groups)}")
    for profile in sorted(groups):
        compare_manifests(groups[profile], set(args.expected_runner or []))
    return 0


def parser() -> argparse.ArgumentParser:
    root = argparse.ArgumentParser(description=__doc__)
    commands = root.add_subparsers(dest="command", required=True)

    collect_parser = commands.add_parser("collect")
    collect_parser.add_argument("--repo-root", default=".")
    collect_parser.add_argument("--project-dir", required=True)
    collect_parser.add_argument("--profile", required=True, choices=sorted({
        "waveshare-p4-rev1", "waveshare-p4"
    }))
    collect_parser.add_argument("--output-dir", required=True)
    collect_parser.add_argument("--runner", required=True)
    collect_parser.add_argument("--source-revision")
    collect_parser.set_defaults(handler=collect)

    compare_parser = commands.add_parser("compare")
    compare_parser.add_argument("--manifest", action="append", required=True)
    compare_parser.add_argument("--expected-runner", action="append")
    compare_parser.set_defaults(handler=compare)

    tree_parser = commands.add_parser("compare-tree")
    tree_parser.add_argument("--root", required=True)
    tree_parser.add_argument("--expected-runner", action="append")
    tree_parser.set_defaults(handler=compare_tree)
    return root


def main() -> int:
    args = parser().parse_args()
    try:
        return args.handler(args)
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
