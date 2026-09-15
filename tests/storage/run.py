"""Compile actual P4 storage.cpp with public fixtures and isolated SDK stubs."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
base = root / "tmp/storage-native"
base.mkdir(parents=True, exist_ok=True)
output = Path(tempfile.mkdtemp(prefix="run-", dir=base))
env = {key.upper(): value for key, value in os.environ.items()}
vswhere = Path(env["PROGRAMFILES(X86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
vs = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires",
    "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], env=env, text=True).strip()
vcvars = Path(vs) / "VC/Auxiliary/Build/vcvars64.bat"
for line in subprocess.check_output(f'cmd /d /s /c ""{vcvars}" >nul && set"', env=env, text=True, errors="replace").splitlines():
    if "=" in line:
        key, value = line.split("=", 1)
        env[key.upper()] = value
compiler = shutil.which("cl", path=env["PATH"])
assert compiler
command = [compiler, "/nologo", "/utf-8", "/std:c++20", "/EHsc", "/O2", "/UNDEBUG",
    "/D_CRT_SECURE_NO_WARNINGS", "/D_CRT_NONSTDC_NO_DEPRECATE", "/DAURORA_BOARD_P4", "/DAURORA_NATIVE_TEST",
    "/I" + str(root / "tests/storage/stubs"), "/I" + str(root / "include"),
    str(root / "tests/storage/test_storage.cpp"), "/Fetest_storage.exe"]
for args in (command, [str(output / "test_storage.exe")]):
    result = subprocess.run(args, cwd=output, env=env, capture_output=True, text=True, errors="replace", timeout=120)
    print(result.stdout + result.stderr, flush=True)
    if result.returncode:
        raise SystemExit(result.returncode)
print(f"Public test artifacts: {output}")
