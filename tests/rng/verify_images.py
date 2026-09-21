"""Read-only gate: the linked uBitcoin random32 must call the tested bridge."""
from pathlib import Path
import os
import subprocess

root = Path(__file__).resolve().parents[2]
packages = Path(os.environ.get("PLATFORMIO_CORE_DIR", str(Path.home() / ".platformio"))) / "packages"
exe = ".exe" if os.name == "nt" else ""
profiles = (
    ("P4 rev3", root / "targets/waveshare_p4/.pio/build/waveshare-p4", "riscv32-esp"),
    ("P4 rev1", root / "targets/waveshare_p4/.pio/build/waveshare-p4-rev1", "riscv32-esp"),
)
for name, folder, architecture in profiles:
    objdump = packages / ("toolchain-" + architecture) / "bin" / (architecture + "-elf-objdump" + exe)
    assembly = subprocess.check_output([str(objdump), "-d", "--disassemble=random32",
                                        str(folder / "firmware.elf")], text=True)
    assert "<random32>:" in assembly, (name, "missing RNG consumer")
    assert "<auroraCryptoRandom32>" in assembly, (name, "missing entropy bridge call")
    assert "<esp_random>" not in assembly, (name, "unowned RNG bypass")
    print(f"PASS: {name} linked random32 calls auroraCryptoRandom32, not raw esp_random")
