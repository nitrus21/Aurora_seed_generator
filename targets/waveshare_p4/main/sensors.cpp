#include "sensors.h"
#include "bsp/esp-bsp.h"
#include "esp_codec_dev_defaults.h"
#include "esp_video_init.h"
#include "esp_video_device.h"
#include "esp_video_ioctl.h"
#include "esp_heap_caps.h"
#include "freertos/semphr.h"
#include <atomic>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace AuroraSensors {
namespace {
constexpr size_t PREVIEW_PIXELS = PREVIEW_WIDTH * PREVIEW_HEIGHT;
struct Packet { uint8_t data[1024]; size_t length; uint32_t sequence; };
Packet packets[2]{};
SemaphoreHandle_t mutex = nullptr;
uint16_t *preview = nullptr;
uint32_t previewSequence = 0;
std::atomic<bool> stopRequested{true}, audioAlive{false}, cameraAlive{false}, stopFailed{false};
std::atomic<State> audioState{State::Off}, cameraState{State::Off};
std::atomic<uint8_t> audioLevel{0};
std::atomic<uint32_t> audioCount{0}, cameraCount{0};

void publish(unsigned index, uint32_t sequence, const void *data, size_t size) {
  if (stopRequested || size > sizeof(packets[index].data)) return;
  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(20)) != pdTRUE) return;
  secureZero(&packets[index], sizeof(Packet));
  if (!stopRequested) {
    memcpy(packets[index].data, data, size);
    packets[index].length = size; packets[index].sequence = sequence;
  }
  xSemaphoreGive(mutex);
}

void audioTask(void *) {
  i2s_chan_handle_t rx = nullptr;
  const audio_codec_ctrl_if_t *control = nullptr;
  const audio_codec_data_if_t *data = nullptr;
  const audio_codec_if_t *codec = nullptr;
  esp_codec_dev_handle_t device = nullptr;
  int16_t samples[512]{};
  bool opened = false;
  do {
    if (stopRequested) break;
    // Codec control uses the 8-bit address; i2c_master_probe uses 7 bits.
    if (i2c_master_probe(bsp_i2c_get_handle(), ES7210_CODEC_DEFAULT_ADDR >> 1, 50) != ESP_OK) {
      audioState = State::Absent; break;
    }
    i2s_chan_config_t channel = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    if (i2s_new_channel(&channel, nullptr, &rx) != ESP_OK) break;
    i2s_std_config_t format{};
    format.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000);
    format.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
    format.gpio_cfg.mclk = BSP_I2S_MCLK;
    format.gpio_cfg.bclk = BSP_I2S_SCLK;
    format.gpio_cfg.ws = BSP_I2S_LCLK;
    format.gpio_cfg.dout = GPIO_NUM_NC;
    format.gpio_cfg.din = BSP_I2S_DSIN;
    if (i2s_channel_init_std_mode(rx, &format) != ESP_OK) break;
    audio_codec_i2c_cfg_t i2c{};
    i2c.port = BSP_I2C_NUM; i2c.addr = ES7210_CODEC_DEFAULT_ADDR; i2c.bus_handle = bsp_i2c_get_handle();
    control = audio_codec_new_i2c_ctrl(&i2c);
    if (!control) break;
    audio_codec_i2s_cfg_t i2s{}; i2s.port = I2S_NUM_0; i2s.rx_handle = rx;
    data = audio_codec_new_i2s_data(&i2s);
    if (!data) break;
    es7210_codec_cfg_t adc{}; adc.ctrl_if = control;
    adc.mic_selected = ES7210_SEL_MIC1 | ES7210_SEL_MIC2;
    codec = es7210_codec_new(&adc);
    if (!codec) break;
    esp_codec_dev_cfg_t dev{};
    dev.dev_type = ESP_CODEC_DEV_TYPE_IN; dev.codec_if = codec; dev.data_if = data;
    device = esp_codec_dev_new(&dev);
    if (!device) break;
    esp_codec_dev_sample_info_t info{};
    info.sample_rate = 16000; info.channel = 2; info.bits_per_sample = 16;
    if (esp_codec_dev_open(device, &info) != ESP_CODEC_DEV_OK) break;
    opened = true;
    if (esp_codec_dev_set_in_gain(device, 24.0f) != ESP_CODEC_DEV_OK) break;
    uint32_t sequence = 0, lastData = millis();
    while (!stopRequested) {
      size_t bytes = 0;
      const esp_err_t result = i2s_channel_read(rx, samples, sizeof(samples), &bytes, 50);
      if (result != ESP_OK && result != ESP_ERR_TIMEOUT) break;
      if (!bytes) { if (millis() - lastData > 1500) break; continue; }
      if (bytes > sizeof(samples) || bytes % sizeof(int16_t)) break;
      lastData = millis();
      uint32_t peak = 0;
      for (size_t i = 0; i < bytes / sizeof(int16_t); ++i) {
        const int32_t value = samples[i];
        const uint32_t amplitude = value < 0 ? -value : value;
        if (amplitude > peak) peak = amplitude;
      }
      audioLevel = static_cast<uint8_t>(peak * 100 / 32768);
      audioState = State::Active; audioCount = ++sequence;
      publish(0, sequence, samples, bytes);
      secureZero(samples, sizeof(samples));
    }
  } while (false);
  if (!stopRequested && audioState != State::Absent) audioState = State::Failed;
  // No BSP-owned always-on I2S handle: this task owns and closes every handle.
  if (device) {
    if (opened && esp_codec_dev_close(device) != ESP_CODEC_DEV_OK) stopFailed = true;
    esp_codec_dev_delete(device);
  }
  if (codec && audio_codec_delete_codec_if(codec) != ESP_CODEC_DEV_OK) stopFailed = true;
  if (data && audio_codec_delete_data_if(data) != ESP_CODEC_DEV_OK) stopFailed = true;
  if (control && audio_codec_delete_ctrl_if(control) != ESP_CODEC_DEV_OK) stopFailed = true;
  if (rx) {
    const esp_err_t disabled = i2s_channel_disable(rx);
    if (disabled != ESP_OK && disabled != ESP_ERR_INVALID_STATE) stopFailed = true;
    if (i2s_del_channel(rx) != ESP_OK) stopFailed = true;
  }
  secureZero(samples, sizeof(samples)); audioLevel = 0;
  audioAlive = false;
  vTaskDelete(nullptr);
}

void cameraTask(void *) {
  int fd = -1;
  bool initialized = false, streaming = false;
  void *buffers[2]{}; size_t lengths[2]{};
  uint8_t digest[32]{};
  uint16_t *small = static_cast<uint16_t *>(heap_caps_calloc(PREVIEW_PIXELS, sizeof(uint16_t), MALLOC_CAP_SPIRAM));
  do {
    if (stopRequested) break;
    // OV5647 SCCB address. Absence is normal on the non-C board.
    if (i2c_master_probe(bsp_i2c_get_handle(), 0x36, 50) != ESP_OK) {
      cameraState = State::Absent; break;
    }
    esp_video_init_csi_config_t csi{};
    csi.sccb_config.init_sccb = false;
    csi.sccb_config.i2c_handle = bsp_i2c_get_handle(); csi.sccb_config.freq = 400000;
    csi.reset_pin = GPIO_NUM_NC; csi.pwdn_pin = GPIO_NUM_NC;
    esp_video_init_config_t config{}; config.csi = &csi;
    initialized = true; // Also clean up a partially successful initialization.
    if (esp_video_init(&config) != ESP_OK) break;
    fd = ::open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, O_RDONLY);
    if (fd < 0) break;
    // Require a bounded dequeue: an unplugged/faulty camera cannot trap shutdown.
    struct timeval timeout{}; timeout.tv_usec = 100000;
    if (ioctl(fd, VIDIOC_S_DQBUF_TIMEOUT, &timeout) != 0) break;
    struct v4l2_format format{}; format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_G_FMT, &format) != 0) break;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;
    if (ioctl(fd, VIDIOC_S_FMT, &format) != 0 || ioctl(fd, VIDIOC_G_FMT, &format) != 0) break;
    const uint32_t width = format.fmt.pix.width, height = format.fmt.pix.height;
    const size_t stride = format.fmt.pix.bytesperline ? format.fmt.pix.bytesperline : width * 2;
    if (!width || width > 2592 || !height || height > 2592 || stride < width * 2 ||
        stride > 8192 || stride * height > 8 * 1024 * 1024 || format.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB565) break;
    struct v4l2_requestbuffers request{};
    request.count = 2; request.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; request.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &request) != 0 || request.count != 2) break;
    bool valid = true;
    for (unsigned i = 0; i < 2; ++i) {
      struct v4l2_buffer buffer{};
      buffer.index = i; buffer.type = request.type; buffer.memory = request.memory;
      if (ioctl(fd, VIDIOC_QUERYBUF, &buffer) != 0 || buffer.length < stride * height) { valid = false; break; }
      buffers[i] = mmap(nullptr, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buffer.m.offset);
      if (!buffers[i] || buffers[i] == MAP_FAILED) { buffers[i] = nullptr; valid = false; break; }
      lengths[i] = buffer.length;
      if (ioctl(fd, VIDIOC_QBUF, &buffer) != 0) { valid = false; break; }
    }
    if (!valid) break;
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) != 0) break;
    streaming = true;
    uint32_t sequence = 0, lastData = millis(), lastProcessed = 0;
    while (!stopRequested) {
      struct v4l2_buffer buffer{};
      buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; buffer.memory = V4L2_MEMORY_MMAP;
      if (ioctl(fd, VIDIOC_DQBUF, &buffer) != 0) {
        if (millis() - lastData > 1500) break;
        delay(5); continue;
      }
      if (buffer.index >= 2 || buffer.bytesused < stride * height || buffer.bytesused > lengths[buffer.index]) break;
      lastData = millis();
      if (lastData - lastProcessed >= 100 && !(buffer.flags & V4L2_BUF_FLAG_ERROR)) {
        lastProcessed = lastData;
        const auto *pixels = static_cast<const uint8_t *>(buffers[buffer.index]);
        SHA256 hash; hash.begin(); hash.write(pixels, stride * height); hash.end(digest);
        cameraCount = ++sequence; cameraState = State::Active;
        publish(1, sequence, digest, sizeof(digest));
        secureZero(digest, sizeof(digest));
        if (small && preview) {
          // Contain, preserving aspect ratio (no distorted camera image).
          const unsigned pw = (width * PREVIEW_HEIGHT > height * PREVIEW_WIDTH) ? PREVIEW_WIDTH : width * PREVIEW_HEIGHT / height;
          const unsigned ph = (width * PREVIEW_HEIGHT > height * PREVIEW_WIDTH) ? height * PREVIEW_WIDTH / width : PREVIEW_HEIGHT;
          secureZero(small, PREVIEW_PIXELS * sizeof(uint16_t));
          for (unsigned y = 0; y < ph; ++y) for (unsigned x = 0; x < pw; ++x) {
            const size_t offset = (y * height / ph) * stride + (x * width / pw) * 2;
            small[(y + (PREVIEW_HEIGHT - ph) / 2) * PREVIEW_WIDTH + x + (PREVIEW_WIDTH - pw) / 2] =
                static_cast<uint16_t>(pixels[offset] | (pixels[offset + 1] << 8));
          }
          if (xSemaphoreTake(mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
            if (!stopRequested) { memcpy(preview, small, PREVIEW_PIXELS * sizeof(uint16_t)); previewSequence = sequence; }
            xSemaphoreGive(mutex);
          }
        }
      }
      if (ioctl(fd, VIDIOC_QBUF, &buffer) != 0) break;
    }
  } while (false);
  if (!stopRequested && cameraState != State::Absent) cameraState = State::Failed;
  if (streaming) {
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) != 0) stopFailed = true;
  }
  // Do not wipe a live DMA buffer after a failed stream stop.
  if (!stopFailed) for (unsigned i = 0; i < 2; ++i) if (buffers[i]) {
    secureZero(buffers[i], lengths[i]);
    // ESP Video owns MMAP allocation; closing the device releases its buffers.
  }
  if (fd >= 0 && ::close(fd) != 0) stopFailed = true;
  if (initialized && esp_video_deinit() != ESP_OK) stopFailed = true;
  if (small) { secureZero(small, PREVIEW_PIXELS * sizeof(uint16_t)); free(small); }
  secureZero(digest, sizeof(digest));
  cameraAlive = false;
  vTaskDelete(nullptr);
}
}

bool start() {
  if (!stopped() || stopFailed) return false;
  if (!mutex) mutex = xSemaphoreCreateMutex();
  if (!mutex || bsp_i2c_init() != ESP_OK) return false;
  if (!preview) preview = static_cast<uint16_t *>(heap_caps_calloc(PREVIEW_PIXELS, sizeof(uint16_t), MALLOC_CAP_SPIRAM));
  secureZero(packets, sizeof(packets)); previewSequence = 0;
  if (preview) secureZero(preview, PREVIEW_PIXELS * sizeof(uint16_t));
  audioCount = cameraCount = 0; audioLevel = 0;
  audioState = cameraState = State::Probing; stopRequested = false;
  audioAlive = true;
  if (xTaskCreate(audioTask, "aurora-mic", 6144, nullptr, 3, nullptr) != pdPASS) {
    audioAlive = false; audioState = State::Failed;
  }
  cameraAlive = true;
  if (xTaskCreate(cameraTask, "aurora-camera", 8192, nullptr, 3, nullptr) != pdPASS) {
    cameraAlive = false; cameraState = State::Failed;
  }
  return true;
}
void requestStop() { stopRequested = true; }
bool stopped() {
  if (audioAlive || cameraAlive || stopFailed) return false;
  if (mutex) {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(20)) != pdTRUE) return false;
    secureZero(packets, sizeof(packets)); previewSequence = 0;
    if (preview) secureZero(preview, PREVIEW_PIXELS * sizeof(uint16_t));
    xSemaphoreGive(mutex);
  }
  return true;
}
Status status() { return {audioState.load(), cameraState.load(), audioLevel.load(), audioCount.load(), cameraCount.load()}; }
void drain(TouchEntropy &collector) {
  if (!mutex || stopRequested || xSemaphoreTake(mutex, 0) != pdTRUE) return;
  for (unsigned i = 0; i < 2; ++i) {
    if (packets[i].length) collector.addAuxiliary(static_cast<TouchEntropy::Source>(i + 1),
        packets[i].sequence, packets[i].data, packets[i].length);
    secureZero(&packets[i], sizeof(Packet));
  }
  xSemaphoreGive(mutex);
}
bool copyPreview(uint16_t *rgb565, size_t pixels, uint32_t &sequence) {
  if (!mutex || !preview || !rgb565 || pixels < PREVIEW_PIXELS || xSemaphoreTake(mutex, 0) != pdTRUE) return false;
  const bool fresh = previewSequence && sequence != previewSequence && !stopRequested;
  if (fresh) { memcpy(rgb565, preview, PREVIEW_PIXELS * sizeof(uint16_t)); sequence = previewSequence; }
  xSemaphoreGive(mutex);
  return fresh;
}
}
