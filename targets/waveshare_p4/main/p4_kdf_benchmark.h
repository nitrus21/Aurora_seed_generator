#pragma once

#if defined(AURORA_P4_KDF_BENCHMARK)

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "security_memory.h"
#include "wallet_kdf.h"

struct AuroraKdfBenchmarkVector {
  uint32_t iterations;
  uint8_t expected[32];
};

// hashlib.pbkdf2_hmac("sha256", b"aurora-public-kdf-benchmark-password",
//                     bytes(range(16)), iterations, 32)
static constexpr AuroraKdfBenchmarkVector AURORA_KDF_BENCHMARK_VECTORS[] = {
  {10000, {0x62,0xf8,0xe5,0xea,0x86,0x94,0xca,0xe8,0xc7,0xb7,0x4f,0x6e,0xa7,0xab,0x76,0x5e,
           0xca,0x41,0xe2,0x9e,0xef,0xca,0x2c,0x50,0x24,0xb5,0x80,0xa0,0x17,0xb5,0xd2,0x4b}},
  {120000,{0x38,0x7c,0x71,0xc0,0xdd,0xcb,0x5b,0x3b,0x79,0x20,0x61,0x4b,0x7f,0x99,0x62,0x69,
           0x32,0x15,0x4f,0xe7,0x2e,0x60,0x32,0xf5,0xd6,0xd7,0xa3,0x1c,0xb8,0xa3,0x6f,0x20}},
  {300000,{0xa7,0xed,0x55,0x33,0xba,0xda,0xc7,0x0d,0x7c,0xe4,0x9e,0x8c,0x32,0xda,0x6c,0x3a,
           0xf6,0x48,0x3b,0x26,0xf7,0x45,0x94,0xfe,0xf2,0x2e,0x79,0xfb,0x15,0x6d,0x98,0xad}},
  {500000,{0x19,0x38,0xa4,0xf9,0x68,0x1f,0x24,0x05,0x71,0xbb,0xab,0x13,0x50,0x53,0xd8,0xab,
           0x2e,0x70,0x22,0xf8,0xf4,0xb0,0x72,0x56,0x64,0x41,0x87,0x60,0xdb,0xc1,0x09,0x5e}},
};

[[noreturn]] static void auroraRunP4KdfBenchmark() {
  static constexpr uint8_t password[] = "aurora-public-kdf-benchmark-password";
  uint8_t salt[16]{};
  uint8_t output[32]{};
  for (unsigned i = 0; i < sizeof(salt); ++i) salt[i] = static_cast<uint8_t>(i);

  printf("AURORA_KDF_BENCH schema=1 begin profile=waveshare-p4-rev1 samples=3\n");
  for (const auto &vector : AURORA_KDF_BENCHMARK_VECTORS) {
    for (unsigned sample = 1; sample <= 3; ++sample) {
      const int64_t started = esp_timer_get_time();
      const bool derived = auroraWalletKdf(password, sizeof(password) - 1,
                                           salt, sizeof(salt), vector.iterations, output);
      const int64_t elapsed = esp_timer_get_time() - started;
      const bool matches = derived && memcmp(output, vector.expected, sizeof(output)) == 0;
      printf("AURORA_KDF_BENCH iterations=%" PRIu32 " sample=%u us=%" PRId64 " ok=%u\n",
             vector.iterations, sample, elapsed, matches ? 1U : 0U);
      auroraSecureZero(output, sizeof(output));
      if (!matches) auroraSecurityPanic();
      vTaskDelay(pdMS_TO_TICKS(250));
    }
  }
  auroraSecureZero(salt, sizeof(salt));
  printf("AURORA_KDF_BENCH done ok=1\n");
  fflush(stdout);
  for (;;) vTaskDelay(pdMS_TO_TICKS(1000));
}

#endif
