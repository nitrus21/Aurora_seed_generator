#pragma once
#include <stddef.h>
#include <stdint.h>

// File KDF only: PBKDF2-HMAC-SHA256, one 32-byte block. No session/global cache.
bool auroraWalletKdf(const uint8_t *password, size_t passwordLength,
                     const uint8_t *salt, size_t saltLength,
                     uint32_t iterations, uint8_t output[32]);
