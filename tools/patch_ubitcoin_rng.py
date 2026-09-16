"""Route pinned uBitcoin ESP RNG to AURORA's owned hardware entropy source."""
from pathlib import Path
import hashlib
import argparse

BASE_SHA256 = "dd27de37a21789910c615ea393f73f7eefdd2fcae9686d67035b73110ead0b00"
OLD = "  uint32_t __attribute__((weak)) random32(void){\n    return esp_random();\n  }"
NEW = ("  extern uint32_t auroraCryptoRandom32(void);\n"
       "  uint32_t __attribute__((weak)) random32(void){\n"
       "    return auroraCryptoRandom32();\n  }")


def patch_tree(root):
    path = Path(root) / "utility/trezor/rand.c"
    text = path.read_text(encoding="utf-8")
    original = text
    if NEW in text:
        if text.count(NEW) != 1:
            raise RuntimeError("AURORA: duplicate RNG bridge")
        text = text.replace(NEW, OLD, 1)
    if hashlib.sha256(text.encode()).hexdigest() != BASE_SHA256 or text.count(OLD) != 1:
        raise RuntimeError("AURORA: unexpected pinned RNG source; refusing unverified build")
    updated = text.replace(OLD, NEW, 1)
    if updated != original:
        path.write_text(updated, encoding="utf-8", newline="\n")
    print("AURORA: verified uBitcoin hardware entropy bridge")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--lib-root", required=True, type=Path)
    patch_tree(parser.parse_args().lib_root)
