#pragma once
// Run on an isolated P4 diagnostic, with public synthetic data only.
// Unlike host KDF tests this exercises the real peripheral/chunk adapter.
#include <cstring>
#include "wallet_kdf.h"
#include "secure_memory.h"
extern "C" {
#include "utility/trezor/pbkdf2.h"
}

static unsigned testP4KdfBoundaries() {
  unsigned failures = 0;
  const unsigned lengths[] = {0, 1, 63, 64, 65, 129};
  const unsigned rounds[] = {1, 2, 2047, 2048, 2049, 4096, 4097};
  uint8_t password[129], salt[128];
  for (unsigned i=0;i<sizeof(password);++i) password[i]=(i*37U)&255U;
  for (unsigned i=0;i<sizeof(salt);++i) salt[i]=(i*53U)&255U;
  for (unsigned length : lengths) for (unsigned count : rounds) {
    uint8_t expected[32]{}, actual[32]{};
    PBKDF2_HMAC_SHA256_CTX reference{};
    const unsigned saltLength = length == 129 ? 128 : length;
    pbkdf2_hmac_sha256_Init(&reference,password,length,salt,saltLength,1);
    pbkdf2_hmac_sha256_Update(&reference,count); // pinned software reference
    pbkdf2_hmac_sha256_Final(&reference,expected);
    if (!auroraWalletKdf(password,length,salt,saltLength,count,actual) ||
        memcmp(actual,expected,sizeof(actual))) ++failures;
    secureZero(&reference,sizeof(reference));
    secureZero(expected,sizeof(expected)); secureZero(actual,sizeof(actual));
  }
  secureZero(password,sizeof(password)); secureZero(salt,sizeof(salt));
  return failures;
}
