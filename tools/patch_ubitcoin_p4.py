"""P4-only supplement to patch_ubitcoin.py for pinned crypto temporaries.

Run the V1 patch first. Both pristine V1 and already supplemented trees are
validated against whole-file SHA-256 values; changed/partial sources fail closed.
This script is deliberately not part of the frozen CYD build.
"""
from pathlib import Path
import argparse
import hashlib

MARKER = "// AURORA_UBITCOIN_P4_RAM_HARDENING_V2\n"
BASE_HASHES = {
    "HDWallet.cpp": "c0f44c12fc577b04c165fd69e3cd99eee25229158b77ad773aa061cf4c2a18e4",
    "utility/trezor/sha2.c": "127f7e9f1da56fa07b1c46e058d48ee63ae0690c4cc2ebc41722e66f8fefcee4",
    "utility/trezor/memzero.c": "001fde1ffaba74f9f902db548ab880a7e81b901b7518c254ebe2476aea3aad2e",
}


def replace_exact(text, old, new, count=1):
    if text.count(old) != count:
        raise RuntimeError("AURORA P4: unexpected pinned crypto source pattern")
    return text.replace(old, new)


def replacements(name):
    if name == "HDWallet.cpp":
        return [
            ("    memcpy(arr, hex, len);\n    return len;\n}\nsize_t HDPrivateKey::to_stream",
             "    memcpy(arr, hex, len);\n    memzero(hex, sizeof(hex));\n    return len;\n}\nsize_t HDPrivateKey::to_stream", 1),
            ("    return bytes_written;\n}\nsize_t HDPrivateKey::from_stream",
             "    memzero(hex, sizeof(hex));\n    return bytes_written;\n}\nsize_t HDPrivateKey::from_stream", 1),
            ("        bn_write_be(&n, num);\n        pubKey = *this * GeneratorPoint;",
             "        bn_write_be(&n, num);\n        memzero(&n, sizeof(n));\n        pubKey = *this * GeneratorPoint;", 1),
            ("    if(l == 0){\n        return 0; // decoding error\n    }\n    ParseByteStream s(arr, sizeof(arr));\n    HDPrivateKey::from_stream(&s);\n    return xprvLen;",
             "    if(l == 0){\n        memzero(arr, sizeof(arr));\n        return 0; // decoding error\n    }\n    ParseByteStream s(arr, sizeof(arr));\n    HDPrivateKey::from_stream(&s);\n    memzero(arr, sizeof(arr));\n    return xprvLen;", 1),
            ("    return toBase58Check(hex, sizeof(hex), arr, len);\n}\nint HDPrivateKey::address",
             "    const int result = toBase58Check(hex, sizeof(hex), arr, len);\n    memzero(hex, sizeof(hex));\n    return result;\n}\nint HDPrivateKey::address", 1),
            ("    xprv(arr, sizeof(arr));\n    return String(arr);",
             "    xprv(arr, sizeof(arr));\n    String result(arr);\n    memzero(arr, sizeof(arr));\n    return result;", 1),
        ]
    if name == "utility/trezor/memzero.c":
        return [("#include <string.h>\n\nvoid memzero(void *s, size_t n)\n{\n\tmemset(s, 0, n);\n}\n",
                 "#include <stddef.h>\n"
                 "#if defined(AURORA_BOARD_P4) && !defined(AURORA_NATIVE_TEST)\n"
                 "extern void auroraSecureZero(void *s, size_t n);\n"
                 "#endif\n\n"
                 "void memzero(void *s, size_t n)\n{\n"
                 "#if defined(AURORA_BOARD_P4) && !defined(AURORA_NATIVE_TEST)\n"
                 "    auroraSecureZero(s, n);\n"
                 "#else\n"
                 "    volatile unsigned char *p = (volatile unsigned char *)s;\n"
                 "    while (n--) *p++ = 0;\n"
                 "#if defined(__GNUC__) || defined(__clang__)\n"
                 "    __asm__ __volatile__(\"\" ::: \"memory\");\n"
                 "#endif\n"
                 "#endif\n}\n", 1)]
    raise RuntimeError("Unknown pinned source")


def sha_replacements(text):
    result = []
    for bits in (256, 512):
        word_bits = bits // 8
        start = ("void sha%d_Transform(const sha2_word%d* state_in, "
                 "const sha2_word%d* data, sha2_word%d* state_out) {" % (bits, word_bits, word_bits, word_bits))
        position = 0
        for unrolled in (True, False):
            first = text.index(start, position)
            end = text.index("\n}", first) + 2
            old = text[first:end]
            scalars = "a b c d e f g h s0 s1 T1".split()
            if not unrolled:
                scalars.append("T2")
            cleanup = "\t/* Clean up */\n\ta = b = c = d = e = f = g = h = T1 = "
            cleanup += "0;" if unrolled else "T2 = 0;"
            wipe = "\t/* Erase reversible schedule and addressable working state. */\n"
            wipe += "\tmemzero(W%d, sizeof(W%d));\n" % (bits, bits)
            wipe += "\n".join("\tmemzero(&%s, sizeof(%s));" % (v, v) for v in scalars)
            new = replace_exact(old, cleanup, wipe)
            result.append((old, new, 1))
            position = end
    return result


def patch_tree(lib_root):
    staged = []
    for name, expected_hash in BASE_HASHES.items():
        path = lib_root / name
        text = path.read_text(encoding="utf-8")
        original = text
        patched = text.startswith(MARKER)
        if patched:
            text = text[len(MARKER):]
        if name == "utility/trezor/sha2.c":
            # Reverse our exact cleanup first, allowing the original whole-file
            # digest to validate both first and repeated invocations.
            if patched:
                for bits in (256, 512):
                    for unrolled in (True, False):
                        scalars = "a b c d e f g h s0 s1 T1".split()
                        if not unrolled:
                            scalars.append("T2")
                        wipe = "\t/* Erase reversible schedule and addressable working state. */\n"
                        wipe += "\tmemzero(W%d, sizeof(W%d));\n" % (bits, bits)
                        wipe += "\n".join("\tmemzero(&%s, sizeof(%s));" % (v, v) for v in scalars)
                        cleanup = "\t/* Clean up */\n\ta = b = c = d = e = f = g = h = T1 = "
                        cleanup += "0;" if unrolled else "T2 = 0;"
                        # Include function closing brace: T1 prefix also occurs
                        # in the longer rolled cleanup containing T2.
                        text = replace_exact(text, wipe + "\n}", cleanup + "\n}")
            changes = sha_replacements(text)
        else:
            changes = replacements(name)
            if patched:
                for old, new, count in reversed(changes):
                    text = replace_exact(text, new, old, count)
        if hashlib.sha256(text.encode()).hexdigest() != expected_hash:
            raise RuntimeError("AURORA P4: unrecognized/partial pinned source: " + name)
        for old, new, count in changes:
            text = replace_exact(text, old, new, count)
        text = MARKER + text
        if patched and original != text:
            raise RuntimeError("AURORA P4: inconsistent hardening marker: " + name)
        staged.append((path, original, text))
    # Validate every input before changing any dependency file.
    for path, original, text in staged:
        if original != text:
            path.write_text(text, encoding="utf-8", newline="\n")
    print("AURORA: verified P4 crypto temporary hardening")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--lib-root", required=True, type=Path)
    patch_tree(parser.parse_args().lib_root)
