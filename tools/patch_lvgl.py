"""Harden the pinned P4 LVGL 9.5.0 dependency, refusing unknown/partial sources.

Password reallocations must retain ownership until success. On the ESP target,
an LVGL assertion must enter AURORA's allocation-free, non-returning failure
handler, never the upstream infinite loop. The platform handler is implemented
by the firmware, not by LVGL; it must not call back into the damaged GUI.
"""

from pathlib import Path
import argparse
import hashlib
import re


MARKER = "AURORA_LVGL_MEMORY_HARDENING_V1"
# Complete pristine LVGL 9.5.0 file digests, with CRLF/CR normalized to LF by
# read_text(). Reverse only our exact patch before checking an installed tree.
BASE_HASHES = {
    "src/widgets/textarea/lv_textarea.c": "c67cffa3bb4ad13580d74d6bdcb436984ef6736e038564239bac503e4e901424",
    "src/misc/lv_assert.h": "e2981f5118b8188088ab5b5344c8d39e37d554c5f59427d94976f96244cd8b11",
}


def replacements():
    old_grow = (
        "        ta->pwd_tmp = lv_realloc(ta->pwd_tmp, realloc_size);\n"
        "        LV_ASSERT_MALLOC(ta->pwd_tmp);\n"
        "        if(ta->pwd_tmp == NULL) return;\n"
    )
    new_grow = (
        "        /* Retain the original secret allocation if realloc fails. */\n"
        "        char * new_pwd_tmp = lv_realloc(ta->pwd_tmp, realloc_size);\n"
        "        LV_ASSERT_MALLOC(new_pwd_tmp);\n"
        "        if(new_pwd_tmp == NULL) return;\n"
        "        ta->pwd_tmp = new_pwd_tmp;\n"
    )
    old_shrink = (
        "        ta->pwd_tmp = lv_realloc(ta->pwd_tmp, lv_strlen(ta->pwd_tmp) + 1);\n"
        "        LV_ASSERT_MALLOC(ta->pwd_tmp);\n"
        "        if(ta->pwd_tmp == NULL) return;\n"
    )
    new_shrink = (
        "        /* A failed shrink still leaves the old allocation owned. */\n"
        "        char * new_pwd_tmp = lv_realloc(ta->pwd_tmp, lv_strlen(ta->pwd_tmp) + 1);\n"
        "        LV_ASSERT_MALLOC(new_pwd_tmp);\n"
        "        if(new_pwd_tmp == NULL) return;\n"
        "        ta->pwd_tmp = new_pwd_tmp;\n"
    )
    old_assert = '#include LV_ASSERT_HANDLER_INCLUDE\n'
    new_assert = old_assert + (
        "\n/* P4 failures must wipe owned secrets and reboot without GUI recursion. */\n"
        "#if defined(ESP_PLATFORM)\n"
        "void auroraSecurityPanic(void) __attribute__((noreturn));\n"
        "#undef LV_ASSERT_HANDLER\n"
        "#define LV_ASSERT_HANDLER auroraSecurityPanic();\n"
        "#endif\n"
    )
    return {
        "src/widgets/textarea/lv_textarea.c": [
            (old_grow, new_grow, 2),
            (old_shrink, new_shrink, 1),
        ],
        "src/misc/lv_assert.h": [(old_assert, new_assert, 1)],
    }


def prepare(path, changes, expected_hash):
    if not path.is_file():
        raise RuntimeError("AURORA: missing pinned LVGL source: %s" % path)
    original_text = path.read_text(encoding="utf-8")
    text = original_text
    prefix = "// %s\n" % MARKER
    already_patched = text.startswith(prefix)
    if already_patched:
        text = text[len(prefix):]
        if MARKER in text:
            raise RuntimeError("AURORA: invalid LVGL marker in %s" % path)
        for original, patched, count in reversed(changes):
            if text.count(patched) != count:
                raise RuntimeError("AURORA: incomplete LVGL hardening in %s" % path)
            text = text.replace(patched, original)
    if hashlib.sha256(text.encode("utf-8")).hexdigest() != expected_hash:
        raise RuntimeError("AURORA: unrecognized/partial pinned LVGL source: %s" % path)
    for original, patched, count in changes:
        if text.count(original) != count:
            raise RuntimeError("AURORA: unexpected LVGL source pattern in %s" % path)
        text = text.replace(original, patched)
    result = prefix + text
    if already_patched:
        if original_text != result:
            raise RuntimeError("AURORA: inconsistent LVGL patch in %s" % path)
        return None
    return result


def patch_tree(lvgl_root):
    version = (lvgl_root / "lv_version.h").read_text(encoding="utf-8")
    for field, expected in (("MAJOR", "9"), ("MINOR", "5"), ("PATCH", "0")):
        if re.findall(r"^#define LVGL_VERSION_%s\s+(\d+)\s*$" % field,
                      version, re.MULTILINE) != [expected]:
            raise RuntimeError("AURORA: LVGL version changed; review hardening before building")
    # Validate every file before writing any: unknown upstream must fail closed.
    prepared = [(lvgl_root / name, prepare(lvgl_root / name, changes, BASE_HASHES[name]))
                for name, changes in replacements().items()]
    changed = False
    for path, text in prepared:
        if text is not None:
            path.write_text(text, encoding="utf-8", newline="\n")
            changed = True
    return changed


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--lvgl-root", required=True, type=Path)
    args = parser.parse_args()
    changed = patch_tree(args.lvgl_root)
    print("AURORA: LVGL memory hardening " + ("applied" if changed else "already applied"))
