#include "entropy_health.h"
#include <cassert>
#include <cstdint>
#include <cstdio>

static AuroraEntropyHealth evaluate(uint32_t (*next)()) {
  AuroraEntropyHealth health;
  for (uint32_t i = 0; i < AuroraEntropyHealth::REQUIRED_WORDS; ++i) health.add(next());
  return health;
}

static uint32_t xorshift() {
  static uint32_t state = 0xA341316CU;
  state ^= state << 13;
  state ^= state >> 17;
  state ^= state << 5;
  return state;
}

static uint32_t zeroes() { return 0; }
static uint32_t alternating() { return 0xAAAAAAAAU; }
static uint32_t counter() { static uint32_t value = 0; return value++; }
static uint32_t biased() { return xorshift() & 0x0FFFFFFFU; }

int main() {
  auto healthy = evaluate(xorshift);
  assert(healthy.passed());
  assert(!evaluate(zeroes).passed());
  assert(!evaluate(alternating).passed());
  assert(!evaluate(counter).passed());
  assert(!evaluate(biased).passed());
  assert(AuroraEntropyHealth::ratioPer10000(1, 2) == 5000);
  puts("PASS: aggregate health checks accept deterministic white-stream witness");
  puts("PASS: stuck, alternating, counter and biased fault witnesses rejected");
}
