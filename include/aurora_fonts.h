#pragma once
#include <lvgl.h>
#ifdef __cplusplus
extern "C" {
#endif

LV_FONT_DECLARE(aurora_font_10);
LV_FONT_DECLARE(aurora_font_12);
LV_FONT_DECLARE(aurora_font_14);
LV_FONT_DECLARE(aurora_font_16);
LV_FONT_DECLARE(aurora_font_20);
#if defined(AURORA_BOARD_P4)
LV_FONT_DECLARE(aurora_font_18);
LV_FONT_DECLARE(aurora_font_24);
LV_FONT_DECLARE(aurora_font_30);
LV_FONT_DECLARE(aurora_font_36);
LV_FONT_DECLARE(aurora_font_72);
#endif
#ifdef __cplusplus
}
#endif
