#pragma once
#include <lvgl.h>
#include "aurora_fonts.h"

// Native widget geometry, not a stretched 320x240 framebuffer. The common
// workflow uses logical coordinates; P4 renders text and controls at 480x800.
namespace AuroraLayout {
constexpr lv_color_t color(uint8_t red, uint8_t green, uint8_t blue) {
#if LVGL_VERSION_MAJOR >= 9
  return {blue, green, red};
#else
  return LV_COLOR_MAKE(red, green, blue);
#endif
}
inline int x(int value) {
#if defined(AURORA_BOARD_P4)
  return value * 3 / 2;
#else
  return value;
#endif
}
inline int y(int value) {
#if defined(AURORA_BOARD_P4)
  return value * 10 / 3;
#else
  return value;
#endif
}
inline void pos(lv_obj_t *object, int left, int top) { lv_obj_set_pos(object, x(left), y(top)); }
inline void size(lv_obj_t *object, int width, int height) {
#if defined(AURORA_BOARD_P4)
  // Circles (step badges/spinners) keep their aspect ratio.
  lv_obj_set_size(object, x(width), width == height ? x(height) : y(height));
  if (lv_obj_check_type(object, &lv_keyboard_class)) {
    // LVGL's keyboard uses private Unicode symbols absent from the text font.
    lv_obj_set_style_text_font(object, &lv_font_montserrat_20, LV_PART_ITEMS);
  }
#else
  lv_obj_set_size(object, width, height);
#endif
}
inline void align(lv_obj_t *object, lv_align_t anchor, int dx, int dy) {
  lv_obj_align(object, anchor, x(dx), y(dy));
}
inline void font(lv_obj_t *object, const lv_font_t *value, lv_style_selector_t selector) {
#if defined(AURORA_BOARD_P4)
  if (value == &aurora_font_10) value = &aurora_font_14;
  else if (value == &aurora_font_12) value = &aurora_font_18;
  else if (value == &aurora_font_14) value = &aurora_font_20;
  else if (value == &aurora_font_16) value = &aurora_font_24;
  else if (value == &aurora_font_20) value = &aurora_font_30;
#endif
  lv_obj_set_style_text_font(object, value, selector);
}
inline lv_obj_t *spinner(lv_obj_t *parent, unsigned period, unsigned arc) {
#if LVGL_VERSION_MAJOR >= 9
  lv_obj_t *object = lv_spinner_create(parent);
  lv_spinner_set_anim_params(object, period, arc);
  return object;
#else
  return lv_spinner_create(parent, period, arc);
#endif
}
}
