"""Convertit le visuel source en image LVGL RGB565 320x240.

Usage: python tools/make_splash_asset.py <source.png>
"""
from pathlib import Path
import sys
from PIL import Image, ImageEnhance

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "assets"
OUT_C = ROOT / "src" / "assets" / "splash_img.c"
OUT_H = ROOT / "include" / "splash_img.h"


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("Usage: make_splash_asset.py <source.png>")
    source = Path(sys.argv[1])
    ASSETS.mkdir(parents=True, exist_ok=True)
    OUT_C.parent.mkdir(parents=True, exist_ok=True)

    img = Image.open(source).convert("RGB")
    src_ratio = img.width / img.height
    target_ratio = 4 / 3
    if src_ratio > target_ratio:
        new_w = round(img.height * target_ratio)
        left = (img.width - new_w) // 2
        img = img.crop((left, 0, left + new_w, img.height))
    elif src_ratio < target_ratio:
        new_h = round(img.width / target_ratio)
        top = (img.height - new_h) // 2
        img = img.crop((0, top, img.width, top + new_h))
    img = img.resize((320, 240), Image.Resampling.LANCZOS)
    img = ImageEnhance.Contrast(img).enhance(1.08)
    img.save(ASSETS / "splash_320x240.png", optimize=True)

    raw = bytearray()
    for r, g, b in img.getdata():
        rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
        raw.extend((rgb565 & 0xFF, rgb565 >> 8))

    rows = []
    for i in range(0, len(raw), 24):
        rows.append("    " + ", ".join(f"0x{x:02x}" for x in raw[i:i + 24]) + ",")
    OUT_C.write_text(
        '#include "splash_img.h"\n\n'
        'const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST '
        'uint8_t aurora_splash_map[] = {\n'
        + "\n".join(rows)
        + '\n};\n\n'
        'const lv_img_dsc_t aurora_splash = {\n'
        '    .header = {.cf = LV_IMG_CF_TRUE_COLOR, .always_zero = 0, .reserved = 0, .w = 320, .h = 240},\n'
        '    .data_size = 153600,\n'
        '    .data = aurora_splash_map,\n'
        '};\n',
        encoding="utf-8",
    )
    OUT_H.write_text(
        '#pragma once\n#include <Arduino.h>\n#include <lvgl.h>\n'
        'LV_IMG_DECLARE(aurora_splash);\n',
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
