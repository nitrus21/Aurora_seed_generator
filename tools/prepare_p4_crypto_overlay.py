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
    # Chunk-owned KDF adapter: byte ordering, digest continuation and idle waits.
    "hal/sha_hal.c": (None, "391d3ed4be3a773298c0775d023d51b121bb3e46016b5ed6f46741501810c47b"),
    "hal/esp32p4/include/hal/sha_ll.h": (None, "cec6b41ab0168a7f01e9340825ef7e182d1215f39798b7d41ba8e3d16adc9fac"),
    "mbedtls/port/include/sha/sha_core.h": (None, "46df52ad2dd70c5ac8de99f8aca63f5bb06bb33fca6fbb2538acdf5f6f8ec9e7"),
    "mbedtls/mbedtls/library/md.c": ("md.c", "5ba79d51ff85951cb4001a22b2428568e4bc1072b367222808f3eac1ad91884d"),
    "mbedtls/mbedtls/library/platform_util.c": ("platform_util.c", "83051fddcd37171831ff5706613dc3d336b2e53c42e6515f2f14ac0f8aa649f1"),
    "mbedtls/port/sha/core/sha.c": ("sha_core.c", "ea21ac935f2012729778d540db895f6bd3f2a7aa33d2ca52decf41a202ac006a"),
    "mbedtls/port/aes/dma/esp_aes.c": ("esp_aes_dma.c", "08d82e4b4e075a4f08e6b54e5ef7e1082f6c058006633ae1624d3afdcf54c0a4"),
    # The release patches rely on true performing a reset while clocks are on.
    "esp_security/src/esp_crypto_periph_clk.c": (None, "e6b70903c506bf0fc0556aab0245a0dc180f8b21fc733f88c4bfa85acffa7a77"),
    "esp_security/src/esp_hmac.c": ("esp_hmac.c", "f21801f9c3094b3268088dedca7e88bf6da9d4b705e5c06b6d96af164b6de8ce"),
    # Review the lock nesting, finish/reset behavior, revision gating and
    # encrypted PSRAM mapping assumptions together with the driver.
    "esp_security/src/esp_crypto_lock.c": (None, "5ec2287139419de96af04c40afdff39829655d3b67893f2bdef8096070068c2b"),
    "hal/hmac_hal.c": (None, "71398f54da9065dd8348b9aea8662e38f66b05e44dc1d027c784c9d4d8d2b36d"),
    "hal/esp32p4/include/hal/hmac_ll.h": (None, "ffef0626fa318d3a4b7adbd306b6b7ce849431f065c4ae598f2d4d0f3a72581f"),
    "hal/esp32p4/include/hal/key_mgr_ll.h": (None, "cc96ac0b3344a34dfa5962428188411edcafa662894c79f4457e17f310af3179"),
    "hal/esp32p4/include/hal/mmu_ll.h": (None, "e8d2f388c7ce90398b90d5d24eaf5d900a8426fa952fc89d8e266e3c2c1cfde8"),
    "bootloader_support/src/secure_boot_v2/secure_boot_signatures_app.c": (None, "21bd52987c91fe92e23b86e002372648128b5ea66bf54af1a886718e929ab2fb"),
    "bootloader_support/include/esp_secure_boot.h": (None, "85f65968af185648e5b8bb3c9e87f70f2c034bebef18be975c9f66d99ed096f7"),
    "esp_rom/esp32p4/include/esp32p4/rom/secure_boot.h": (None, "6330384e013a5c4c22bd76bdd236439b62121ebe1dc4cd6c315f7a685366871b"),
}


def harden_hmac(text):
    # Only the non-S2 implementation is used by this P4-only overlay. Keep
    # downstream/JTAG functions unchanged; the application never calls them.
    first = text.index("esp_err_t esp_hmac_calculate(")
    end = text.index("\nstatic ets_efuse_block_t convert_key_type", first)
    original = text[first:end]
    patched = replace_exact(original,
        "    uint32_t conf_error = hmac_hal_configure(HMAC_OUTPUT_USER, key_id);",
        "    esp_err_t result = ESP_FAIL;\n"
        "    uint32_t conf_error = hmac_hal_configure(HMAC_OUTPUT_USER, key_id);")
    patched = replace_exact(patched,
        "        esp_crypto_hmac_lock_release();\n        return ESP_FAIL;",
        "        goto cleanup;")
    patched = replace_exact(patched,
        "        hmac_hal_write_one_block_512(block);",
        "        hmac_hal_write_one_block_512(block);\n"
        "        auroraSecureZero(block, sizeof(block));\n"
        "        auroraSecureZero(&bit_len, sizeof(bit_len));")
    patched = replace_exact(patched,
        "        hmac_hal_next_block_padding();\n        hmac_hal_write_block_512(block);",
        "        hmac_hal_next_block_padding();\n        hmac_hal_write_block_512(block);\n"
        "        auroraSecureZero(block, sizeof(block));\n"
        "        auroraSecureZero(&bit_len, sizeof(bit_len));")
    patched = replace_exact(patched,
        "    esp_crypto_key_mgr_enable_periph_clk(true);",
        "    const bool use_key_manager = key_mgr_ll_is_supported();\n"
        "    if (use_key_manager) {\n"
        "        esp_crypto_key_manager_lock_acquire();\n"
        "        esp_crypto_key_mgr_enable_periph_clk(true);\n"
        "    }")
    patched = replace_exact(patched,
        "    hmac_hal_read_result_256(hmac);",
        "    hmac_hal_read_result_256(hmac);\n"
        "    result = ESP_OK;\n"
        "cleanup:\n"
        "    /* Both normal and configure-error exits retain the HMAC/SHA-AES\n"
        "     * lock while clearing result and message state. The pinned HAL\n"
        "     * finishes on either path. Reset each peripheral before clock-off. */\n"
        "    hmac_ll_clean();\n"
        "    esp_crypto_hmac_enable_periph_clk(true);\n"
        "    esp_crypto_sha_enable_periph_clk(true);\n"
        "    esp_crypto_ds_enable_periph_clk(true);\n"
        "    if (result != ESP_OK) auroraSecureZero(hmac, 32);")
    patched = replace_exact(patched,
        "    esp_crypto_key_mgr_enable_periph_clk(false);",
        "    if (use_key_manager) {\n"
        "        esp_crypto_key_mgr_enable_periph_clk(false);\n"
        "        esp_crypto_key_manager_lock_release();\n"
        "    }")
    patched = replace_exact(patched, "    return ESP_OK;", "    return result;")
    text = replace_exact(text, original, patched)
    return replace_exact(text, "#define SHA256_BLOCK_SZ 64",
        "#if SOC_KEY_MANAGER_HMAC_KEY_DEPLOY\n"
        "#include \"hal/key_mgr_ll.h\"\n"
        "#endif\n"
        "extern void auroraSecureZero(void *buf, size_t len);\n\n"
        "#define SHA256_BLOCK_SZ 64")


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
        elif destination == "esp_hmac.c":
            text = harden_hmac(text)
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
    print("AURORA: verified P4 Mbed TLS/HMAC cleanup and peripheral reset overlays")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--components", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    generate(args.components, args.output)
