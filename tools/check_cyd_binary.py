"""Fail a CYD build if crash dumping or the legacy PIN session comes back."""
from pathlib import Path
import argparse
import subprocess
import struct

def verify(elf, partitions, nm):
    output = subprocess.check_output([str(nm), "-C", str(elf)], text=True)
    symbols = [line.split(maxsplit=2)[-1] for line in output.splitlines() if line.strip()]
    assert "__wrap_esp_panic_handler" in symbols
    assert "__wrap_esp_core_dump_init" in symbols
    assert "auroraCydBootCleanup" in symbols and "auroraCydUiFailure" in symbols
    assert "esp_panic_handler" not in symbols
    assert not any(s.startswith("esp_core_dump_") for s in symbols)
    assert not any("AuroraPinGuard::" in s or "AuroraUI::submitPin" in s for s in symbols)
    assert "AuroraUI::loadPrivateWallet()" in symbols
    raw = Path(partitions).read_bytes()
    entries = []
    for offset in range(0, len(raw), 32):
        record = raw[offset:offset+32]
        if record[:2] != b"\xaa\x50": break
        _, kind, subtype, address, size, label, flags = struct.unpack("<HBBII16sI", record)
        entries.append((kind, subtype, address, size, label.rstrip(b"\0"), flags))
    assert not any(kind == 1 and subtype == 3 for kind,subtype,*_ in entries)
    assert (1, 0x40, 0x3f0000, 0x10000, b"aurora_scrub", 0) in entries
    print("PASS: linked CYD image has no dump implementation or PIN session; reserved scrub partition validated")
    return output

if __name__ == "__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf",type=Path,required=True)
    parser.add_argument("--partitions",type=Path,required=True)
    parser.add_argument("--nm",type=Path,required=True)
    args=parser.parse_args()
    verify(args.elf,args.partitions,args.nm)
