#include "bsp/esp-bsp.h"
#include "driver/gpio.h"
#include "board_config.h"
#include "ui.h"
#include "version.h"

namespace { AuroraUI ui; }

extern "C" void app_main() {
  // The separate C6 must stay reset; not starting a Wi-Fi driver on P4 is
  // insufficient to disable firmware already installed on the radio module.
  ESP_ERROR_CHECK(gpio_set_level(static_cast<gpio_num_t>(AURORA_RADIO_RESET_PIN), 0));
  ESP_ERROR_CHECK(gpio_set_direction(static_cast<gpio_num_t>(AURORA_RADIO_RESET_PIN), GPIO_MODE_OUTPUT));
  ESP_ERROR_CHECK(gpio_hold_en(static_cast<gpio_num_t>(AURORA_RADIO_RESET_PIN)));
  ESP_ERROR_CHECK(gpio_set_level(BSP_POWER_AMP_IO, 0));
  ESP_ERROR_CHECK(gpio_set_direction(BSP_POWER_AMP_IO, GPIO_MODE_OUTPUT));

  lv_display_t *display = bsp_display_start();
  if (!display || lv_display_get_horizontal_resolution(display) != 480 ||
      lv_display_get_vertical_resolution(display) != 800) {
    printf("AURORA P4: unexpected display profile; startup stopped\n");
    return;
  }
  // The LVGL task has just started and can already own this mutex. In this
  // adapter, zero means a non-blocking try, not an infinite wait.
  if (bsp_display_lock(5000) != ESP_OK) {
    printf("AURORA P4: display startup lock timed out; startup stopped\n");
    return;
  }
  ui.begin();
  bsp_display_unlock();
  bsp_display_backlight_on();
  printf("AURORA %s: interface P4 480x800 initialisee\n", AURORA_FIRMWARE_VERSION);
  for (;;) {
    if (bsp_display_lock(50) == ESP_OK) {
      ui.tick();
      bsp_display_unlock();
    }
    delay(5);
  }
}
