#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <new>
#include "Arduino.h"
#include "Hash.h"
#include "audio_entropy_gate.h"
#include "wallet_file_format.h"

// Match GCC's packed sample layout when exercising the real header under MSVC.
// MSVC has no GNU inline-assembly barrier; the volatile wipe loop still runs.
#if defined(_MSC_VER) && !defined(__clang__)
#define __attribute__(...)
#define __asm__
#define __volatile__(...)
#endif
#pragma pack(push, 1)
#include "entropy.h"
#pragma pack(pop)

using Digest = std::array<uint8_t, 32>;
struct Result { Digest digest; uint32_t preview; };
#if defined(AURORA_BOARD_P4)
constexpr bool HAS_LIGHT_PRESSURE = false;
#else
constexpr bool HAS_LIGHT_PRESSURE = true;
#endif

static int16_t touchX(int16_t base, unsigned index) {
#if defined(AURORA_BOARD_P4)
  return static_cast<int16_t>(24 + (static_cast<unsigned>(base) + index * 37) % 432);
#else
  (void)index;
  return base;
#endif
}

static int16_t touchY(int16_t base, unsigned index) {
#if defined(AURORA_BOARD_P4)
  return static_cast<int16_t>(152 + (static_cast<unsigned>(base) + index * 29) % 188);
#else
  (void)index;
  return base;
#endif
}

Result collect(uint16_t light = 1234, int16_t x = 25, int16_t y = 70,
               uint16_t pressure = 300, uint32_t timeOffset = 0,
               uint32_t rngSalt = 0, uint8_t previewSalt = 0, bool inspect = false,
               unsigned auxiliarySource = 0, uint8_t auxiliaryValue = 42) {
  mock = {};
  mock.light = light;
  mock.rngSalt = rngSalt;
  mock.previewSalt = previewSalt;
  TouchEntropy entropy;
  entropy.begin();
  if (auxiliarySource) {
    assert(entropy.addAuxiliary(static_cast<TouchEntropy::Source>(auxiliarySource), 1, &auxiliaryValue, 1));
    assert(entropy.sampleCount() == 0 && !entropy.ready());
  }
  for (unsigned i = 0; i < TouchEntropy::REQUIRED_SAMPLES; ++i) {
    mock.time = 40000 + i * 20000 + timeOffset;
    entropy.add(touchX(x, i), touchY(y, i), pressure);
    if (inspect) for (unsigned j = 0; j < 20; ++j) (void)entropy.previewToken();
  }
  Result result{};
  result.preview = entropy.previewToken();
  assert(entropy.finish(result.digest.data()));
  assert(!mock.rngEnabled && entropy.previewToken() == 0);
  assert(std::all_of(mock.previewKey, mock.previewKey + 32, [](uint8_t b) { return b == 0; }));
  return result;
}

static void append16(std::vector<uint8_t> &bytes, uint16_t value) {
  bytes.push_back(static_cast<uint8_t>(value));
  bytes.push_back(static_cast<uint8_t>(value >> 8));
}
static void append32(std::vector<uint8_t> &bytes, uint32_t value) {
  append16(bytes, static_cast<uint16_t>(value));
  append16(bytes, static_cast<uint16_t>(value >> 16));
}

int main() {
  using namespace AuroraWalletFormat;
  {
    AudioEntropyGate gate;
    for (uint32_t i = 0; i < AudioEntropyGate::CALIBRATION_BLOCKS; ++i) {
      const auto result = gate.observe(400);
      assert(!result.qualified && result.threshold == AudioEntropyGate::MIN_ACTIVITY);
    }
    assert(!gate.observe(AudioEntropyGate::MIN_ACTIVITY - 1).qualified);
    const auto event = gate.observe(AudioEntropyGate::MIN_ACTIVITY);
    assert(event.qualified && event.level == 50);
    assert(gate.noiseFloor() < AudioEntropyGate::MIN_ACTIVITY);

    AudioEntropyGate noisy;
    for (uint32_t i = 0; i < AudioEntropyGate::CALIBRATION_BLOCKS; ++i)
      assert(!noisy.observe(2000).qualified);
    const auto belowAmbientThreshold = noisy.observe(5999);
    assert(!belowAmbientThreshold.qualified && belowAmbientThreshold.threshold == 6000);
    assert(noisy.observe(7000).qualified);

    AudioEntropyGate capped;
    for (uint32_t i = 0; i < AudioEntropyGate::CALIBRATION_BLOCKS; ++i)
      assert(capped.observe(10000).threshold == AudioEntropyGate::MAX_ACTIVITY);
    assert(capped.observe(AudioEntropyGate::MAX_ACTIVITY).qualified);
  }
  puts("PASS: dual-microphone activity gate calibration, adaptive floor, minimum threshold and cap");
  static_assert(HEADER_SIZE == 46 && SALT_OFFSET == 16 && SALT_SIZE == 16);
  static_assert(NONCE_OFFSET == 32 && NONCE_SIZE == 12 && TAG_SIZE == 16 && KEY_SIZE == 32);
  static_assert(FILE_VERSION == 1 && KDF_PBKDF2_HMAC_SHA256 == 1 && CIPHER_AES_256_GCM == 1);
  static_assert(KDF_ITERATIONS == 500000 && sizeof(AuroraPayloadV1) == 1058);
  static_assert(offsetof(AuroraPayloadV1, addressKind) == 8 && offsetof(AuroraPayloadV1, wordCount) == 9);
  static_assert(offsetof(AuroraPayloadV1, firmwareVersion) == 10 && offsetof(AuroraPayloadV1, addressType) == 26);
  static_assert(offsetof(AuroraPayloadV1, derivationPath) == 66 && offsetof(AuroraPayloadV1, mnemonic) == 98);
  static_assert(offsetof(AuroraPayloadV1, passphrase) == 354 && offsetof(AuroraPayloadV1, address) == 418);
  static_assert(offsetof(AuroraPayloadV1, accountXpub) == 514 && offsetof(AuroraPayloadV1, accountXprv) == 642);
  static_assert(offsetof(AuroraPayloadV1, privateWif) == 770 && offsetof(AuroraPayloadV1, receiveDescriptor) == 834);
  assert(memcmp(FILE_MAGIC, "AURORAW1", 8) == 0 && memcmp(PAYLOAD_MAGIC, "AURDAT01", 8) == 0);
  puts("PASS: frozen Aurora Wallet V1 identifiers, cryptographic parameters, sizes and all field offsets");
#if defined(AURORA_BOARD_P4)
  static_assert(TouchEntropy::REQUIRED_SAMPLES == 512, "P4 collection threshold must be 512 qualified movements");
#else
  static_assert(TouchEntropy::REQUIRED_SAMPLES == 320, "Archived threshold must remain 320 samples");
#endif
  {
    TouchEntropy entropy;
    Digest untouched;
    untouched.fill(0xA5);
    assert(!entropy.ready() && entropy.progress() == 0);
    entropy.add(25, 70, 300);
    assert(mock.lightReads == 0 && !entropy.finish(untouched.data()));
    assert(std::all_of(untouched.begin(), untouched.end(), [](uint8_t b) { return b == 0xA5; }));
    entropy.begin();
#if defined(AURORA_BOARD_P4)
    mock.time += 30000000;
    assert(entropy.progress() == 0 && entropy.activeCollectionMs() == 0 && !entropy.ready());
    entropy.cancel();
    mock = {};
    entropy.begin();
#endif
    for (unsigned i = 1; i <= TouchEntropy::REQUIRED_SAMPLES; ++i) {
#if defined(AURORA_BOARD_P4)
      mock.time = 40000 + (i - 1) * 20000;
#endif
      entropy.add(touchX(25, i - 1), touchY(70, i - 1), 300);
      assert(entropy.sampleCount() == i && mock.lightReads == (HAS_LIGHT_PRESSURE ? i : 0));
      assert(entropy.ready() == (i == TouchEntropy::REQUIRED_SAMPLES));
      if (i == TouchEntropy::REQUIRED_SAMPLES / 2 - 1) assert(entropy.progress() == 49);
      if (i == TouchEntropy::REQUIRED_SAMPLES / 2) assert(entropy.progress() == 50);
      if (i == TouchEntropy::REQUIRED_SAMPLES - 1) assert(entropy.progress() == 99);
      if (i == 160) {
        assert(!entropy.finish(untouched.data()));
        assert(std::all_of(untouched.begin(), untouched.end(), [](uint8_t b) { return b == 0xA5; }));
      }
    }
    assert(entropy.progress() == 100);
    entropy.add(touchX(26, TouchEntropy::REQUIRED_SAMPLES),
                touchY(71, TouchEntropy::REQUIRED_SAMPLES), 301);
#if defined(AURORA_BOARD_P4)
    assert(entropy.sampleCount() == TouchEntropy::REQUIRED_SAMPLES);
    mock.time += 20000;
    entropy.add(touchX(26, TouchEntropy::REQUIRED_SAMPLES + 1),
                touchY(71, TouchEntropy::REQUIRED_SAMPLES + 1), 301);
    assert(entropy.sampleCount() == TouchEntropy::REQUIRED_SAMPLES + 1 && entropy.ready());
#else
    assert(entropy.sampleCount() == TouchEntropy::REQUIRED_SAMPLES &&
           mock.lightReads == (HAS_LIGHT_PRESSURE ? TouchEntropy::REQUIRED_SAMPLES : 0));
#endif
    assert(!entropy.finish(nullptr) && entropy.ready());
    assert(entropy.finish(untouched.data()));
    assert(!entropy.ready() && !mock.rngEnabled && entropy.progress() == 0);
    assert(!entropy.finish(untouched.data()));
  }
  puts("PASS: threshold boundaries, saturation, ADC/RNG exclusion, inactive and null calls");

  {
    mock = {};
    TouchEntropy entropy;
    const uint8_t block[] = {0x00, 0x7F, 0xA5, 0xFF};
    using Source = TouchEntropy::Source;
    assert(!entropy.addAuxiliary(Source::Camera, 1, block, sizeof(block)));
    entropy.begin();
    assert(!entropy.addAuxiliary(static_cast<Source>(0), 1, block, sizeof(block)));
    assert(!entropy.addAuxiliary(static_cast<Source>(3), 1, block, sizeof(block)));
    assert(!entropy.addAuxiliary(Source::Camera, 1, nullptr, sizeof(block)));
    assert(!entropy.addAuxiliary(Source::Camera, 1, block, 0));
    assert(!entropy.addAuxiliary(Source::Camera, 1, block, 4097));
    assert(entropy.addAuxiliary(Source::Camera, 10, block, sizeof(block)));
    assert(!entropy.addAuxiliary(Source::Camera, 10, block, sizeof(block)));
    assert(!entropy.addAuxiliary(Source::Camera, 9, block, sizeof(block)));
    assert(entropy.addAuxiliary(Source::Microphone, 10, block, sizeof(block)));
    assert(entropy.addAuxiliary(Source::Camera, 11, block, sizeof(block)));
    assert(entropy.auxiliaryCount(Source::Camera) == 2);
    assert(entropy.auxiliaryCount(Source::Microphone) == 1);
    assert(entropy.sampleCount() == 0 && entropy.progress() == 0 && !entropy.ready());
    entropy.cancel();
    assert(entropy.auxiliaryCount(Source::Camera) == 0 && entropy.auxiliaryCount(Source::Microphone) == 0);
    entropy.begin();
    assert(entropy.addAuxiliary(Source::Camera, 1, block, sizeof(block)));
    for (unsigned i = 0; i < TouchEntropy::REQUIRED_SAMPLES; ++i) {
#if defined(AURORA_BOARD_P4)
      mock.time = 40000 + i * 20000;
#endif
      entropy.add(touchX(25, i), touchY(70, i), 300);
    }
#if defined(AURORA_BOARD_P4)
    assert(entropy.addAuxiliary(Source::Camera, 2, block, sizeof(block)));
#else
    assert(!entropy.addAuxiliary(Source::Camera, 2, block, sizeof(block)));
#endif
  }
  const auto microphone = collect(1234, 25, 70, 300, 0, 0, 0, false, 1);
  const auto camera = collect(1234, 25, 70, 300, 0, 0, 0, false, 2);
  assert(microphone.digest != camera.digest && microphone.digest != collect().digest);
  assert(camera.digest != collect(1234, 25, 70, 300, 0, 0, 0, false, 2, 43).digest);
  assert(camera.preview == microphone.preview && camera.preview == collect().preview);
  puts("PASS: auxiliary source separation, freshness, input bounds, cancellation, unchanged touch threshold and preview isolation");

  const auto baseline = collect();
  assert(baseline.digest == collect().digest);
  assert((baseline.digest != collect(1235).digest) == HAS_LIGHT_PRESSURE);
  assert(baseline.digest != collect(1234, 26).digest);
  assert(baseline.digest != collect(1234, 25, 71).digest);
  assert((baseline.digest != collect(1234, 25, 70, 301).digest) == HAS_LIGHT_PRESSURE);
  assert(baseline.digest != collect(1234, 25, 70, 300, 1).digest);
  assert(baseline.digest != collect(1234, 25, 70, 300, 0, 1).digest);
  assert((collect(0).digest != collect(4095).digest) == HAS_LIGHT_PRESSURE);
  puts("PASS: profile-specific inputs affect the digest; P4 has no fictitious light/pressure");

  const auto otherPreviewKey = collect(1234, 25, 70, 300, 0, 0, 1);
  assert(otherPreviewKey.digest == baseline.digest);
  assert(otherPreviewKey.preview != baseline.preview);
  assert(collect(1234, 25, 70, 300, 0, 0, 0, true).digest == baseline.digest);
  puts("PASS: independent preview key, read-only preview, preview key/token wiped on finish");

  // Independently encode all fields and the preserved final timing/RNG tail.
  std::vector<uint8_t> expected;
#if defined(AURORA_BOARD_P4)
  uint16_t expectedZones = 0;
#endif
  for (uint32_t i = 0; i < TouchEntropy::REQUIRED_SAMPLES; ++i) {
    const int16_t sampleX = touchX(25, i), sampleY = touchY(70, i);
    append16(expected, static_cast<uint16_t>(sampleX));
    append16(expected, static_cast<uint16_t>(sampleY));
    if (HAS_LIGHT_PRESSURE) append16(expected, 300);
    append32(expected, 40000 + i * 20000);
    append32(expected, 0x12345678u + 0x01020304u * (i + 1));
    if (HAS_LIGHT_PRESSURE) append16(expected, 1234);
#if defined(AURORA_BOARD_P4)
    append32(expected, i + 1);
    expected.push_back(1);
    const unsigned column = static_cast<unsigned>(sampleX - 24) * 4 / 432;
    const unsigned row = static_cast<unsigned>(sampleY - 152) * 3 / 188;
    expectedZones |= static_cast<uint16_t>(
        1u << ((row > 2 ? 2 : row) * 4 + (column > 3 ? 3 : column)));
#endif
  }
  const uint32_t finalTime = 40000 + (TouchEntropy::REQUIRED_SAMPLES - 1) * 20000;
  append32(expected, finalTime / 1000 - 20);
#if defined(AURORA_BOARD_P4)
  append32(expected, (TouchEntropy::REQUIRED_SAMPLES - 1) * 20);
#endif
  append32(expected, finalTime);
  append32(expected, 0x12345678u + 0x01020304u * (TouchEntropy::REQUIRED_SAMPLES + 1));
#if defined(AURORA_BOARD_P4)
  append32(expected, TouchEntropy::REQUIRED_SAMPLES);
  append32(expected, TouchEntropy::REQUIRED_SAMPLES);
  append32(expected, expectedZones);
  append32(expected, 0);
  append32(expected, 0);
#endif
  Digest expectedDigest{};
  nativeDigest(expected.data(), expected.size(), expectedDigest.data());
  assert(expectedDigest == baseline.digest);
  puts("PASS: exact sample serialization and final timing/RNG tail preserved");

  mock = {};
  {
    TouchEntropy entropy;
    entropy.begin(); entropy.add(25, 70, 300);
    entropy.cancel(); entropy.cancel();
    assert(!mock.rngEnabled && !entropy.ready() && entropy.sampleCount() == 0);
    assert(entropy.previewToken() == 0);
    assert(std::all_of(mock.previewKey, mock.previewKey + 32, [](uint8_t b) { return b == 0; }));
    entropy.begin(); entropy.add(25, 70, 300);
    assert(entropy.sampleCount() == 1 && !entropy.ready());
    entropy.begin();
    assert(entropy.sampleCount() == 0 && entropy.previewToken() == 0);
  }
  assert(!mock.rngEnabled);
  puts("PASS: cancel, repeated cancel, restart and destructor release the RNG");
}
