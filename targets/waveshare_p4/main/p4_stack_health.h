#pragma once

#if defined(AURORA_P4_STACK_HEALTH)

#include <cstdint>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

struct AuroraStackHealthSnapshot {
  uint32_t mainFreeBytes;
  uint32_t lvglFreeBytes;
  uint32_t mainStackBytes;
  uint32_t lvglStackBytes;
  uint32_t mainThresholdBytes;
  uint32_t lvglThresholdBytes;
  uint32_t sampleCount;
  bool lvglTaskFound;
  bool pass;
};

void auroraStackHealthBegin(TaskHandle_t mainTask);
void auroraStackHealthSample();
AuroraStackHealthSnapshot auroraStackHealthSnapshot();

#endif
