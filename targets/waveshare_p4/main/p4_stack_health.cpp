#if defined(AURORA_P4_STACK_HEALTH)

#include "p4_stack_health.h"

#include <algorithm>
#include <climits>
#include "sdkconfig.h"
#include "esp_lv_adapter.h"

namespace {
constexpr uint32_t MAIN_STACK_BYTES = CONFIG_ESP_MAIN_TASK_STACK_SIZE;
constexpr uint32_t LVGL_STACK_BYTES = ESP_LV_ADAPTER_DEFAULT_STACK_SIZE;
constexpr uint32_t MAIN_THRESHOLD_BYTES = MAIN_STACK_BYTES / 4;
constexpr uint32_t LVGL_THRESHOLD_BYTES = LVGL_STACK_BYTES / 4;

portMUX_TYPE stateLock = portMUX_INITIALIZER_UNLOCKED;
TaskHandle_t mainTaskHandle = nullptr;
TaskHandle_t lvglTaskHandle = nullptr;
uint32_t mainMinimum = UINT32_MAX;
uint32_t lvglMinimum = UINT32_MAX;
uint32_t samples = 0;
}

void auroraStackHealthBegin(TaskHandle_t mainTask) {
  portENTER_CRITICAL(&stateLock);
  mainTaskHandle = mainTask;
  mainMinimum = UINT32_MAX;
  lvglMinimum = UINT32_MAX;
  samples = 0;
  portEXIT_CRITICAL(&stateLock);
  auroraStackHealthSample();
}

void auroraStackHealthSample() {
  TaskHandle_t mainHandle = nullptr;
  TaskHandle_t lvglHandle = nullptr;
  portENTER_CRITICAL(&stateLock);
  mainHandle = mainTaskHandle;
  lvglHandle = lvglTaskHandle;
  portEXIT_CRITICAL(&stateLock);

  if (!lvglHandle) lvglHandle = xTaskGetHandle("lvgl");
  if (!mainHandle) return;

  // ESP-IDF's FreeRTOS port reports high-water marks in bytes.
  const uint32_t mainFree = static_cast<uint32_t>(uxTaskGetStackHighWaterMark(mainHandle));
  const uint32_t lvglFree = lvglHandle
      ? static_cast<uint32_t>(uxTaskGetStackHighWaterMark(lvglHandle)) : 0;

  portENTER_CRITICAL(&stateLock);
  if (lvglHandle) lvglTaskHandle = lvglHandle;
  mainMinimum = std::min(mainMinimum,mainFree);
  if (lvglHandle) lvglMinimum = std::min(lvglMinimum,lvglFree);
  ++samples;
  portEXIT_CRITICAL(&stateLock);
}

AuroraStackHealthSnapshot auroraStackHealthSnapshot() {
  AuroraStackHealthSnapshot result{};
  portENTER_CRITICAL(&stateLock);
  result.mainFreeBytes = mainMinimum==UINT32_MAX?0:mainMinimum;
  result.lvglFreeBytes = lvglMinimum==UINT32_MAX?0:lvglMinimum;
  result.sampleCount = samples;
  result.lvglTaskFound = lvglTaskHandle!=nullptr;
  portEXIT_CRITICAL(&stateLock);

  result.mainStackBytes = MAIN_STACK_BYTES;
  result.lvglStackBytes = LVGL_STACK_BYTES;
  result.mainThresholdBytes = MAIN_THRESHOLD_BYTES;
  result.lvglThresholdBytes = LVGL_THRESHOLD_BYTES;
  result.pass = result.lvglTaskFound && result.sampleCount>0 &&
                result.mainFreeBytes>=MAIN_THRESHOLD_BYTES &&
                result.lvglFreeBytes>=LVGL_THRESHOLD_BYTES;
  return result;
}

#endif
