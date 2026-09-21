"""Compile and run the host-only entropy health fault witnesses."""
from pathlib import Path
import os
import re
import shutil
import subprocess

root = Path(__file__).resolve().parents[2]
platformio = (root / "targets/waveshare_p4/platformio.ini").read_text(encoding="utf-8")
main = (root / "targets/waveshare_p4/main/main.cpp").read_text(encoding="utf-8")
diagnostic = (root / "targets/waveshare_p4/main/p4_entropy_health.h").read_text(encoding="utf-8")
revision = (root / "targets/waveshare_p4/tools/image_revision.py").read_text(encoding="utf-8")
section = re.search(r"\[env:waveshare-p4-rev1-entropy-health\](.*?)(?:\n\[|\Z)",
                    platformio, re.DOTALL)
assert section and "extends = env:waveshare-p4-rev1" in section.group(1)
assert "-DAURORA_P4_ENTROPY_HEALTH=1" in section.group(1)
assert "default_envs = waveshare-p4\n" in platformio.replace("\r\n", "\n")
assert '"waveshare-p4-rev1-entropy-health": (100, 199)' in revision
assert main.index("auroraRunP4EntropyHealth();") < main.index("bsp_display_start()")
for required in ("hardwareRngEnable();", "health.add(esp_random());", "hardwareRngDisable();"):
    assert required in diagnostic
assert not re.search(r"(?<![A-Za-z_])(?:printf|puts)\s*\(", diagnostic)
for forbidden in ("AURORA_DIAG", "sdmmc", "WalletSnapshot", "writeAuroraWallet",
                  "%08x", "%08X"):
    assert forbidden not in diagnostic
print("PASS: rev1 diagnostic is non-default, display-only and revision bounded")

base = root / "tmp/entropy-health-tests"
base.mkdir(parents=True, exist_ok=True)
out = base / f"run-{os.getpid()}"
out.mkdir()
env = {k.upper(): v for k, v in os.environ.items()}
vswhere = Path(env["PROGRAMFILES(X86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
vs = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires",
    "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"],
    env=env, text=True).strip()
vcvars = Path(vs) / "VC/Auxiliary/Build/vcvars64.bat"
for line in subprocess.check_output(f'cmd /d /s /c ""{vcvars}" >nul && set"',
                                    env=env, text=True).splitlines():
    if "=" in line:
        key, value = line.split("=", 1)
        env[key.upper()] = value
cl = shutil.which("cl", path=env["PATH"])
args = [cl, "/nologo", "/O2", "/UNDEBUG", "/std:c++17", "/EHsc",
        "/I" + str(root / "include"), str(root / "tests/rng/test_entropy_health.cpp"),
        "/Fe:" + str(out / "entropy-health.exe")]
subprocess.run(args, cwd=out, env=env, check=True)
subprocess.run([str(out / "entropy-health.exe")], cwd=out, env=env, check=True)
print(f"Evidence: {out}; deterministic witnesses only, not physical entropy")
