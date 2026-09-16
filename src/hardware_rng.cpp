#include "hardware_rng.h"
#include <cstdlib>
#include <limits>
#include <mutex>

namespace {
// Only ownership metadata is retained, never random bytes or a DRBG seed.
std::mutex sourceMutex;
unsigned sourceUsers = 0;
}

void hardwareRngEnable() {
  std::lock_guard<std::mutex> lock(sourceMutex);
  if (sourceUsers == std::numeric_limits<unsigned>::max()) std::abort();
  if (sourceUsers++ == 0) bootloader_random_enable();
}

void hardwareRngDisable() {
  std::lock_guard<std::mutex> lock(sourceMutex);
  if (!sourceUsers) std::abort();
  if (--sourceUsers == 0) bootloader_random_disable();
}

// Called by the pinned uBitcoin rand.c on both devices, including coordinate
// blinding during public-key derivation. No password/key/random cache.
extern "C" uint32_t auroraCryptoRandom32(void) {
  hardwareRngEnable();
  const uint32_t result = esp_random();
  hardwareRngDisable();
  return result;
}
