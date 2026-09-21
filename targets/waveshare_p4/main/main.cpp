#include "bsp/esp-bsp.h"
#include "driver/gpio.h"
#include "board_config.h"
#include "ui.h"
#include "version.h"
#include "security_memory.h"
#include "aurora_log.h"
#if defined(AURORA_P4_KDF_BENCHMARK)
#include "p4_kdf_benchmark.h"
#endif
#if defined(AURORA_P4_ENTROPY_HEALTH)
#include "p4_entropy_health.h"
#endif
#if defined(AURORA_P4_STACK_HEALTH)
#include "p4_stack_health.h"
#endif

namespace { AuroraUI ui; }

extern "C" void app_main() {
  // First application action on every boot, before GPIO/display initialization.
  // The ROM and ESP-IDF have necessarily initialized the runtime before this.
  auroraSecuritySetEmergencyWipe([] { ui.emergencyWipeSecrets(); });
  ui.emergencyWipeSecrets();

  // The separate C6 must stay reset; not starting a Wi-Fi driver on P4 is
  // insufficient to disable firmware already installed on the radio module.
  ESP_ERROR_CHECK(gpio_set_level(static_cast<gpio_num_t>(AURORA_RADIO_RESET_PIN), 0));
  ESP_ERROR_CHECK(gpio_set_direction(static_cast<gpio_num_t>(AURORA_RADIO_RESET_PIN), GPIO_MODE_OUTPUT));
  ESP_ERROR_CHECK(gpio_hold_en(static_cast<gpio_num_t>(AURORA_RADIO_RESET_PIN)));
  ESP_ERROR_CHECK(gpio_set_level(BSP_POWER_AMP_IO, 0));
  ESP_ERROR_CHECK(gpio_set_direction(BSP_POWER_AMP_IO, GPIO_MODE_OUTPUT));

  // Unconditionally erase and verify owned, allocatable RAM before any display
  // or secret input. Do not try to recognize seeds in residual data. Never
  // mount or modify the user's SD, or overwrite live SDK allocations.
  size_t internalCleaned = 0, externalCleaned = 0;
  if (!auroraStartupMemoryScrub(&internalCleaned, &externalCleaned))
    auroraSecurityPanic();
  AURORA_DIAG("AURORA: startup scrub verified %u internal / %u external bytes\n",
         (unsigned)internalCleaned, (unsigned)externalCleaned);

#if defined(AURORA_P4_KDF_BENCHMARK)
  // Isolated F51-07 image: public constants only, no UI, wallet or microSD.
  auroraRunP4KdfBenchmark();
#elif defined(AURORA_P4_ENTROPY_HEALTH)
  // Isolated F51-05 image: aggregate health counters on screen only. No SD,
  // wallet, serial report, raw random value or conditioned digest is exported.
  auroraRunP4EntropyHealth();
#endif

  lv_display_t *display = bsp_display_start();
  if (!display || lv_display_get_horizontal_resolution(display) != 480 ||
      lv_display_get_vertical_resolution(display) != 800) {
    AURORA_DIAG("AURORA P4: unexpected display profile; startup stopped\n");
    return;
  }
  // The LVGL task has just started and can already own this mutex. In this
  // adapter, zero means a non-blocking try, not an infinite wait.
  if (bsp_display_lock(5000) != ESP_OK) {
    AURORA_DIAG("AURORA P4: display startup lock timed out; startup stopped\n");
    return;
  }
#if defined(AURORA_P4_STACK_HEALTH)
  auroraStackHealthBegin(xTaskGetCurrentTaskHandle());
#endif
  ui.begin();
  // The adapter's framebuffer ISR wakes the LVGL worker, so render from its
  // timer context. Holding the LVGL lock in app_main alone is not sufficient.
  lv_timer_t *refreshTimer = lv_timer_create([](lv_timer_t *timer) {
    static_cast<AuroraUI *>(lv_timer_get_user_data(timer))->refreshDisplayAfterClear();
  }, 20, &ui);
  ESP_ERROR_CHECK(refreshTimer ? ESP_OK : ESP_ERR_NO_MEM);
  bsp_display_unlock();
  bsp_display_backlight_on();
  AURORA_DIAG("AURORA %s: interface P4 480x800 initialisee\n", AURORA_FIRMWARE_VERSION);
  for (;;) {
    if (bsp_display_lock(50) == ESP_OK) {
      ui.tick();
      bsp_display_unlock();
    }
#if defined(AURORA_P4_STACK_HEALTH)
    auroraStackHealthSample();
#endif
    delay(5);
  }
}
