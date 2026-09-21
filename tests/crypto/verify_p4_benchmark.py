"""Verify public vectors and isolation rules of the P4 KDF diagnostic."""

import hashlib
from pathlib import Path
import re


root = Path(__file__).resolve().parents[2]
header = (root / "targets/waveshare_p4/main/p4_kdf_benchmark.h").read_text(encoding="utf-8")
platformio = (root / "targets/waveshare_p4/platformio.ini").read_text(encoding="utf-8")
main = (root / "targets/waveshare_p4/main/main.cpp").read_text(encoding="utf-8")

blocks = re.findall(r"\{(10000|120000|300000|500000),\s*\{([^}]+)\}\}", header)
if [int(count) for count, _ in blocks] != [10000, 120000, 300000, 500000]:
    raise RuntimeError("unexpected P4 KDF benchmark iteration set")
password = b"aurora-public-kdf-benchmark-password"
salt = bytes(range(16))
for count, byte_list in blocks:
    actual = bytes(int(value, 16) for value in re.findall(r"0x([0-9a-f]{2})", byte_list))
    expected = hashlib.pbkdf2_hmac("sha256", password, salt, int(count), 32)
    if actual != expected:
        raise RuntimeError(f"invalid independent PBKDF2 vector for {count}")
print("PASS: four independent P4 benchmark vectors")

section = re.search(r"\[env:waveshare-p4-rev1-kdf-benchmark\](.*?)(?:\n\[|\Z)",
                   platformio, re.DOTALL)
if not section or "extends = env:waveshare-p4-rev1" not in section.group(1):
    raise RuntimeError("diagnostic profile must inherit the rev1 hardware profile")
if "-DAURORA_P4_KDF_BENCHMARK=1" not in section.group(1):
    raise RuntimeError("diagnostic compile marker is missing")
if "default_envs = waveshare-p4\n" not in platformio.replace("\r\n", "\n"):
    raise RuntimeError("diagnostic profile must never be the default build")

call = main.index("auroraRunP4KdfBenchmark();")
display = main.index("bsp_display_start()")
if call > display:
    raise RuntimeError("diagnostic must run before the UI starts")
for forbidden in ("mount", "sdmmc", "WalletSnapshot", "writeAuroraWallet"):
    if forbidden in header:
        raise RuntimeError("diagnostic unexpectedly references storage or wallet data")
print("PASS: rev1 diagnostic is non-default and runs without UI or storage")
