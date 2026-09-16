#include "hardware_rng.h"
#include <atomic>
#include <cassert>
#include <cstdio>
#include <thread>
#include <vector>
#include <cstring>
extern "C" {
#include "ecdsa.h"
#include "secp256k1.h"
}
extern "C" uint32_t random32(void);
extern "C" void random_buffer(uint8_t *, size_t);
static std::atomic<bool> enabled{false};
static std::atomic<unsigned> starts{0}, stops{0}, reads{0};
extern "C" void bootloader_random_enable(void) {
  assert(!enabled.exchange(true)); ++starts;
}
extern "C" void bootloader_random_disable(void) {
  assert(enabled.exchange(false)); ++stops;
}
extern "C" uint32_t esp_random(void) {
  assert(enabled.load()); return ++reads;
}
extern "C" void esp_fill_random(void *p, size_t n) {
  auto *out = static_cast<uint8_t *>(p);
  while (n--) *out++ = static_cast<uint8_t>(esp_random());
}
int main() {
  assert(random32() == 1 && !enabled && starts == 1 && stops == 1);
  // A derivation/random_buffer nested inside collection must leave its owner on.
  hardwareRngEnable();
  hardwareRngEnable();
  auto before = starts.load();
  uint8_t bytes[32];
  random_buffer(bytes, sizeof(bytes));
  hardwareRandomFill(bytes, sizeof(bytes));
  assert(enabled && starts == before);
  hardwareRngDisable(); assert(enabled);
  hardwareRngDisable(); assert(!enabled && starts == stops);
  // Real elliptic-curve consumers: public scalar 1 must still yield generator G
  // under fresh coordinate masks, both with and without a collection owner.
  for (unsigned i = 0; i < 2; ++i) {
    if (i) hardwareRngEnable();
    bignum256 scalar; bn_read_uint32(1, &scalar);
    curve_point point{};
    const auto previousReads = reads.load();
    scalar_multiply(&secp256k1, &scalar, &point);
    assert(!memcmp(&point, &secp256k1.G, sizeof(point)));
    point_multiply(&secp256k1, &scalar, &secp256k1.G, &point);
    assert(!memcmp(&point, &secp256k1.G, sizeof(point)) && reads > previousReads);
    assert(enabled.load() == (i != 0));
    if (i) hardwareRngDisable();
  }
  // No owner may disable the source underneath another task's read.
  std::vector<std::thread> workers;
  for (unsigned n = 0; n < 4; ++n) workers.emplace_back([] {
    for (unsigned i = 0; i < 1000; ++i) {
      hardwareRngEnable();
      for (unsigned j = 0; j < 9; ++j) (void)random32();
      hardwareRngDisable();
    }
  });
  for (auto &worker : workers) worker.join();
  assert(!enabled && starts == stops && reads > 36000);
  puts("PASS: actual pinned random32/random_buffer use enabled entropy; nested and concurrent ownership balanced, no RNG cache");
  puts("PASS: real scalar/point multiplication consumes the bridge and preserves the public generator vector");
}
