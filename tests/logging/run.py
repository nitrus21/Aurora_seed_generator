"""Host-only logging policy regression; never opens a USB/serial port."""
from pathlib import Path
import os
import re
import shutil
import subprocess

root = Path(__file__).resolve().parents[2]
output = root / "tmp/logging-native"
output.mkdir(parents=True, exist_ok=True)
env = {k.upper(): v for k, v in os.environ.items()}
vswhere = Path(env["PROGRAMFILES(X86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
vs = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires",
    "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], env=env, text=True).strip()
vcvars = Path(vs) / "VC/Auxiliary/Build/vcvars64.bat"
for line in subprocess.check_output(f'cmd /d /s /c ""{vcvars}" >nul && set"', env=env, text=True, errors="replace").splitlines():
    if "=" in line:
        key, value = line.split("=", 1)
        env[key.upper()] = value
compiler = shutil.which("cl", path=env["PATH"])
base = [compiler, "/nologo", "/std:c++17", "/EHsc", "/O2",
        f'/I{root / "tests/logging/stubs"}', f'/I{root / "include"}',
        str(root / "tests/logging/test_logging.cpp")]
for mode in (None, 0, 1):
    name = "default" if mode is None else str(mode)
    exe = output / f"logging-{name}.exe"
    command = base + ([] if mode is None else [f"/DAURORA_DEBUG={mode}"]) + [f"/Fe{exe}"]
    result = subprocess.run(command, cwd=output, env=env, capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    result = subprocess.run([str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, "Logging arguments evaluated in release or lost in diagnostics"
    assert result.stdout == ("AURORA_LOG_TEST_MARKER 1\n" if mode == 1 else "")
    assert (b"AURORA_LOG_TEST_MARKER" in exe.read_bytes()) == (mode == 1)
    print(f"PASS: mode {name}, output, argument evaluation and binary strings")
result = subprocess.run(base + ["/DAURORA_DEBUG=2", "/Felogging-invalid.exe"],
                        cwd=output, env=env, capture_output=True, text=True)
assert result.returncode != 0 and "AURORA_DEBUG must be" in result.stdout + result.stderr
print("PASS: invalid diagnostic flag rejected")

# Application-owned sources only. The isolated benchmark is reviewed separately:
# it is never a production profile and prints public timing vectors by design.
benchmark = root / "targets/waveshare_p4/main/p4_kdf_benchmark.h"
for folder in (root / "src", root / "targets/waveshare_p4/main"):
    for source in folder.rglob("*"):
        if source.suffix not in (".c", ".cpp", ".h"):
            continue
        if source == benchmark:
            continue
        code = source.read_text(encoding="utf-8")
        code = re.sub(r"//[^\n]*|/\*.*?\*/", "", code, flags=re.S)
        assert not re.search(r"\b(?:Serial\.(?:print|printf|println|write)|printf|puts|putchar|esp_rom_printf|ets_printf|ESP_LOG\w*|log_[ewidv])\s*\(", code), source
ui = (root / "src/ui.cpp").read_text(encoding="utf-8")
p4 = (root / "targets/waveshare_p4/main/main.cpp").read_text(encoding="utf-8")
assert ui.count("AURORA_DIAG(") == 4 and p4.count("AURORA_DIAG(") == 4, "Review every new diagnostic call for secret exposure"
assert "selfTestResult_ = engine_.selfTest();" in ui and "!auroraWalletCryptoSelfTest()" in ui
cyd = (root / "src/main.cpp").read_text(encoding="utf-8")
assert "#if AURORA_DEBUG\n  Serial.begin(115200);\n#endif" in cyd
print("PASS: owned sources use diagnostic gate; CYD serial init gated; self-tests preserved")

defaults = (root / "targets/waveshare_p4/sdkconfig.defaults").read_text(encoding="utf-8")
required_silence = {
    "CONFIG_BOOTLOADER_LOG_LEVEL_NONE=y",
    "CONFIG_LOG_DEFAULT_LEVEL_NONE=y",
    "CONFIG_LOG_MAXIMUM_EQUALS_DEFAULT=y",
}
assert required_silence.issubset(set(defaults.splitlines()))
assert "CONFIG_ESP_SYSTEM_PANIC_SILENT_REBOOT=y" in defaults
print("PASS: P4 production defaults compile out bootloader/ESP-IDF logs and keep silent panics")
