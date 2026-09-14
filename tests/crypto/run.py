"""Compile the real SD codec with portable Mbed TLS and a RAM-only card."""
from pathlib import Path
import os
import shutil
import subprocess

root = Path(__file__).resolve().parents[2]
output = root / "tmp/crypto-native"
output.mkdir(parents=True, exist_ok=True)
env = {k.upper(): v for k, v in os.environ.items()}
vswhere = Path(env["PROGRAMFILES(X86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
vs = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires",
    "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], env=env, text=True).strip()
vcvars = Path(vs) / "VC/Auxiliary/Build/vcvars64.bat"
for line in subprocess.check_output(f'cmd /d /s /c ""{vcvars}" >nul && set"', env=env, text=True, errors="replace").splitlines():
    if "=" in line:
        key, value = line.split("=", 1); env[key.upper()] = value
compiler = shutil.which("cl", path=env["PATH"])
mbed = Path.home() / ".platformio/packages/framework-espidf/components/mbedtls/mbedtls"
includes = [root / "tests/crypto", root / "tests/crypto/stubs", root / "tests/ui/stubs",
            root / "tests/native/stubs", root / "include", mbed / "include", mbed / "library"]
flags = ["/nologo", "/utf-8", "/D_CRT_SECURE_NO_WARNINGS", f'/FI"{root / "tests/crypto/config_select.h"}"']
flags += [f'/I"{path}"' for path in includes]
names = ["aes", "gcm", "md", "pkcs5", "sha256", "sha512", "platform_util", "constant_time", "cipher", "cipher_wrap", "block_cipher"]
sources = [mbed / "library" / (name + ".c") for name in names]
c_rsp = output / "compile.rsp"
c_rsp.write_text("\n".join(flags + ["/c", "/std:c11", "/O2", "/w"] + [f'"{p}"' for p in sources]), encoding="utf-8")
cpp_rsp = output / "link.rsp"
cpp_rsp.write_text("\n".join(flags + ["/std:c++20", "/EHsc", "/UNDEBUG", "/DAURORA_BOARD_P4", "/DAURORA_NATIVE_TEST",
    f'"{root / "tests/crypto/test_crypto.cpp"}"', "/Fecrypto_tests.exe"] + [name + ".obj" for name in names]), encoding="utf-8")
for command in ([compiler, "@" + str(c_rsp)], [compiler, "@" + str(cpp_rsp)], [str(output / "crypto_tests.exe")]):
    result = subprocess.run(command, cwd=output, env=env, capture_output=True, text=True, errors="replace", timeout=180)
    print(result.stdout[-6000:] + result.stderr[-3000:], flush=True)
    if result.returncode:
        raise SystemExit(result.returncode)
