"""Generate private-build P4 crypto overlays without changing the shared SDK.

ESP-IDF 5.5.5 / Mbed TLS 3.6.6 inputs are whole-file pinned. A dependency
upgrade must be reviewed explicitly; an unrecognized source stops the build.
"""
from pathlib import Path
import argparse
import hashlib
import sys
sys.dont_write_bytecode = True
from patch_ubitcoin_p4 import replace_exact

SOURCES = {
    "mbedtls/mbedtls/library/md.c": ("md.c", "5ba79d51ff85951cb4001a22b2428568e4bc1072b367222808f3eac1ad91884d"),
    "mbedtls/mbedtls/library/platform_util.c": ("platform_util.c", "83051fddcd37171831ff5706613dc3d336b2e53c42e6515f2f14ac0f8aa649f1"),
    "mbedtls/port/sha/core/sha.c": ("sha_core.c", "ea21ac935f2012729778d540db895f6bd3f2a7aa33d2ca52decf41a202ac006a"),
    "mbedtls/port/aes/dma/esp_aes.c": ("esp_aes_dma.c", "08d82e4b4e075a4f08e6b54e5ef7e1082f6c058006633ae1624d3afdcf54c0a4"),
    # The release patches rely on true performing a reset while clocks are on.
    "esp_security/src/esp_crypto_periph_clk.c": (None, "e6b70903c506bf0fc0556aab0245a0dc180f8b21fc733f88c4bfa85acffa7a77"),
}


def generate(components, output):
    staged = []
    for name, (destination, digest) in SOURCES.items():
        text = (components / name).read_text(encoding="utf-8")
        if hashlib.sha256(text.encode()).hexdigest() != digest:
            raise RuntimeError("AURORA P4: unrecognized crypto SDK source: " + name)
        if destination is None:
            continue
        if destination == "md.c":
            first = text.index("int mbedtls_md_hmac_finish(")
            end = text.index("\n}", first) + 2
            original = text[first:end]
            patched = replace_exact(original, "        return ret;", "        goto cleanup;", 4)
            patched = replace_exact(patched,
                "    return mbedtls_md_finish(ctx, output);",
                "    ret = mbedtls_md_finish(ctx, output);\n"
                "cleanup:\n"
                "    mbedtls_platform_zeroize(tmp, sizeof(tmp));\n"
                "    return ret;")
            # Invalid context has not handled any secret yet, but wipe tmp on
            # this path too so every exit has the same cleanup ownership.
            patched = replace_exact(patched,
                "        return MBEDTLS_ERR_MD_BAD_INPUT_DATA;",
                "        ret = MBEDTLS_ERR_MD_BAD_INPUT_DATA;\n        goto cleanup;")
            text = replace_exact(text, original, patched)
        elif destination == "platform_util.c":
            first = text.index("void mbedtls_platform_zeroize(void *buf, size_t len)")
            end = text.index("\n}", first) + 2
            original = text[first:end]
            signature, body = original.split("\n{\n", 1)
            patched = (
                "#if defined(AURORA_BOARD_P4) && !defined(AURORA_NATIVE_TEST)\n"
                "extern void auroraSecureZero(void *buf, size_t len);\n"
                "#endif\n" + signature + "\n{\n"
                "#if defined(AURORA_BOARD_P4) && !defined(AURORA_NATIVE_TEST)\n"
                "    auroraSecureZero(buf, len);\n"
                "#else\n" + body[:-2] + "\n#endif\n}")
            text = replace_exact(text, original, patched)
        else:
            peripheral = "aes" if destination == "esp_aes_dma.c" else "sha"
            old = "    esp_crypto_%s_enable_periph_clk(false);" % peripheral
            new = (
                "    /* Caller has consumed output; still holding the shared crypto lock.\n"
                "     * The pinned SDK true path resets peripheral (and crypto DMA).\n"
                "     * Reset now, not only on the next acquisition. */\n"
                "    esp_crypto_%s_enable_periph_clk(true);\n" % peripheral) + old
            text = replace_exact(text, old, new)
        staged.append((output / destination, "// AURORA_P4_CRYPTO_OVERLAY_V1\n" + text))
    output.mkdir(parents=True, exist_ok=True)
    for path, text in staged:
        if not path.exists() or path.read_text(encoding="utf-8") != text:
            path.write_text(text, encoding="utf-8", newline="\n")
    print("AURORA: verified P4 Mbed TLS cleanup and peripheral reset overlays")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--components", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    generate(args.components, args.output)
