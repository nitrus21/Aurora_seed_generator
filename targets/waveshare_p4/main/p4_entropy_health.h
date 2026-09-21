#pragma once

#if defined(AURORA_P4_ENTROPY_HEALTH)

#include <inttypes.h>
#include <stdio.h>
#include "bsp/esp-bsp.h"
#include "entropy_health.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hardware_rng.h"
#include "security_memory.h"

namespace AuroraP4EntropyDiagnostic {
struct View {
  lv_obj_t *status = nullptr;
  lv_obj_t *metrics = nullptr;
};

inline void setText(lv_obj_t *label, const char *text, lv_color_t color) {
  lv_label_set_text(label, text);
  lv_obj_set_style_text_color(label, color, 0);
}

inline View startDisplay() {
  lv_display_t *display = bsp_display_start();
  if (!display || lv_display_get_horizontal_resolution(display) != 480 ||
      lv_display_get_vertical_resolution(display) != 800 ||
      bsp_display_lock(5000) != ESP_OK) auroraSecurityPanic();

  lv_obj_t *screen = lv_screen_active();
  lv_obj_set_style_bg_color(screen, lv_color_hex(0x080A0D), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_t *title = lv_label_create(screen);
  setText(title, "AURORA ENTROPY HEALTH", lv_color_hex(0xF7931A));
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 48);

  lv_obj_t *scope = lv_label_create(screen);
  lv_label_set_text(scope,
      "P4 rev1 - sortie RNG conditionnee\n"
      "Aucune valeur brute affichee ou stockee\n"
      "Ce test n'est pas une certification");
  lv_obj_set_style_text_align(scope, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(scope, lv_color_hex(0xD1D5DB), 0);
  lv_obj_align(scope, LV_ALIGN_TOP_MID, 0, 105);

  View view;
  view.status = lv_label_create(screen);
  lv_obj_set_style_text_align(view.status, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(view.status, LV_ALIGN_CENTER, 0, -70);
  view.metrics = lv_label_create(screen);
  lv_obj_set_style_text_align(view.metrics, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_align(view.metrics, LV_ALIGN_CENTER, 0, 45);
  setText(view.status, "Preparation...", lv_color_hex(0xFBBF24));
  lv_label_set_text(view.metrics, "");
  bsp_display_unlock();
  bsp_display_backlight_on();
  return view;
}

inline void update(const View &view, unsigned campaign,
                   const AuroraEntropyHealthSummary &summary, bool complete, bool passed) {
  if (bsp_display_lock(5000) != ESP_OK) auroraSecurityPanic();
  char status[64];
  if (complete) {
    snprintf(status, sizeof(status), "Serie %u / 3 : %s", campaign, passed ? "PASS" : "ECHEC");
  } else {
    const unsigned percent = static_cast<unsigned>(
        (static_cast<uint64_t>(summary.words) * 100U) / AuroraEntropyHealth::REQUIRED_WORDS);
    snprintf(status, sizeof(status), "Serie %u / 3 : %u %%", campaign, percent);
  }
  setText(view.status, status, complete ? (passed ? lv_color_hex(0x22C55E) : lv_color_hex(0xEF4444))
                                       : lv_color_hex(0xFBBF24));
  const uint64_t bits = static_cast<uint64_t>(summary.words) * 32U;
  lv_label_set_text_fmt(view.metrics,
      "Mots analyses : %" PRIu32 "\n"
      "Bits a 1 : %" PRIu32 " / 10000\n"
      "Transitions : %" PRIu32 " / 10000\n"
      "Plus longue serie : %" PRIu32 "\n"
      "Mots consecutifs egaux : %" PRIu32,
      summary.words,
      AuroraEntropyHealth::ratioPer10000(summary.ones, bits),
      AuroraEntropyHealth::ratioPer10000(summary.transitions, bits > 0 ? bits - 1U : 0),
      summary.longestBitRun, summary.adjacentDuplicateWords);
  bsp_display_unlock();
}
}  // namespace AuroraP4EntropyDiagnostic

[[noreturn]] inline void auroraRunP4EntropyHealth() {
  using namespace AuroraP4EntropyDiagnostic;
  const View view = startDisplay();
  bool allPassed = true;
  for (unsigned campaign = 1; campaign <= 3; ++campaign) {
    AuroraEntropyHealth health;
    hardwareRngEnable();
    for (uint32_t i = 0; i < AuroraEntropyHealth::REQUIRED_WORDS; ++i) {
      health.add(esp_random());
      if ((i + 1U) % 4096U == 0) update(view, campaign, health.summary(), false, false);
    }
    hardwareRngDisable();
    const bool passed = health.passed();
    allPassed = allPassed && passed;
    update(view, campaign, health.summary(), true, passed);
    auroraSecureZero(&health, sizeof(health));
    if (!passed) break;
    vTaskDelay(pdMS_TO_TICKS(1500));
  }
  if (bsp_display_lock(5000) != ESP_OK) auroraSecurityPanic();
  setText(view.status, allPassed ? "RESULTAT FINAL : PASS" : "RESULTAT FINAL : ECHEC",
          allPassed ? lv_color_hex(0x22C55E) : lv_color_hex(0xEF4444));
  bsp_display_unlock();
  for (;;) vTaskDelay(pdMS_TO_TICKS(1000));
}

#endif
