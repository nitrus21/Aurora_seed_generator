#pragma once
#include "entropy.h"

namespace AuroraSensors {
enum class State : uint8_t { Off, Probing, Active, Absent, Failed };
struct Status { State microphone; State camera; uint8_t level; uint32_t audioBlocks; uint32_t frames; };
constexpr unsigned PREVIEW_WIDTH = 150;
constexpr unsigned PREVIEW_HEIGHT = 240;
// All lifecycle calls and drain() belong to the UI task. Drivers never see
// wallet data and communicate only through bounded queues / copied previews.
bool start();
void requestStop();
bool stopped();
Status status();
void drain(TouchEntropy &collector);
bool copyPreview(uint16_t *rgb565, size_t pixels, uint32_t &sequence);
}
