#include <Arduino.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <lvgl.h>
#include <WiFi.h>
#include <esp_bt.h>
#include "board_config.h"
#include "ui.h"

namespace {
TFT_eSPI tft;
SPIClass touchSpi(VSPI);
XPT2046_Touchscreen touch(AURORA_TOUCH_CS, AURORA_TOUCH_IRQ);
AuroraUI ui;
lv_disp_draw_buf_t drawBuffer;
lv_color_t pixels[AURORA_SCREEN_WIDTH * 20];

int16_t mapClamp(int32_t v, int32_t inMin, int32_t inMax, int32_t outMax) {
  v = constrain(v, inMin, inMax);
  return (int16_t)((v - inMin) * outMax / (inMax - inMin));
}

void flushDisplay(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color) {
  uint32_t w = area->x2 - area->x1 + 1, h = area->y2 - area->y1 + 1;
  tft.startWrite(); tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors(reinterpret_cast<uint16_t *>(&color->full), w * h, true); tft.endWrite();
  lv_disp_flush_ready(drv);
}

void readTouch(lv_indev_drv_t *, lv_indev_data_t *data) {
  if (!touch.touched()) { data->state = LV_INDEV_STATE_REL; return; }
  TS_Point p = touch.getPoint(); int16_t x, y;
#if AURORA_TOUCH_SWAP_XY
  x = mapClamp(p.y, AURORA_TOUCH_MIN_Y, AURORA_TOUCH_MAX_Y, AURORA_SCREEN_WIDTH - 1);
  y = mapClamp(p.x, AURORA_TOUCH_MIN_X, AURORA_TOUCH_MAX_X, AURORA_SCREEN_HEIGHT - 1);
#else
  x = mapClamp(p.x, AURORA_TOUCH_MIN_X, AURORA_TOUCH_MAX_X, AURORA_SCREEN_WIDTH - 1);
  y = mapClamp(p.y, AURORA_TOUCH_MIN_Y, AURORA_TOUCH_MAX_Y, AURORA_SCREEN_HEIGHT - 1);
#endif
#if AURORA_TOUCH_INVERT_X
  x = AURORA_SCREEN_WIDTH - 1 - x;
#endif
#if AURORA_TOUCH_INVERT_Y
  y = AURORA_SCREEN_HEIGHT - 1 - y;
#endif
  data->point.x = x; data->point.y = y; data->state = LV_INDEV_STATE_PR;
  ui.onTouchSample(x, y, p.z);
}
}

void setup() {
  Serial.begin(115200);
  // Stop radios without erasing or writing Wi-Fi credentials in NVS.
  WiFi.disconnect(false, false); WiFi.mode(WIFI_OFF);
  btStop(); esp_bt_controller_disable();
  pinMode(AURORA_BACKLIGHT_PIN, OUTPUT); digitalWrite(AURORA_BACKLIGHT_PIN, LOW);
  tft.begin(); tft.setRotation(AURORA_TFT_ROTATION); tft.fillScreen(TFT_BLACK);
  touchSpi.begin(AURORA_TOUCH_CLK, AURORA_TOUCH_MISO, AURORA_TOUCH_MOSI, AURORA_TOUCH_CS);
  touch.begin(touchSpi); touch.setRotation(0);
  lv_init(); lv_disp_draw_buf_init(&drawBuffer, pixels, nullptr, AURORA_SCREEN_WIDTH * 20);
  static lv_disp_drv_t displayDriver; lv_disp_drv_init(&displayDriver);
  displayDriver.hor_res=AURORA_SCREEN_WIDTH; displayDriver.ver_res=AURORA_SCREEN_HEIGHT;
  displayDriver.flush_cb=flushDisplay; displayDriver.draw_buf=&drawBuffer; lv_disp_drv_register(&displayDriver);
  static lv_indev_drv_t inputDriver; lv_indev_drv_init(&inputDriver); inputDriver.type=LV_INDEV_TYPE_POINTER;
  inputDriver.read_cb=readTouch; lv_indev_drv_register(&inputDriver);
  ui.begin(); digitalWrite(AURORA_BACKLIGHT_PIN, HIGH);
}

void loop() { lv_timer_handler(); ui.tick(); delay(5); }
