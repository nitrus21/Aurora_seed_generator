"""P4-only supplement to patch_ubitcoin.py for pinned crypto temporaries.

Run the V1 patch first. Both pristine V1 and already supplemented trees are
validated against whole-file SHA-256 values; changed/partial sources fail closed.
CYD 1.9.3 also applies this portable supplement via harden_cyd.py. Its memory
wipe stays portable; the conditional P4 hardware implementation is unchanged.
"""
from pathlib import Path
import argparse
import hashlib

MARKER = "// AURORA_UBITCOIN_P4_RAM_HARDENING_V2\n"
BASE_HASHES = {
    "HDWallet.cpp": "c0f44c12fc577b04c165fd69e3cd99eee25229158b77ad773aa061cf4c2a18e4",
    "utility/trezor/sha2.c": "127f7e9f1da56fa07b1c46e058d48ee63ae0690c4cc2ebc41722e66f8fefcee4",
    "utility/trezor/memzero.c": "001fde1ffaba74f9f902db548ab880a7e81b901b7518c254ebe2476aea3aad2e",
    "utility/trezor/pbkdf2.c": "4c1d6e3d8a44899cf18bebcbe82c0369fce5b5085039cbfc9c0a865b92246645",
    "utility/trezor/pbkdf2.h": "9f814e17e7c9a5f958e57c1f2228d8737c9873ed99282022739700be57727ce8",
    "utility/trezor/hmac.c": "70d062a8f41387fef8eaa610720e652d3924b10153ebd2f59892da66873b72fe",
}


def replace_exact(text, old, new, count=1):
    if text.count(old) != count:
        raise RuntimeError("AURORA P4: unexpected pinned crypto source pattern")
    return text.replace(old, new)


def replacements(name):
    if name in ("utility/trezor/pbkdf2.c", "utility/trezor/pbkdf2.h"):
        return []  # Pin the chunked API and its exact first-iteration semantics.
    if name == "utility/trezor/hmac.c":
        return [
            ("static CONFIDENTIAL uint32_t key_pad[SHA256_BLOCK_LENGTH/sizeof(uint32_t)];",
             "uint32_t key_pad[SHA256_BLOCK_LENGTH/sizeof(uint32_t)];", 1),
            ("static CONFIDENTIAL SHA256_CTX context;", "SHA256_CTX context;", 1),
        ]  # SHA256 preparation temporaries are call-owned, already wiped by upstream.
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


def group_sha256(text, reverse=False):
    # Preserve every addressable working word and schedule wipe, but issue one
    # cache writeback for a contiguous owned object instead of 12/13 tiny ones.
    # Prefix field names to prevent macro argument rescanning collisions.
    for unrolled in (True, False):
        names = "a b c d e f g h s0 s1 T1".split()
        if not unrolled:
            names.append("T2")
        old_decl = "\tsha2_word32\ta, b, c, d, e, f, g, h, s0, s1;\n"
        old_decl += ("\tsha2_word32\tT1;\n\tsha2_word32 W256[16];" if unrolled
                     else "\tsha2_word32\tT1, T2, W256[16];")
        new_decl = ("\t/* AURORA_SHA256_GROUPED_WIPE: same secrets, one owned range. */\n"
                    "\tstruct {\n\t\tsha2_word32 " + ", ".join("v_"+n for n in names) + ";\n"
                    "\t\tsha2_word32 v_W256[16];\n\t} aurora_sha_work;\n")
        new_decl += "\n".join("#define %s aurora_sha_work.v_%s" % (n,n) for n in names+["W256"])
        old_wipe = "\t/* Erase reversible schedule and addressable working state. */\n\tmemzero(W256, sizeof(W256));\n"
        old_wipe += "\n".join("\tmemzero(&%s, sizeof(%s));" % (n,n) for n in names)
        new_wipe = "\tmemzero(&aurora_sha_work, sizeof(aurora_sha_work));\n"
        new_wipe += "\n".join("#undef " + n for n in names+["W256"])
        for old,new in ((old_decl,new_decl),(old_wipe+"\n}",new_wipe+"\n}")):
            text=replace_exact(text,new if reverse else old,old if reverse else new)
    return text


def patch_tree(lib_root):
    staged = []
    for name, expected_hash in BASE_HASHES.items():
        path = lib_root / name
        text = path.read_text(encoding="utf-8")
        original = text
        patched = text.startswith(MARKER)
        grouped = "AURORA_SHA256_GROUPED_WIPE" in text
        if patched:
            text = text[len(MARKER):]
        if grouped:
            text = group_sha256(text, reverse=True)
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
        if name == "utility/trezor/sha2.c":
            text = group_sha256(text)
        text = MARKER + text
        if patched and original != text and not (name == "utility/trezor/sha2.c" and not grouped):
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
