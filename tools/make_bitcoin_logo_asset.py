"""Intègre le logo Bitcoin fourni en blanc sur le fond noir AURORA."""

from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets" / "bitcoin_logo_source.png"
OUT_PNG = ROOT / "assets" / "bitcoin_logo_112x160.png"
OUT_C = ROOT / "src" / "assets" / "bitcoin_logo.c"
WIDTH, HEIGHT = 112, 160


def main() -> None:
    source = Image.open(SOURCE).convert("RGBA")
    alpha = source.getchannel("A")
    bounds = alpha.getbbox()
    if bounds is None:
        raise RuntimeError("Le logo source est entièrement transparent")
    source = source.crop(bounds)
    max_width = round((WIDTH - 6) * 0.90)
    max_height = round((HEIGHT - 6) * 0.90)
    ratio = min(max_width / source.width, max_height / source.height)
    resized = source.resize(
        (max(1, round(source.width * ratio)), max(1, round(source.height * ratio))),
        Image.Resampling.LANCZOS,
    )
    symbol = Image.new("L", (WIDTH, HEIGHT), 0)
    mask = resized.getchannel("A")
    x = (WIDTH - resized.width) // 2
    y = (HEIGHT - resized.height) // 2
    symbol.paste(mask, (x, y))

    image = Image.new("RGB", (WIDTH, HEIGHT), (8, 9, 11))
    white = Image.new("RGB", (WIDTH, HEIGHT), (255, 255, 255))
    image.paste(white, mask=symbol)
    OUT_PNG.parent.mkdir(parents=True, exist_ok=True)
    OUT_C.parent.mkdir(parents=True, exist_ok=True)
    image.save(OUT_PNG, optimize=True)

    raw = bytearray()
    for r, g, b in image.getdata():
        rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
        raw.extend((rgb565 & 0xFF, rgb565 >> 8))
    rows = [
        "    " + ", ".join(f"0x{x:02x}" for x in raw[i:i + 24]) + ","
        for i in range(0, len(raw), 24)
    ]
    OUT_C.write_text(
        '#include "bitcoin_logo.h"\n\n'
        'const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST '
        'uint8_t aurora_bitcoin_logo_map[] = {\n'
        + "\n".join(rows)
        + '\n};\n\n'
        'const lv_img_dsc_t aurora_bitcoin_logo = {\n'
        f'    .header = {{.cf = LV_IMG_CF_TRUE_COLOR, .always_zero = 0, .reserved = 0, .w = {WIDTH}, .h = {HEIGHT}}},\n'
        f'    .data_size = {WIDTH * HEIGHT * 2},\n'
        '    .data = aurora_bitcoin_logo_map,\n'
        '};\n',
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
