"""Compile the real SD codec with portable Mbed TLS and a RAM-only card."""
from pathlib import Path
import os
import shutil
import subprocess
import sys
import argparse

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cyd', action='store_true', help='CYD policy/backend on host, no hardware I/O')
args = parser.parse_args()

root = Path(__file__).resolve().parents[2]
output = root / ("tmp/crypto-native-cyd" if args.cyd else "tmp/crypto-native-p4")
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
crypto_overlay = output / "p4-overlay"
subprocess.run([sys.executable, str(root / "tools/prepare_p4_crypto_overlay.py"),
    "--components", str(mbed.parents[1]), "--output", str(crypto_overlay)], check=True)
includes = [root / "tests/crypto", root / "tests/crypto/stubs", root / "tests/ui/stubs",
            root / "tests/native/stubs", root / "include", mbed / "include", mbed / "library"]
ubitcoin_override = os.environ.get("AURORA_TEST_UBITCOIN_LIB")
if ubitcoin_override:
    ubitcoin = Path(ubitcoin_override).expanduser().resolve()
else:
    p4_candidates = (
        root / "targets/waveshare_p4/.pio/build/waveshare-p4/_deps/ubitcoin-src/src",
        root / "targets/waveshare_p4/.pio/build/waveshare-p4-rev1/_deps/ubitcoin-src/src",
    )
    ubitcoin = next((path for path in p4_candidates if (path / "HDWallet.cpp").is_file()), None)
    if ubitcoin is None:
        raise SystemExit("Configurez d'abord un profil P4 pour rendre uBitcoin disponible aux tests hote.")
subprocess.run([sys.executable, str(root / "tools/patch_ubitcoin_p4.py"),
    "--lib-root", str(ubitcoin)], check=True)
includes.append(ubitcoin)
flags = ["/nologo", "/utf-8", "/D_CRT_SECURE_NO_WARNINGS", f'/FI"{root / "tests/crypto/config_select.h"}"']
flags += [f'/I"{path}"' for path in includes]
names = ["aes", "gcm", "md", "pkcs5", "sha256", "sha512", "platform_util", "constant_time", "cipher", "cipher_wrap", "block_cipher"]
sources = [mbed / "library" / (name + ".c") for name in names]
sources = [crypto_overlay / source.name if source.name in ("md.c", "platform_util.c") else source
           for source in sources]
sources += [ubitcoin / "utility/trezor" / (name + ".c") for name in ("sha2", "hmac", "pbkdf2", "memzero")]
names += ["sha2", "hmac", "pbkdf2", "memzero"]
c_rsp = output / "compile.rsp"
c_rsp.write_text("\n".join(flags + ["/c", "/std:c11", "/O2", "/w"] + [f'"{p}"' for p in sources]), encoding="utf-8")
cpp_rsp = output / "link.rsp"
board_flag = '/DAURORA_BOARD_CYD' if args.cyd else '/DAURORA_BOARD_P4'
cpp_rsp.write_text("\n".join(flags + ["/std:c++20", "/EHsc", "/UNDEBUG", board_flag, "/DAURORA_NATIVE_TEST", "/DAURORA_KDF_TEST",
    f'"{root / "tests/crypto/test_crypto.cpp"}"', f'"{root / "src/hardware_rng.cpp"}"',
    f'"{root / "src/wallet_kdf.cpp"}"', "/Fecrypto_tests.exe"] + [name + ".obj" for name in names]), encoding="utf-8")
for command in ([compiler, "@" + str(c_rsp)], [compiler, "@" + str(cpp_rsp)], [str(output / "crypto_tests.exe")]):
    result = subprocess.run(command, cwd=output, env=env, capture_output=True, text=True, errors="replace", timeout=180)
    print(result.stdout[-6000:] + result.stderr[-3000:], flush=True)
    if result.returncode:
        raise SystemExit(result.returncode)
