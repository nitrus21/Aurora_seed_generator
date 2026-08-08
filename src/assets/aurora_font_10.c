/*******************************************************************************
 * Size: 10 px
 * Bpp: 2
 * Opts: --size 10 --bpp 2 --format lvgl --no-compress --font C:\Windows\Fonts\Roboto_Regular.ttf -r 0x20-0xFF,0x152-0x153,0x2000-0x206F -o src\assets\aurora_font_10.c --lv-font-name aurora_font_10 --lv-include lvgl.h
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef AURORA_FONT_10
#define AURORA_FONT_10 1
#endif

#if AURORA_FONT_10

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */

    /* U+0021 "!" */
    0x32, 0x22, 0x21, 0x2,

    /* U+0022 "\"" */
    0x59, 0x60, 0x40,

    /* U+0023 "#" */
    0x5, 0x50, 0x22, 0x3, 0xff, 0x2, 0x20, 0x8,
    0x81, 0xff, 0x81, 0x54, 0x8, 0x80,

    /* U+0024 "$" */
    0x0, 0x0, 0x50, 0x1f, 0x3, 0x1c, 0x30, 0x82,
    0x80, 0x7, 0x40, 0xc, 0x60, 0xc2, 0xf4, 0x8,
    0x0,

    /* U+0025 "%" */
    0x28, 0x1, 0x54, 0x45, 0x58, 0xa, 0x50, 0x2,
    0x64, 0x22, 0x20, 0x88, 0x80, 0x29,

    /* U+0026 "&" */
    0x1f, 0x0, 0xc5, 0x3, 0x24, 0x7, 0x40, 0x3a,
    0x11, 0x8e, 0x86, 0xd, 0xa, 0xec,

    /* U+0027 "'" */
    0x44, 0x40,

    /* U+0028 "(" */
    0x4, 0x18, 0x20, 0x30, 0x60, 0x60, 0x60, 0x20,
    0x30, 0x24, 0x8, 0x0,

    /* U+0029 ")" */
    0x80, 0x82, 0x5, 0x18, 0x20, 0x86, 0x24, 0xc5,
    0x0,

    /* U+002A "*" */
    0x8, 0x16, 0x82, 0xd0, 0xe0, 0x0, 0x0,

    /* U+002B "+" */
    0x8, 0x0, 0x90, 0x9, 0xb, 0xfc, 0x9, 0x0,
    0x90,

    /* U+002C "," */
    0x59, 0x40,

    /* U+002D "-" */
    0xb4,

    /* U+002E "." */
    0x2,

    /* U+002F "/" */
    0x2, 0x5, 0x8, 0x8, 0x24, 0x30, 0x20, 0x80,
    0x40,

    /* U+0030 "0" */
    0x2f, 0x3, 0xc, 0x60, 0xc5, 0xc, 0x50, 0xc6,
    0xc, 0x30, 0x81, 0xf0,

    /* U+0031 "1" */
    0xa, 0x2a, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6,

    /* U+0032 "2" */
    0x2f, 0x6, 0xc, 0x0, 0xc0, 0x18, 0x3, 0x0,
    0xc0, 0x24, 0x7, 0xfc,

    /* U+0033 "3" */
    0x2f, 0x18, 0x30, 0x18, 0x3c, 0x1, 0x80, 0x36,
    0xc, 0xbc,

    /* U+0034 "4" */
    0x3, 0x40, 0x74, 0xe, 0x42, 0x24, 0x62, 0x4b,
    0xfd, 0x2, 0x40, 0x24,

    /* U+0035 "5" */
    0x2f, 0xc3, 0x0, 0x30, 0x3, 0xf4, 0x10, 0xc0,
    0x8, 0x30, 0xc1, 0xf4,

    /* U+0036 "6" */
    0xb, 0x2, 0x80, 0x30, 0x7, 0xf4, 0x70, 0xc6,
    0xc, 0x30, 0xc1, 0xf4,

    /* U+0037 "7" */
    0xbf, 0xc0, 0xc, 0x1, 0x40, 0x30, 0x6, 0x0,
    0x80, 0xc, 0x2, 0x40,

    /* U+0038 "8" */
    0x2f, 0x43, 0xc, 0x30, 0xc2, 0xf0, 0x30, 0xc5,
    0xc, 0x60, 0xc2, 0xf4,

    /* U+0039 "9" */
    0x2f, 0x18, 0x65, 0xd, 0x87, 0x2e, 0xc0, 0x30,
    0x24, 0x74,

    /* U+003A ":" */
    0x20, 0x0, 0x2,

    /* U+003B ";" */
    0x60, 0x0, 0x6, 0x90,

    /* U+003C "<" */
    0x2, 0x4b, 0x4b, 0x0, 0x78, 0x2, 0x40,

    /* U+003D "=" */
    0x3f, 0x80, 0x3, 0xf8,

    /* U+003E ">" */
    0x60, 0x7, 0x80, 0x2c, 0x78, 0x60, 0x0,

    /* U+003F "?" */
    0x3e, 0x24, 0x80, 0x20, 0x1c, 0xc, 0x1, 0x0,
    0x0, 0x20,

    /* U+0040 "@" */
    0x2, 0xa8, 0x2, 0x1, 0x82, 0xa, 0x20, 0x88,
    0x81, 0x52, 0x20, 0x55, 0x88, 0x15, 0x23, 0x10,
    0x8a, 0x68, 0x24, 0x0, 0x1, 0xa8, 0x0,

    /* U+0041 "A" */
    0x7, 0x0, 0x2c, 0x0, 0xd8, 0x6, 0x30, 0x20,
    0x80, 0xff, 0x86, 0x3, 0x24, 0x8,

    /* U+0042 "B" */
    0x3f, 0x83, 0x9, 0x30, 0x93, 0xfc, 0x30, 0x93,
    0x6, 0x30, 0x93, 0xf8,

    /* U+0043 "C" */
    0xf, 0x80, 0xc1, 0x86, 0x2, 0x18, 0x0, 0x60,
    0x1, 0x80, 0x83, 0x6, 0x3, 0xe0,

    /* U+0044 "D" */
    0x3f, 0x43, 0x9, 0x30, 0x33, 0x3, 0x30, 0x33,
    0x3, 0x30, 0x93, 0xf4,

    /* U+0045 "E" */
    0x3f, 0xd3, 0x0, 0x30, 0x3, 0xf8, 0x30, 0x3,
    0x0, 0x30, 0x3, 0xfd,

    /* U+0046 "F" */
    0x3f, 0xc3, 0x0, 0x30, 0x3, 0x0, 0x3f, 0x83,
    0x0, 0x30, 0x3, 0x0,

    /* U+0047 "G" */
    0xf, 0xc0, 0xc1, 0xc6, 0x1, 0x18, 0x0, 0x61,
    0xf1, 0x80, 0xc3, 0x43, 0x3, 0xf4,

    /* U+0048 "H" */
    0x30, 0x24, 0xc0, 0x93, 0x2, 0x4f, 0xfd, 0x30,
    0x24, 0xc0, 0x93, 0x2, 0x4c, 0x9,

    /* U+0049 "I" */
    0x33, 0x33, 0x33, 0x33,

    /* U+004A "J" */
    0x0, 0xc0, 0x30, 0xc, 0x3, 0x0, 0xc0, 0x29,
    0x18, 0xf8,

    /* U+004B "K" */
    0x30, 0x60, 0xc7, 0x3, 0x30, 0xf, 0x40, 0x3b,
    0x0, 0xca, 0x3, 0xd, 0xc, 0xc,

    /* U+004C "L" */
    0x30, 0x3, 0x0, 0x30, 0x3, 0x0, 0x30, 0x3,
    0x0, 0x30, 0x3, 0xfc,

    /* U+004D "M" */
    0x30, 0x7, 0x38, 0xf, 0x2c, 0xb, 0x29, 0x27,
    0x36, 0x33, 0x33, 0x63, 0x32, 0xd3, 0x30, 0xc3,

    /* U+004E "N" */
    0x30, 0x24, 0xe0, 0x93, 0xc2, 0x4d, 0xc9, 0x32,
    0x64, 0xc3, 0x93, 0xb, 0x4c, 0xd,

    /* U+004F "O" */
    0xf, 0x80, 0xd1, 0xc6, 0x3, 0x18, 0x9, 0x60,
    0x25, 0x80, 0xc3, 0x47, 0x3, 0xe0,

    /* U+0050 "P" */
    0x3f, 0xc3, 0x6, 0x30, 0x33, 0x6, 0x3f, 0x83,
    0x0, 0x30, 0x3, 0x0,

    /* U+0051 "Q" */
    0xf, 0x80, 0xc1, 0x86, 0x3, 0x14, 0x8, 0x50,
    0x21, 0x80, 0xc3, 0x6, 0x3, 0xf4, 0x0, 0x70,
    0x0, 0x0,

    /* U+0052 "R" */
    0x3f, 0x83, 0x9, 0x30, 0x63, 0x9, 0x3f, 0x83,
    0x18, 0x30, 0xc3, 0x6,

    /* U+0053 "S" */
    0x1f, 0x83, 0x9, 0x60, 0x2, 0x90, 0x2, 0xc0,
    0x9, 0x60, 0x92, 0xf8,

    /* U+0054 "T" */
    0xbf, 0xe0, 0x50, 0x5, 0x0, 0x50, 0x5, 0x0,
    0x50, 0x5, 0x0, 0x50,

    /* U+0055 "U" */
    0x60, 0x36, 0x3, 0x60, 0x36, 0x3, 0x60, 0x36,
    0x3, 0x30, 0x61, 0xf8,

    /* U+0056 "V" */
    0x90, 0x31, 0x81, 0x83, 0x9, 0x8, 0x30, 0x18,
    0x80, 0x39, 0x0, 0xb0, 0x1, 0xc0,

    /* U+0057 "W" */
    0x90, 0xc1, 0x98, 0x74, 0x92, 0x2a, 0x30, 0xcc,
    0x8c, 0x23, 0x32, 0xa, 0x8a, 0x41, 0xd1, 0xd0,
    0x30, 0x30,

    /* U+0058 "X" */
    0x60, 0x63, 0x4c, 0x1e, 0x80, 0xb0, 0xb, 0x1,
    0xe8, 0x30, 0xc6, 0x6,

    /* U+0059 "Y" */
    0x90, 0x63, 0xc, 0x25, 0x80, 0xf0, 0xa, 0x0,
    0x50, 0x5, 0x0, 0x50,

    /* U+005A "Z" */
    0x7f, 0xd0, 0xc, 0x2, 0x40, 0x70, 0xc, 0x2,
    0x80, 0x30, 0xb, 0xfe,

    /* U+005B "[" */
    0x75, 0x86, 0x18, 0x61, 0x86, 0x18, 0x61, 0x87,
    0x40,

    /* U+005C "\\" */
    0x80, 0x18, 0x3, 0x0, 0x90, 0x8, 0x3, 0x0,
    0x90, 0x8, 0x1, 0x0,

    /* U+005D "]" */
    0xf3, 0x33, 0x33, 0x33, 0x33, 0xf0,

    /* U+005E "^" */
    0x18, 0x3c, 0x29, 0x92,

    /* U+005F "_" */
    0xff, 0x40,

    /* U+0060 "`" */
    0x50, 0xc0,

    /* U+0061 "a" */
    0x2f, 0x4, 0x61, 0xb9, 0x82, 0x61, 0x8f, 0xa0,

    /* U+0062 "b" */
    0x60, 0x6, 0x0, 0x7f, 0x47, 0xc, 0x60, 0xc6,
    0xc, 0x70, 0xc6, 0xf4,

    /* U+0063 "c" */
    0x2f, 0x18, 0x39, 0x1, 0x40, 0x60, 0x8b, 0xc0,

    /* U+0064 "d" */
    0x0, 0xc0, 0x32, 0xed, 0x83, 0x50, 0xd4, 0x36,
    0xc, 0xbb,

    /* U+0065 "e" */
    0x1f, 0x8, 0x37, 0xfd, 0x40, 0x70, 0x4b, 0xd0,

    /* U+0066 "f" */
    0xd, 0x30, 0x30, 0xbc, 0x30, 0x30, 0x30, 0x30,
    0x30,

    /* U+0067 "g" */
    0x2e, 0xdc, 0x35, 0xd, 0x43, 0x60, 0xcb, 0xb0,
    0xc, 0xbc,

    /* U+0068 "h" */
    0x60, 0x18, 0x6, 0xb5, 0xc3, 0x60, 0xd8, 0x36,
    0xd, 0x83,

    /* U+0069 "i" */
    0x20, 0x22, 0x22, 0x22,

    /* U+006A "j" */
    0x18, 0x1, 0x86, 0x18, 0x61, 0x86, 0x19, 0xd0,

    /* U+006B "k" */
    0x60, 0x6, 0x0, 0x62, 0x46, 0xa0, 0x7c, 0x7,
    0x90, 0x63, 0x6, 0x18,

    /* U+006C "l" */
    0x22, 0x22, 0x22, 0x22,

    /* U+006D "m" */
    0x6f, 0x6e, 0x18, 0x30, 0xc6, 0xc, 0x31, 0x83,
    0xc, 0x60, 0xc3, 0x18, 0x30, 0xc0,

    /* U+006E "n" */
    0x6b, 0x5c, 0x36, 0xd, 0x83, 0x60, 0xd8, 0x30,

    /* U+006F "o" */
    0x1f, 0x47, 0xc, 0x90, 0x85, 0x8, 0x30, 0xc1,
    0xf4,

    /* U+0070 "p" */
    0x7b, 0x46, 0xc, 0x60, 0xc6, 0xc, 0x70, 0xc7,
    0xf4, 0x60, 0x6, 0x0,

    /* U+0071 "q" */
    0x2e, 0xd8, 0x35, 0xd, 0x43, 0x60, 0xcb, 0xb0,
    0xc, 0x3,

    /* U+0072 "r" */
    0x0, 0x7c, 0x70, 0x60, 0x60, 0x60, 0x60,

    /* U+0073 "s" */
    0x2f, 0x18, 0x53, 0x80, 0x1d, 0x51, 0x8b, 0xc0,

    /* U+0074 "t" */
    0x10, 0xcb, 0x8c, 0x30, 0xc3, 0xb,

    /* U+0075 "u" */
    0x60, 0xd8, 0x36, 0xd, 0x83, 0x21, 0xcb, 0xb0,

    /* U+0076 "v" */
    0x81, 0x58, 0x83, 0x30, 0x98, 0x1c, 0x3, 0x0,

    /* U+0077 "w" */
    0x83, 0x8, 0x53, 0x8c, 0x25, 0xc8, 0x38, 0xa4,
    0x2c, 0x70, 0x18, 0x30,

    /* U+0078 "x" */
    0x62, 0x4d, 0xc1, 0xc0, 0x74, 0x33, 0x28, 0x90,

    /* U+0079 "y" */
    0x82, 0x58, 0xc3, 0x30, 0x94, 0x1c, 0x3, 0x1,
    0x42, 0xc0,

    /* U+007A "z" */
    0x7f, 0x40, 0xc0, 0x90, 0x60, 0x30, 0x2f, 0xe0,

    /* U+007B "{" */
    0x8, 0x18, 0x24, 0x20, 0x30, 0xa0, 0x30, 0x20,
    0x24, 0x18, 0x8,

    /* U+007C "|" */
    0x22, 0x22, 0x22, 0x22, 0x21,

    /* U+007D "}" */
    0x80, 0x30, 0x30, 0x30, 0x20, 0x1c, 0x20, 0x30,
    0x30, 0x30, 0x80,

    /* U+007E "~" */
    0x2d, 0x20, 0x4b, 0x80,

    /* U+00A0 " " */

    /* U+00A1 "¡" */
    0x20, 0x12, 0x22, 0x22,

    /* U+00A2 "¢" */
    0x9, 0x7, 0x83, 0x19, 0x81, 0x50, 0x18, 0x13,
    0x18, 0x38, 0x9, 0x0,

    /* U+00A3 "£" */
    0xf, 0x82, 0x4c, 0x30, 0x3, 0x0, 0x7e, 0x2,
    0x0, 0x30, 0x7, 0xfd,

    /* U+00A4 "¤" */
    0x0, 0x1, 0xaf, 0x93, 0x47, 0x8, 0x9, 0x60,
    0x14, 0x80, 0x93, 0x47, 0xa, 0xf9, 0x0, 0x0,

    /* U+00A5 "¥" */
    0x90, 0x63, 0xc, 0x25, 0x80, 0xf0, 0x2f, 0x82,
    0xf8, 0x5, 0x0, 0x50,

    /* U+00A6 "¦" */
    0x66, 0x66, 0x6, 0x66, 0x60,

    /* U+00A7 "§" */
    0x1f, 0x83, 0x9, 0x30, 0x3, 0xd0, 0x62, 0xc5,
    0x6, 0x39, 0x90, 0x7c, 0x0, 0x96, 0xc, 0x1f,
    0x40,

    /* U+00A8 "¨" */
    0x56, 0x0,

    /* U+00A9 "©" */
    0x6, 0x90, 0x24, 0x14, 0x12, 0x88, 0x48, 0x55,
    0x88, 0x1, 0x48, 0x55, 0x12, 0x88, 0x24, 0x14,
    0x6, 0x90,

    /* U+00AA "ª" */
    0x2d, 0x12, 0x2a, 0x3b,

    /* U+00AB "«" */
    0x0, 0x9, 0x86, 0x80, 0x94, 0x16, 0x0,

    /* U+00AC "¬" */
    0x7f, 0x80, 0x20, 0x4,

    /* U+00AD "­" */
    0xb4,

    /* U+00AE "®" */
    0x6, 0x90, 0x24, 0x14, 0x16, 0x88, 0x48, 0x55,
    0x8b, 0xc1, 0x48, 0x55, 0x14, 0x18, 0x24, 0x14,
    0x6, 0x90,

    /* U+00AF "¯" */
    0x7f, 0x0,

    /* U+00B0 "°" */
    0x28, 0x58, 0x28,

    /* U+00B1 "±" */
    0x8, 0x2, 0x7, 0xfc, 0x20, 0x8, 0x0, 0x7,
    0xf8,

    /* U+00B2 "²" */
    0x68, 0x8, 0x24, 0x78,

    /* U+00B3 "³" */
    0x68, 0x1c, 0x48, 0x68,

    /* U+00B4 "´" */
    0x10, 0x80,

    /* U+00B5 "µ" */
    0x60, 0xd8, 0x36, 0xd, 0x83, 0x70, 0xdf, 0xb6,
    0x1, 0x80,

    /* U+00B6 "¶" */
    0x2f, 0x1f, 0xcb, 0xf1, 0xfc, 0x3f, 0x0, 0xc0,
    0x30, 0xc,

    /* U+00B7 "·" */
    0x2, 0x0,

    /* U+00B8 "¸" */
    0x10, 0xc6, 0x0,

    /* U+00B9 "¹" */
    0x24, 0x51, 0x45,

    /* U+00BA "º" */
    0x29, 0x53, 0x53, 0x29,

    /* U+00BB "»" */
    0x0, 0x9, 0x41, 0x60, 0x9c, 0x24, 0x0,

    /* U+00BC "¼" */
    0x20, 0x0, 0x81, 0x42, 0x8, 0x8, 0x50, 0x2,
    0x34, 0x22, 0x50, 0x8f, 0xc0, 0x5,

    /* U+00BD "½" */
    0x0, 0x0, 0x70, 0x0, 0x20, 0x80, 0x21, 0x40,
    0x12, 0x78, 0x5, 0x4c, 0x8, 0x8, 0x14, 0x20,
    0x0, 0xb8,

    /* U+00BE "¾" */
    0x28, 0x0, 0x5, 0x20, 0x1c, 0x20, 0x5, 0x80,
    0x2a, 0x4c, 0x2, 0x2c, 0x4, 0xad, 0x0, 0xc,

    /* U+00BF "¿" */
    0xc, 0x0, 0x0, 0x80, 0x60, 0x30, 0x24, 0x9,
    0x20, 0xf8,

    /* U+00C0 "À" */
    0xc, 0x0, 0x4, 0x0, 0x70, 0x2, 0xc0, 0xd,
    0x80, 0x63, 0x2, 0x8, 0xf, 0xf8, 0x60, 0x32,
    0x40, 0x80,

    /* U+00C1 "Á" */
    0x2, 0x40, 0x4, 0x0, 0x70, 0x2, 0xc0, 0xd,
    0x80, 0x63, 0x2, 0x8, 0xf, 0xf8, 0x60, 0x32,
    0x40, 0x80,

    /* U+00C2 "Â" */
    0x7, 0x0, 0x11, 0x0, 0x70, 0x2, 0xc0, 0xd,
    0x80, 0x63, 0x2, 0x8, 0xf, 0xf8, 0x60, 0x32,
    0x40, 0x80,

    /* U+00C3 "Ã" */
    0x8, 0x80, 0x4a, 0x0, 0x70, 0x2, 0xc0, 0xd,
    0x80, 0x63, 0x2, 0x8, 0xf, 0xf8, 0x60, 0x32,
    0x40, 0x80,

    /* U+00C4 "Ä" */
    0x8, 0x80, 0x0, 0x0, 0x70, 0x2, 0xc0, 0xd,
    0x80, 0x63, 0x2, 0x8, 0xf, 0xf8, 0x60, 0x32,
    0x40, 0x80,

    /* U+00C5 "Å" */
    0x6, 0x0, 0x21, 0x0, 0x60, 0x1, 0xc0, 0xb,
    0x0, 0x36, 0x1, 0x8c, 0x8, 0x20, 0x3f, 0xe1,
    0x80, 0xc9, 0x2, 0x0,

    /* U+00C6 "Æ" */
    0x0, 0x3f, 0xe0, 0x2, 0xa0, 0x0, 0xd, 0x80,
    0x0, 0x97, 0xf4, 0x3, 0x18, 0x0, 0x2f, 0xe0,
    0x1, 0xc0, 0x80, 0xd, 0x3, 0xfc,

    /* U+00C7 "Ç" */
    0xf, 0x80, 0xc1, 0x86, 0x2, 0x18, 0x0, 0x60,
    0x1, 0x80, 0x83, 0x6, 0x3, 0xe0, 0x2, 0x0,
    0x8, 0x0,

    /* U+00C8 "È" */
    0xc, 0x0, 0x0, 0x3f, 0xd3, 0x0, 0x30, 0x3,
    0xf8, 0x30, 0x3, 0x0, 0x30, 0x3, 0xfd,

    /* U+00C9 "É" */
    0x3, 0x0, 0x0, 0x3f, 0xd3, 0x0, 0x30, 0x3,
    0xf8, 0x30, 0x3, 0x0, 0x30, 0x3, 0xfd,

    /* U+00CA "Ê" */
    0xa, 0x0, 0x0, 0x3f, 0xd3, 0x0, 0x30, 0x3,
    0xf8, 0x30, 0x3, 0x0, 0x30, 0x3, 0xfd,

    /* U+00CB "Ë" */
    0x15, 0x40, 0x0, 0x3f, 0xd3, 0x0, 0x30, 0x3,
    0xf8, 0x30, 0x3, 0x0, 0x30, 0x3, 0xfd,

    /* U+00CC "Ì" */
    0x24, 0x10, 0xc3, 0xc, 0x30, 0xc3, 0xc, 0x30,

    /* U+00CD "Í" */
    0x24, 0x43, 0xc, 0x30, 0xc3, 0xc, 0x30, 0xc0,

    /* U+00CE "Î" */
    0x1c, 0x11, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc,
    0xc, 0xc,

    /* U+00CF "Ï" */
    0x23, 0x0, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc,
    0xc, 0xc,

    /* U+00D0 "Ð" */
    0x3f, 0x80, 0xc2, 0x83, 0x3, 0x2f, 0xc, 0x30,
    0x30, 0xc0, 0xc3, 0xa, 0xf, 0xe0,

    /* U+00D1 "Ñ" */
    0x9, 0x40, 0x16, 0x3, 0x2, 0x4e, 0x9, 0x3c,
    0x24, 0xdc, 0x93, 0x26, 0x4c, 0x39, 0x30, 0xb4,
    0xc0, 0xd0,

    /* U+00D2 "Ò" */
    0x9, 0x0, 0x4, 0x0, 0xf8, 0xd, 0x1c, 0x60,
    0x31, 0x80, 0x96, 0x2, 0x58, 0xc, 0x34, 0x70,
    0x3e, 0x0,

    /* U+00D3 "Ó" */
    0x1, 0x80, 0x4, 0x0, 0xf8, 0xd, 0x1c, 0x60,
    0x31, 0x80, 0x96, 0x2, 0x58, 0xc, 0x34, 0x70,
    0x3e, 0x0,

    /* U+00D4 "Ô" */
    0x7, 0x0, 0x11, 0x0, 0xf8, 0xd, 0x1c, 0x60,
    0x31, 0x80, 0x96, 0x2, 0x58, 0xc, 0x34, 0x70,
    0x3e, 0x0,

    /* U+00D5 "Õ" */
    0xb, 0x80, 0x0, 0x0, 0xf8, 0xd, 0x1c, 0x60,
    0x31, 0x80, 0x96, 0x2, 0x58, 0xc, 0x34, 0x70,
    0x3e, 0x0,

    /* U+00D6 "Ö" */
    0xc, 0xc0, 0x0, 0x0, 0xf8, 0xd, 0x1c, 0x60,
    0x31, 0x80, 0x96, 0x2, 0x58, 0xc, 0x34, 0x70,
    0x3e, 0x0,

    /* U+00D7 "×" */
    0x0, 0x18, 0x62, 0xb0, 0x34, 0x37, 0x14, 0x10,

    /* U+00D8 "Ø" */
    0xf, 0xa0, 0xd2, 0xc6, 0xb, 0x18, 0x59, 0x62,
    0x24, 0xa0, 0xc3, 0x87, 0xb, 0xe0, 0x10, 0x0,

    /* U+00D9 "Ù" */
    0xc, 0x0, 0x10, 0x60, 0x36, 0x3, 0x60, 0x36,
    0x3, 0x60, 0x36, 0x3, 0x30, 0x61, 0xf8,

    /* U+00DA "Ú" */
    0x2, 0x40, 0x10, 0x60, 0x36, 0x3, 0x60, 0x36,
    0x3, 0x60, 0x36, 0x3, 0x30, 0x61, 0xf8,

    /* U+00DB "Û" */
    0x6, 0x0, 0x44, 0x60, 0x36, 0x3, 0x60, 0x36,
    0x3, 0x60, 0x36, 0x3, 0x30, 0x61, 0xf8,

    /* U+00DC "Ü" */
    0x8, 0x80, 0x0, 0x60, 0x36, 0x3, 0x60, 0x36,
    0x3, 0x60, 0x36, 0x3, 0x30, 0x61, 0xf8,

    /* U+00DD "Ý" */
    0x3, 0x0, 0x0, 0x90, 0x63, 0xc, 0x25, 0x80,
    0xf0, 0xa, 0x0, 0x50, 0x5, 0x0, 0x50,

    /* U+00DE "Þ" */
    0x20, 0x2, 0x0, 0x3f, 0x82, 0x9, 0x20, 0x93,
    0xf8, 0x20, 0x2, 0x0,

    /* U+00DF "ß" */
    0x2f, 0x2, 0x24, 0x62, 0x46, 0x30, 0x63, 0x46,
    0xc, 0x60, 0x56, 0xac,

    /* U+00E0 "à" */
    0x18, 0x0, 0x2, 0xf0, 0x86, 0x1b, 0x98, 0x26,
    0x18, 0xfa,

    /* U+00E1 "á" */
    0x3, 0x0, 0x2, 0xf0, 0x46, 0x1b, 0x98, 0x26,
    0x18, 0xfa,

    /* U+00E2 "â" */
    0xa, 0x0, 0x2, 0xf0, 0x46, 0x1b, 0x98, 0x26,
    0x18, 0xfa,

    /* U+00E3 "ã" */
    0x0, 0xb, 0x90, 0x0, 0xbc, 0x11, 0x86, 0xe6,
    0x9, 0x86, 0x3e, 0x80,

    /* U+00E4 "ä" */
    0x22, 0x40, 0x2, 0xf0, 0x46, 0x1b, 0x98, 0x26,
    0x18, 0xfa,

    /* U+00E5 "å" */
    0x9, 0x1, 0x80, 0x90, 0xbc, 0x11, 0x86, 0xe6,
    0x9, 0x86, 0x3e, 0x80,

    /* U+00E6 "æ" */
    0x2e, 0x7d, 0x4, 0x70, 0xc2, 0xff, 0xf2, 0x86,
    0x0, 0x92, 0xc0, 0xf, 0x9f, 0x80,

    /* U+00E7 "ç" */
    0x2f, 0x18, 0x39, 0x1, 0x40, 0x60, 0x8b, 0xc0,
    0x90, 0x24,

    /* U+00E8 "è" */
    0x18, 0x0, 0x1, 0xf0, 0x83, 0x7f, 0xd4, 0x7,
    0x4, 0xbd,

    /* U+00E9 "é" */
    0x2, 0x0, 0x1, 0xf0, 0x83, 0x7f, 0xd4, 0x7,
    0x4, 0xbd,

    /* U+00EA "ê" */
    0x9, 0x0, 0x1, 0xf0, 0x83, 0x7f, 0xd4, 0x7,
    0x4, 0xbd,

    /* U+00EB "ë" */
    0x22, 0x0, 0x1, 0xf0, 0x83, 0x7f, 0xd4, 0x7,
    0x4, 0xbd,

    /* U+00EC "ì" */
    0x20, 0x1, 0x86, 0x18, 0x61, 0x86,

    /* U+00ED "í" */
    0x24, 0x6, 0x18, 0x61, 0x86, 0x18,

    /* U+00EE "î" */
    0x28, 0x0, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,

    /* U+00EF "ï" */
    0x22, 0x0, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,

    /* U+00F0 "ð" */
    0x0, 0x2, 0xd4, 0xb, 0x0, 0x58, 0x1e, 0xc3,
    0xc, 0x60, 0x86, 0xc, 0x30, 0xc1, 0xf0,

    /* U+00F1 "ñ" */
    0x0, 0xb, 0x90, 0x1, 0xad, 0x70, 0xd8, 0x36,
    0xd, 0x83, 0x60, 0xc0,

    /* U+00F2 "ò" */
    0x18, 0x0, 0x0, 0x1f, 0x47, 0xc, 0x90, 0x85,
    0x8, 0x30, 0xc1, 0xf4,

    /* U+00F3 "ó" */
    0x3, 0x0, 0x0, 0x1f, 0x47, 0xc, 0x90, 0x85,
    0x8, 0x30, 0xc1, 0xf4,

    /* U+00F4 "ô" */
    0xa, 0x0, 0x0, 0x1f, 0x47, 0xc, 0x90, 0x85,
    0x8, 0x30, 0xc1, 0xf4,

    /* U+00F5 "õ" */
    0x0, 0x2, 0xe4, 0x0, 0x1, 0xf4, 0x70, 0xc9,
    0x8, 0x50, 0x83, 0xc, 0x1f, 0x40,

    /* U+00F6 "ö" */
    0x22, 0x40, 0x0, 0x1f, 0x47, 0xc, 0x90, 0x85,
    0x8, 0x30, 0xc1, 0xf4,

    /* U+00F7 "÷" */
    0x9, 0x0, 0x0, 0xbf, 0xc0, 0x0, 0x9, 0x0,
    0x0,

    /* U+00F8 "ø" */
    0x0, 0x42, 0xf4, 0x62, 0xc9, 0x18, 0x98, 0x87,
    0x4c, 0x2f, 0x41, 0x0,

    /* U+00F9 "ù" */
    0x18, 0x0, 0x6, 0xd, 0x83, 0x60, 0xd8, 0x32,
    0x1c, 0xbb,

    /* U+00FA "ú" */
    0x2, 0x0, 0x6, 0xd, 0x83, 0x60, 0xd8, 0x32,
    0x1c, 0xbb,

    /* U+00FB "û" */
    0x9, 0x0, 0x6, 0xd, 0x83, 0x60, 0xd8, 0x32,
    0x1c, 0xbb,

    /* U+00FC "ü" */
    0x22, 0x40, 0x6, 0xd, 0x83, 0x60, 0xd8, 0x32,
    0x1c, 0xbb,

    /* U+00FD "ý" */
    0x6, 0x0, 0x8, 0x25, 0x8c, 0x33, 0x9, 0x41,
    0xc0, 0x30, 0x14, 0x2c, 0x0,

    /* U+00FE "þ" */
    0x60, 0x6, 0x0, 0x7f, 0x47, 0xc, 0x60, 0xc6,
    0xc, 0x70, 0xc7, 0xf4, 0x60, 0x6, 0x0,

    /* U+00FF "ÿ" */
    0x33, 0x0, 0x8, 0x25, 0x8c, 0x33, 0x9, 0x41,
    0xc0, 0x30, 0x14, 0x2c, 0x0,

    /* U+0152 "Œ" */
    0x1f, 0xff, 0x8c, 0x20, 0x6, 0x8, 0x1, 0x42,
    0xfd, 0x50, 0x80, 0x18, 0x20, 0x3, 0x8, 0x0,
    0x7f, 0xfe,

    /* U+0153 "œ" */
    0x2f, 0x2f, 0xc, 0x38, 0x65, 0xf, 0xf9, 0x43,
    0x40, 0x30, 0xe0, 0x7, 0xcb, 0xd0,

    /* U+2000 " " */

    /* U+2001 " " */

    /* U+2002 " " */

    /* U+2003 " " */

    /* U+2004 " " */

    /* U+2005 " " */

    /* U+2006 " " */

    /* U+2007 " " */

    /* U+2008 " " */

    /* U+2009 " " */

    /* U+200A " " */

    /* U+200B "​" */

    /* U+2010 "‐" */
    0xb4,

    /* U+2011 "‑" */
    0xb4,

    /* U+2013 "–" */
    0x3f, 0xe0,

    /* U+2014 "—" */
    0x3f, 0xfc,

    /* U+2015 "―" */
    0x3f, 0xfc,

    /* U+2017 "‗" */
    0xff, 0x7f, 0xd0,

    /* U+2018 "‘" */
    0x16, 0x90,

    /* U+2019 "’" */
    0x65, 0x40,

    /* U+201A "‚" */
    0x69, 0x40,

    /* U+201B "‛" */
    0x95, 0x10,

    /* U+201C "“" */
    0x10, 0x6c, 0x5c,

    /* U+201D "”" */
    0x28, 0x5c, 0x44,

    /* U+201E "„" */
    0x6d, 0x79, 0x40,

    /* U+2020 "†" */
    0x8, 0x0, 0x80, 0xbf, 0xc0, 0x80, 0x8, 0x0,
    0x80, 0x8, 0x0, 0x80,

    /* U+2021 "‡" */
    0x9, 0x0, 0x90, 0x7f, 0xc0, 0x90, 0x9, 0x0,
    0x90, 0x9, 0x7, 0xfc, 0x9, 0x0, 0x90,

    /* U+2022 "•" */
    0x11, 0xe3, 0x40,

    /* U+2025 "‥" */
    0x0, 0x23,

    /* U+2026 "…" */
    0x0, 0x2, 0x33,

    /* U+2027 "‧" */
    0x80,

    /* U+2030 "‰" */
    0x28, 0x0, 0x8, 0x85, 0x0, 0x88, 0x80, 0x2,
    0x94, 0x0, 0x2, 0x66, 0x40, 0x88, 0xc8, 0x8,
    0x8c, 0x80, 0x6, 0xa8,

    /* U+2032 "′" */
    0x44, 0x40,

    /* U+2033 "″" */
    0x59, 0x60, 0x40,

    /* U+2039 "‹" */
    0x0, 0x86, 0x8, 0x14,

    /* U+203A "›" */
    0x0, 0x82, 0x48, 0x50,

    /* U+203C "‼" */
    0x32, 0x48, 0x92, 0x24, 0x89, 0x22, 0x44, 0x40,
    0x0, 0x89,

    /* U+2044 "⁄" */
    0x0, 0x0, 0x80, 0x20, 0x20, 0x14, 0x8, 0x5,
    0x0, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 40, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 0, .adv_w = 41, .box_w = 2, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 4, .adv_w = 51, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 7, .adv_w = 99, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 21, .adv_w = 90, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 38, .adv_w = 117, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 52, .adv_w = 99, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 66, .adv_w = 28, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 68, .adv_w = 55, .box_w = 4, .box_h = 12, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 80, .adv_w = 56, .box_w = 3, .box_h = 12, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 89, .adv_w = 69, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 96, .adv_w = 91, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 105, .adv_w = 31, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 107, .adv_w = 44, .box_w = 3, .box_h = 1, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 108, .adv_w = 42, .box_w = 2, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 109, .adv_w = 66, .box_w = 4, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 118, .adv_w = 90, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 130, .adv_w = 90, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 138, .adv_w = 90, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 150, .adv_w = 90, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 160, .adv_w = 90, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 172, .adv_w = 90, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 184, .adv_w = 90, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 196, .adv_w = 90, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 208, .adv_w = 90, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 220, .adv_w = 90, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 230, .adv_w = 39, .box_w = 2, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 233, .adv_w = 34, .box_w = 2, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 237, .adv_w = 81, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 244, .adv_w = 88, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 248, .adv_w = 84, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 255, .adv_w = 76, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 265, .adv_w = 144, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 288, .adv_w = 104, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 302, .adv_w = 100, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 314, .adv_w = 104, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 328, .adv_w = 105, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 340, .adv_w = 91, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 352, .adv_w = 88, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 364, .adv_w = 109, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 378, .adv_w = 114, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 392, .adv_w = 44, .box_w = 2, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 396, .adv_w = 88, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 406, .adv_w = 100, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 420, .adv_w = 86, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 432, .adv_w = 140, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 448, .adv_w = 114, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 462, .adv_w = 110, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 476, .adv_w = 101, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 488, .adv_w = 110, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 506, .adv_w = 99, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 518, .adv_w = 95, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 530, .adv_w = 95, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 542, .adv_w = 104, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 554, .adv_w = 102, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 568, .adv_w = 142, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 586, .adv_w = 100, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 598, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 610, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 622, .adv_w = 42, .box_w = 3, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 631, .adv_w = 66, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 643, .adv_w = 42, .box_w = 2, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 649, .adv_w = 67, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 653, .adv_w = 72, .box_w = 5, .box_h = 1, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 655, .adv_w = 49, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 7},
    {.bitmap_index = 657, .adv_w = 87, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 665, .adv_w = 90, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 677, .adv_w = 84, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 685, .adv_w = 90, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 695, .adv_w = 85, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 703, .adv_w = 56, .box_w = 4, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 712, .adv_w = 90, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 722, .adv_w = 88, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 732, .adv_w = 39, .box_w = 2, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 736, .adv_w = 38, .box_w = 3, .box_h = 10, .ofs_x = -1, .ofs_y = -2},
    {.bitmap_index = 744, .adv_w = 81, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 756, .adv_w = 39, .box_w = 2, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 760, .adv_w = 140, .box_w = 9, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 774, .adv_w = 88, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 782, .adv_w = 91, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 791, .adv_w = 90, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 803, .adv_w = 91, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 813, .adv_w = 54, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 820, .adv_w = 83, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 828, .adv_w = 52, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 834, .adv_w = 88, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 842, .adv_w = 78, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 850, .adv_w = 120, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 862, .adv_w = 79, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 870, .adv_w = 76, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 880, .adv_w = 79, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 888, .adv_w = 54, .box_w = 4, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 899, .adv_w = 39, .box_w = 2, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 904, .adv_w = 54, .box_w = 4, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 915, .adv_w = 109, .box_w = 7, .box_h = 2, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 919, .adv_w = 40, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 919, .adv_w = 39, .box_w = 2, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 923, .adv_w = 88, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 935, .adv_w = 93, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 947, .adv_w = 114, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 963, .adv_w = 97, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 975, .adv_w = 38, .box_w = 2, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 980, .adv_w = 98, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 997, .adv_w = 67, .box_w = 4, .box_h = 2, .ofs_x = 0, .ofs_y = 6},
    {.bitmap_index = 999, .adv_w = 126, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1017, .adv_w = 71, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 1021, .adv_w = 75, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 1028, .adv_w = 89, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 1032, .adv_w = 44, .box_w = 3, .box_h = 1, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 1033, .adv_w = 126, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1051, .adv_w = 73, .box_w = 5, .box_h = 1, .ofs_x = 0, .ofs_y = 7},
    {.bitmap_index = 1053, .adv_w = 60, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 1056, .adv_w = 85, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1065, .adv_w = 59, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 1069, .adv_w = 59, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 1073, .adv_w = 50, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 7},
    {.bitmap_index = 1075, .adv_w = 91, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1085, .adv_w = 78, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1095, .adv_w = 42, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 1097, .adv_w = 40, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1100, .adv_w = 59, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 1103, .adv_w = 73, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 1107, .adv_w = 75, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 1114, .adv_w = 117, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1128, .adv_w = 124, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1146, .adv_w = 124, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1162, .adv_w = 76, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1172, .adv_w = 104, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1190, .adv_w = 104, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1208, .adv_w = 104, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1226, .adv_w = 104, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1244, .adv_w = 104, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1262, .adv_w = 104, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1282, .adv_w = 150, .box_w = 11, .box_h = 8, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 1304, .adv_w = 104, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1322, .adv_w = 91, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1337, .adv_w = 91, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1352, .adv_w = 91, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1367, .adv_w = 91, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1382, .adv_w = 44, .box_w = 3, .box_h = 10, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 1390, .adv_w = 44, .box_w = 3, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1398, .adv_w = 44, .box_w = 4, .box_h = 10, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 1408, .adv_w = 44, .box_w = 4, .box_h = 10, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 1418, .adv_w = 107, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1432, .adv_w = 114, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1450, .adv_w = 110, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1468, .adv_w = 110, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1486, .adv_w = 110, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1504, .adv_w = 110, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1522, .adv_w = 110, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1540, .adv_w = 85, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 1548, .adv_w = 110, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1564, .adv_w = 104, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1579, .adv_w = 104, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1594, .adv_w = 104, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1609, .adv_w = 104, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1624, .adv_w = 96, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1639, .adv_w = 95, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1651, .adv_w = 95, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1663, .adv_w = 87, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1673, .adv_w = 87, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1683, .adv_w = 87, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1693, .adv_w = 87, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1705, .adv_w = 87, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1715, .adv_w = 87, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1727, .adv_w = 135, .box_w = 9, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1741, .adv_w = 84, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1751, .adv_w = 85, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1761, .adv_w = 85, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1771, .adv_w = 85, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1781, .adv_w = 85, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1791, .adv_w = 40, .box_w = 3, .box_h = 8, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 1797, .adv_w = 40, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1803, .adv_w = 40, .box_w = 4, .box_h = 8, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 1811, .adv_w = 40, .box_w = 4, .box_h = 8, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 1819, .adv_w = 94, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1834, .adv_w = 88, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1846, .adv_w = 91, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1858, .adv_w = 91, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1870, .adv_w = 91, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1882, .adv_w = 91, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1896, .adv_w = 91, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1908, .adv_w = 91, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1917, .adv_w = 91, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1929, .adv_w = 88, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1939, .adv_w = 88, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1949, .adv_w = 88, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1959, .adv_w = 88, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1969, .adv_w = 76, .box_w = 5, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1982, .adv_w = 92, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1997, .adv_w = 76, .box_w = 5, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 2010, .adv_w = 153, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2028, .adv_w = 145, .box_w = 9, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2042, .adv_w = 82, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2042, .adv_w = 163, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2042, .adv_w = 82, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2042, .adv_w = 163, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2042, .adv_w = 54, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2042, .adv_w = 41, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2042, .adv_w = 27, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2042, .adv_w = 90, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2042, .adv_w = 44, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2042, .adv_w = 33, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2042, .adv_w = 16, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2042, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2042, .adv_w = 44, .box_w = 3, .box_h = 1, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 2043, .adv_w = 44, .box_w = 3, .box_h = 1, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 2044, .adv_w = 105, .box_w = 6, .box_h = 1, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 2046, .adv_w = 125, .box_w = 8, .box_h = 1, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 2048, .adv_w = 125, .box_w = 8, .box_h = 1, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 2050, .adv_w = 73, .box_w = 5, .box_h = 2, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 2053, .adv_w = 32, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 6},
    {.bitmap_index = 2055, .adv_w = 32, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 2057, .adv_w = 32, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 2059, .adv_w = 32, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 2061, .adv_w = 57, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 6},
    {.bitmap_index = 2064, .adv_w = 57, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 2067, .adv_w = 55, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 2070, .adv_w = 88, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2082, .adv_w = 91, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 2097, .adv_w = 54, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 2100, .adv_w = 75, .box_w = 4, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2102, .adv_w = 107, .box_w = 6, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2105, .adv_w = 19, .box_w = 2, .box_h = 1, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 2106, .adv_w = 153, .box_w = 10, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2126, .adv_w = 28, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 2128, .adv_w = 51, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 2131, .adv_w = 48, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 2135, .adv_w = 48, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 2139, .adv_w = 82, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2149, .adv_w = 73, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_2[] = {
    0x0, 0x1, 0x1eae, 0x1eaf, 0x1eb0, 0x1eb1, 0x1eb2, 0x1eb3,
    0x1eb4, 0x1eb5, 0x1eb6, 0x1eb7, 0x1eb8, 0x1eb9, 0x1ebe, 0x1ebf,
    0x1ec1, 0x1ec2, 0x1ec3, 0x1ec5, 0x1ec6, 0x1ec7, 0x1ec8, 0x1ec9,
    0x1eca, 0x1ecb, 0x1ecc, 0x1ece, 0x1ecf, 0x1ed0, 0x1ed3, 0x1ed4,
    0x1ed5, 0x1ede, 0x1ee0, 0x1ee1, 0x1ee7, 0x1ee8, 0x1eea, 0x1ef2
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 160, .range_length = 96, .glyph_id_start = 96,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 338, .range_length = 7923, .glyph_id_start = 192,
        .unicode_list = unicode_list_2, .glyph_id_ofs_list = NULL, .list_length = 40, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};

/*-----------------
 *    KERNING
 *----------------*/


/*Map glyph_ids to kern left classes*/
static const uint8_t kern_left_class_mapping[] =
{
    0, 1, 0, 2, 0, 0, 0, 0,
    2, 3, 0, 0, 0, 4, 0, 4,
    5, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 6, 7, 8, 9, 10, 11,
    0, 12, 12, 13, 14, 15, 12, 12,
    9, 16, 17, 18, 0, 19, 13, 20,
    21, 22, 23, 24, 25, 0, 0, 0,
    0, 0, 26, 27, 28, 0, 29, 30,
    0, 31, 0, 0, 32, 0, 31, 31,
    33, 27, 0, 34, 0, 35, 0, 36,
    37, 38, 36, 39, 40, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    6, 6, 6, 6, 6, 6, 0, 8,
    10, 10, 10, 10, 12, 12, 12, 12,
    9, 12, 9, 9, 9, 9, 9, 0,
    0, 13, 13, 13, 13, 23, 0, 0,
    26, 26, 26, 26, 26, 26, 0, 28,
    29, 29, 29, 29, 0, 0, 0, 0,
    0, 31, 33, 33, 33, 33, 33, 0,
    0, 0, 0, 0, 0, 36, 27, 36,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 2, 41, 4, 2,
    2, 2, 4, 0, 0, 0, 4, 4,
    0, 0, 2, 2, 0, 0, 0, 0
};

/*Map glyph_ids to kern right classes*/
static const uint8_t kern_right_class_mapping[] =
{
    0, 1, 0, 2, 0, 0, 0, 3,
    2, 0, 4, 5, 0, 6, 7, 6,
    8, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    9, 0, 10, 0, 11, 0, 0, 0,
    11, 0, 0, 12, 0, 0, 0, 0,
    11, 0, 11, 0, 13, 14, 15, 16,
    17, 18, 19, 20, 0, 0, 21, 0,
    0, 0, 22, 0, 23, 23, 23, 24,
    23, 25, 0, 0, 25, 25, 26, 26,
    27, 26, 23, 28, 29, 30, 31, 32,
    33, 34, 32, 35, 0, 0, 36, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 37, 0, 7, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 38, 0, 0, 0, 0,
    10, 10, 10, 10, 10, 10, 39, 11,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 11, 11, 11, 11, 11, 0,
    11, 15, 15, 15, 15, 19, 0, 0,
    22, 22, 22, 22, 22, 22, 40, 23,
    23, 23, 23, 23, 0, 0, 0, 0,
    0, 26, 27, 27, 27, 27, 27, 0,
    41, 31, 31, 31, 31, 32, 0, 32,
    11, 23, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    7, 7, 7, 0, 2, 42, 6, 2,
    2, 2, 6, 0, 0, 43, 6, 6,
    0, 0, 2, 2, 0, 0, 0, 0
};

/*Kern values between classes*/
static const int8_t kern_class_values[] =
{
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -3, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -8, 0, 0, 0,
    0, 0, 0, 0, -9, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -4, -5, 0, 0, -2, -5, 0, -6,
    0, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 0, 0, -8, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 2, 1, 0,
    2, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -13, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -13, 0, 0, 0, 0, 0,
    0, 0, 0, -17, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -9, 0, 0, 0, 0, 0, 0, -5,
    0, -1, 0, 0, -10, -1, -7, -5,
    0, -7, 0, 0, 0, 0, 0, 0,
    0, -1, 0, 0, -1, -1, -4, -3,
    0, 1, 0, 0, 0, 0, 0, 0,
    -9, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -2,
    0, -2, 0, 0, -4, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -2, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -2, 0, 0, 0, 0, 0,
    0, -1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -8, 0, 0,
    0, -2, 0, 0, 0, -2, 0, -2,
    0, -2, -3, -2, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -3, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    2, 0, 0, 0, 0, 0, 0, 0,
    0, -1, -1, 0, 0, -1, 0, 0,
    0, -1, -2, -2, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -18, 0, 0, 0, -13,
    0, -21, 0, 2, 0, 0, 0, 0,
    0, 0, 0, -3, -2, 0, 0, 0,
    -2, -2, 0, 0, -2, -2, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 0, 0, 0, -2, 0,
    0, 0, 1, -2, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -2, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -5, 0, 0,
    0, -2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -2, 0, 0,
    -2, -2, 0, 0, 0, -2, -3, -5,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -26, 0, 0, 0, 0,
    0, 0, 0, 1, -5, 0, 0, -21,
    -4, -14, -11, 0, -19, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -3, -10, -7, 0, 0, 0, 0, 0,
    0, 0, 0, -26, 0, 0, 0, 0,
    0, 0, -25, 0, 0, 0, -11, 0,
    -16, 0, 0, 0, 0, 0, -2, 0,
    -2, 0, -1, -1, 0, 0, 0, -1,
    0, 0, 1, 0, 1, 0, 0, 0,
    0, 0, 0, -8, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -3, 0, -2,
    -2, 0, -3, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -6, 0, -1, 0, 0, -4, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -3, 0,
    0, 0, 0, -17, -18, 0, 0, -6,
    -2, -19, -1, 1, 0, 1, 1, 0,
    1, 0, 0, -9, -8, 0, 0, -9,
    -8, -6, -9, 0, -7, -6, -4, -6,
    -5, 0, -26, -17, -14, -9, -7, 0,
    0, 0, 0, 0, 2, 0, -18, -3,
    0, 0, -6, -1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, -4, -3,
    0, 0, 0, -4, -2, 0, 0, -2,
    -1, 0, 0, 0, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1,
    0, -10, -5, 0, 0, -3, 0, 0,
    0, 1, 0, 0, 0, 0, 0, 0,
    1, -3, -2, 0, 0, 0, -2, -2,
    0, 0, -1, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -4, 0, 0,
    0, -2, 0, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 0, -2, 0, 0,
    0, -2, 0, 0, 0, -2, -2, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -2, 2, -4, -16,
    -4, 0, 0, -7, -2, -7, -1, 1,
    -7, 1, 1, 1, 1, 0, 1, -6,
    -5, -2, 0, -3, -5, -3, -5, -2,
    -3, -2, 0, -2, -2, 1, -6, -4,
    -7, -5, -5, 0, -4, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, -2,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -2, 0, 0, 0, -2,
    0, 0, 0, -1, -2, -2, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -1, 0, 0, -1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -5, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -5, 0, 0, -2,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -1, 0, -1,
    -1, 0, 0, 0, 0, 0, 0, -2,
    0, 0, -1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -1, 0, 0, -1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -6, 0, 0,
    1, 0, 2, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 0, -2, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 0, 0, 0, 0, 0,
    1, 0, 0, -8, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -6, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -2, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -11, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -1,
    0, -2, -1, 0, 0, 0, 0, 0,
    0, -6, 0, 0, 1, 0, 0, 0,
    -10, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -3, -1, 1, 0, 0, -2, 0, 0,
    4, 0, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -2, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 0, 0, 0, -8, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -1, -1,
    1, 0, 0, -1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 0, 0, 0, 0, 0,
    0, -10, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -2, 0, 0,
    0, -2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -1, 0, 0, 0, -1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -2, 0, 0, -2, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -8, 0, 0, 0, 0, 0, 0,
    0, -9, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -4, -5, 0,
    3, -2, -10, 0, -9, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0,
    0, -8, 0
};


/*Collect the kern class' data in one place*/
static const lv_font_fmt_txt_kern_classes_t kern_classes =
{
    .class_pair_values   = kern_class_values,
    .left_class_mapping  = kern_left_class_mapping,
    .right_class_mapping = kern_right_class_mapping,
    .left_class_cnt      = 41,
    .right_class_cnt     = 43,
};

/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = &kern_classes,
    .kern_scale = 16,
    .cmap_num = 3,
    .bpp = 2,
    .kern_classes = 1,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t aurora_font_10 = {
#else
lv_font_t aurora_font_10 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 14,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 0,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if AURORA_FONT_10*/

