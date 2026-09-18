"""Test the real pinned uBitcoin RNG bridge on both build profiles (no device)."""
from pathlib import Path
import hashlib
import importlib.util
import os
import shutil
import subprocess
import tempfile
import sys

sys.dont_write_bytecode = True

root = Path(__file__).resolve().parents[2]
base = root / "tmp/rng-tests"
base.mkdir(parents=True, exist_ok=True)
out = Path(tempfile.mkdtemp(prefix="run-", dir=base))
spec = importlib.util.spec_from_file_location("rng_patch", root / "tools/patch_ubitcoin_rng.py")
patch = importlib.util.module_from_spec(spec)
spec.loader.exec_module(patch)
lib = root / ".pio/libdeps/esp32-2432S028R/uBitcoin/src/utility/trezor"
text = (lib / "rand.c").read_text(encoding="utf-8").replace(patch.NEW, patch.OLD)
assert hashlib.sha256(text.encode()).hexdigest() == patch.BASE_SHA256
copied = out / "utility/trezor"
copied.mkdir(parents=True)
source = copied / "rand.c"
source.write_text(text, encoding="utf-8", newline="\n")
patch.patch_tree(out)
first = source.read_bytes()
patch.patch_tree(out)
assert source.read_bytes() == first
source.write_bytes(first.replace(b"return auroraCryptoRandom32();", b"return 0;"))
try:
    patch.patch_tree(out)
except RuntimeError:
    pass
else:
    raise AssertionError("corrupt bridge accepted")
source.write_bytes(first)
print("PASS: pinned bridge idempotent; tampered source rejected", flush=True)
env = {k.upper(): v for k, v in os.environ.items()}
vswhere = Path(env["PROGRAMFILES(X86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
vs = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires",
    "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], env=env, text=True).strip()
vcvars = Path(vs) / "VC/Auxiliary/Build/vcvars64.bat"
for line in subprocess.check_output(f'cmd /d /s /c ""{vcvars}" >nul && set"', env=env, text=True).splitlines():
    if "=" in line:
        k, v = line.split("=", 1)
        env[k.upper()] = v
cl = shutil.which("cl", path=env["PATH"])
crypto_names = ("ecdsa", "bignum", "secp256k1", "memzero", "address", "hasher",
                "rfc6979", "hmac", "sha2", "sha3", "ripemd160")
for profile in ("CYD", "P4"):
    flags = ["/nologo", "/O2", "/UNDEBUG", "/DESP_PLATFORM", "/DAURORA_NATIVE_TEST",
             "/DAURORA_BOARD_" + profile, "/I" + str(root / "tests/rng/stubs"),
             "/I" + str(root / "include"), "/I" + str(lib)]
    for args in (
        [cl, *flags, "/TC", "/c", "/Gy", "/FI" + str(root / "tests/rng/stubs/compiler_compat.h"), str(source),
         str(root / "tests/rng/base58_unreachable_stubs.c"), *[str(lib / (n + ".c")) for n in crypto_names]],
        [cl, *flags, "/std:c++17", "/EHsc", str(root / "tests/rng/test_rng.cpp"),
         str(root / "src/hardware_rng.cpp"), "rand.obj", "base58_unreachable_stubs.obj", *[n + ".obj" for n in crypto_names],
         "/Ferng.exe", "/link", "/OPT:REF"],
        [str(out / "rng.exe")]):
        result = subprocess.run(args, cwd=out, env=env, capture_output=True, text=True, timeout=120)
        print(result.stdout + result.stderr, flush=True)
        if result.returncode:
            raise SystemExit(result.returncode)
print(f"Evidence: {out}; simulated hardware, not a physical entropy measurement")
