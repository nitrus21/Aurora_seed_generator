#include "wallet_kdf.h"
#include "secure_memory.h"
#include <limits.h>
extern "C" {
#include "utility/trezor/pbkdf2.h"
#if defined(AURORA_BOARD_P4) && !defined(AURORA_NATIVE_TEST)
void auroraP4KdfUpdate(PBKDF2_HMAC_SHA256_CTX *context, uint32_t iterations);
#endif
}
#if !defined(AURORA_NATIVE_TEST)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif
#if defined(AURORA_KDF_TEST)
extern void auroraWalletKdfAudit(const void *state, size_t length, bool cleared);
extern void auroraWalletKdfYieldAudit();
#endif

bool auroraWalletKdf(const uint8_t *password, size_t passwordLength,
                     const uint8_t *salt, size_t saltLength,
                     uint32_t iterations, uint8_t output[32]) {
  if (!output) return false;
  if (!password || !salt || passwordLength > INT_MAX || saltLength > INT_MAX || !iterations) {
    secureZero(output, 32);
    return false;
  }
  // The pinned library prepares inner/outer SHA states once per derivation.
  // They are password-equivalent secrets and must never outlive this call.
  PBKDF2_HMAC_SHA256_CTX context{};
  pbkdf2_hmac_sha256_Init(&context, password, static_cast<int>(passwordLength),
                         salt, static_cast<int>(saltLength), 1);
#if defined(AURORA_KDF_TEST)
  auroraWalletKdfAudit(&context, sizeof(context), false);
#endif
  for (uint32_t remaining = iterations; remaining;) {
    const uint32_t count = remaining > 2048 ? 2048 : remaining;
    // Init computes U1; the first Update accounts for it via context.first.
    // Subsequent Updates perform exactly count further iterations.
#if defined(AURORA_BOARD_P4) && !defined(AURORA_NATIVE_TEST)
    auroraP4KdfUpdate(&context, count);
#else
    pbkdf2_hmac_sha256_Update(&context, count);
#endif
    remaining -= count;
#if defined(AURORA_KDF_TEST)
    if (remaining) auroraWalletKdfYieldAudit();
#endif
#if !defined(AURORA_NATIVE_TEST)
    if (remaining) vTaskDelay(1); // Feed idle tasks, not a reentrant UI callback.
#endif
  }
  pbkdf2_hmac_sha256_Final(&context, output); // Library clears its state too.
  secureZero(&context, sizeof(context));
#if defined(AURORA_KDF_TEST)
  auroraWalletKdfAudit(&context, sizeof(context), true);
#endif
  return true;
}
