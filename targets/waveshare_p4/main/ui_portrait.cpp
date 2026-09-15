#include "ui.h"
#include "sensors.h"
#include "esp_heap_caps.h"

namespace {
lv_color_t rgb(uint32_t value) { return lv_color_hex(value); }
lv_obj_t *text(lv_obj_t *parent, const char *value, int x, int y,
               const lv_font_t *font = &aurora_font_18) {
  lv_obj_t *object = lv_label_create(parent);
  lv_label_set_text(object, value); lv_obj_set_style_text_font(object, font, 0);
  lv_obj_set_pos(object, x, y); lv_obj_set_style_text_color(object, rgb(0xF3F4F6), 0);
  return object;
}
const char *stateName(AuroraSensors::State state) {
  switch (state) {
    case AuroraSensors::State::Probing: return "détection...";
    case AuroraSensors::State::Active: return "actif";
    case AuroraSensors::State::Absent: return "absent";
    case AuroraSensors::State::Failed: return "indisponible";
    default: return "arrêté";
  }
}
}

void AuroraUI::refreshDisplayAfterClear() {
  if (!displayRefreshPending_ || !root_) return;
  displayRefreshPending_ = false;
  // After screen replacement, run from a timer on the LVGL worker, outside
  // input/render callbacks; the framebuffer ISR notifies this worker task.
  // The pinned P4 BSP uses TRIPLE_PARTIAL with three FIFO framebuffers.
  // Three independent full redraws replace old pixels via the normal DMA pipeline.
  // Recheck this count if the BSP display mode or buffer count ever changes.
  for (unsigned frame = 0; frame < 3; ++frame) {
    lv_obj_invalidate(lv_obj_get_screen(root_));
    lv_refr_now(lv_obj_get_display(root_));
  }
}

void AuroraUI::buildPortraitEntropy() {
  header("Collecte d'entropie", "2 / 7");
  secureZero(mixedEntropy_, sizeof(mixedEntropy_));
  entropyReadyPending_ = entropyFailurePending_ = false;
  entropyCompleteDueMs_ = entropyPreviewUpdatedMs_ = sensorUiUpdated_ = 0;
  entropy_.begin();
  text(root_, "Bougez votre doigt dans le cadre", 24, 122, &aurora_font_18);
  lv_obj_t *pad = lv_obj_create(root_);
  lv_obj_set_pos(pad, 24, 152); lv_obj_set_size(pad, 432, 188);
  lv_obj_remove_flag(pad, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(pad, rgb(0x16181D), 0);
  lv_obj_set_style_border_color(pad, rgb(0xF7931A), 0);
  lv_obj_set_style_border_width(pad, 2, 0);
  // One contribution per real LVGL input read, never repeated cached polling.
  lv_obj_add_event_cb(pad, [](lv_event_t *event) {
    lv_indev_t *input = lv_event_get_indev(event);
    if (!input) return;
    lv_point_t point{};
    lv_indev_get_point(input, &point);
    static_cast<AuroraUI *>(lv_event_get_user_data(event))->onTouchSample(point.x, point.y, 0);
  }, LV_EVENT_PRESSING, this);
  lv_obj_t *hint = text(pad, "Mouvement + temps + RNG matériel\nMicrophones / caméra si disponibles", 0, 0, &aurora_font_18);
  lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0); lv_obj_center(hint);

  microphoneStatus_ = text(root_, "Microphones : détection...", 24, 364, &aurora_font_14);
  microphoneLevel_ = lv_bar_create(root_);
  lv_obj_set_pos(microphoneLevel_, 24, 412); lv_obj_set_size(microphoneLevel_, 226, 10);
  lv_bar_set_range(microphoneLevel_, 0, 100);
  lv_obj_set_style_bg_color(microphoneLevel_, rgb(0xF7931A), LV_PART_INDICATOR);
  text(root_, "Sources complémentaires\nAucun enregistrement\nAucune transmission", 24, 430, &aurora_font_14);
  text(root_, "Le compteur mesure la collecte,\npas des bits d'entropie certifiés.", 24, 505, &aurora_font_14);
  cameraStatus_ = text(root_, "Caméra : détection...", 266, 349, &aurora_font_14);
  lv_obj_set_width(cameraStatus_, 190); lv_label_set_long_mode(cameraStatus_, LV_LABEL_LONG_WRAP);
  cameraPixels_ = static_cast<uint16_t *>(heap_caps_calloc(
      AuroraSensors::PREVIEW_WIDTH * AuroraSensors::PREVIEW_HEIGHT, sizeof(uint16_t), MALLOC_CAP_SPIRAM));
  if (cameraPixels_) {
    cameraImage_.header.magic = LV_IMAGE_HEADER_MAGIC;
    cameraImage_.header.cf = LV_COLOR_FORMAT_RGB565;
    cameraImage_.header.w = AuroraSensors::PREVIEW_WIDTH;
    cameraImage_.header.h = AuroraSensors::PREVIEW_HEIGHT;
    cameraImage_.header.stride = AuroraSensors::PREVIEW_WIDTH * sizeof(uint16_t);
    cameraImage_.data_size = AuroraSensors::PREVIEW_WIDTH * AuroraSensors::PREVIEW_HEIGHT * sizeof(uint16_t);
    cameraImage_.data = reinterpret_cast<const uint8_t *>(cameraPixels_);
    cameraPreview_ = lv_image_create(root_); lv_image_set_src(cameraPreview_, &cameraImage_);
    lv_obj_set_pos(cameraPreview_, 306, 392);
  }
  text(root_, "Aperçu cryptographique défilant", 24, 644, &aurora_font_18);
  strlcpy(entropyPreviewText_, "-------- -------- -------- --------", sizeof(entropyPreviewText_));
  entropyPreview_ = text(root_, "", 24, 670, &aurora_font_18);
  lv_label_set_text_static(entropyPreview_, entropyPreviewText_);
  // Reserve the full lower width for progress, below the camera view.
  entropyStatus_ = text(root_, "Collecte insuffisante - 0 %", 24, 700, &aurora_font_18);
  lv_obj_set_style_text_color(entropyStatus_, rgb(0xFF3B30), 0);
  lv_obj_set_width(entropyStatus_, 432);
  entropyBar_ = lv_bar_create(root_);
  lv_obj_set_pos(entropyBar_, 24, 737); lv_obj_set_size(entropyBar_, 432, 16);
  lv_bar_set_range(entropyBar_, 0, 100); lv_bar_set_value(entropyBar_, 0, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(entropyBar_, rgb(0x47110E), LV_PART_MAIN);
  lv_obj_set_style_bg_color(entropyBar_, rgb(0xFF3B30), LV_PART_INDICATOR);
  entropyCount_ = text(root_, "", 24, 768, &aurora_font_14);
  lv_label_set_text_fmt(entropyCount_, "0 / %u échantillons", TouchEntropy::REQUIRED_SAMPLES);
  if (!AuroraSensors::start()) entropyFailurePending_ = true;
}

void AuroraUI::updatePortraitSensors() {
  if (!microphoneStatus_ || millis() - sensorUiUpdated_ < 100) return;
  sensorUiUpdated_ = millis();
  const auto state = AuroraSensors::status();
  lv_label_set_text_fmt(microphoneStatus_, "Microphones : %s\n%lu blocs intégrés",
      stateName(state.microphone), static_cast<unsigned long>(entropy_.auxiliaryCount(TouchEntropy::Source::Microphone)));
  lv_bar_set_value(microphoneLevel_, state.level, LV_ANIM_OFF);
  lv_label_set_text_fmt(cameraStatus_, "Caméra : %s\n%lu images intégrées", stateName(state.camera),
      static_cast<unsigned long>(entropy_.auxiliaryCount(TouchEntropy::Source::Camera)));
  if (cameraPixels_ && cameraPreview_ && AuroraSensors::copyPreview(cameraPixels_,
      AuroraSensors::PREVIEW_WIDTH * AuroraSensors::PREVIEW_HEIGHT, cameraPreviewSequence_)) {
    lv_obj_invalidate(cameraPreview_);
  }
}
