#pragma once
#include <limits.h>

static unsigned kdfWipes = 0, kdfLive = 0, kdfYields = 0;
void auroraWalletKdfAudit(const void *state, size_t length, bool cleared) {
  bool nonzero = false;
  for (size_t i = 0; i < length; ++i) nonzero |= static_cast<const uint8_t *>(state)[i] != 0;
  assert(nonzero != cleared);
  if (cleared) ++kdfWipes; else ++kdfLive;
}
void auroraWalletKdfYieldAudit() { ++kdfYields; }

static void testOptimizedKdf() {
  uint8_t password[129]{}, salt[128]{}, expected[32]{}, actual[32]{};
  for (size_t i=0;i<sizeof(password);++i) password[i]=static_cast<uint8_t>(i*17);
  for (size_t i=0;i<sizeof(salt);++i) salt[i]=static_cast<uint8_t>(i*31);
  // Differential tests against independent Mbed TLS: binary/long/empty inputs
  // and boundaries of the chunk API, including its first-iteration convention.
  for (size_t length : {0u,1u,63u,64u,65u,129u}) {
    for (uint32_t rounds : {1u,2u,2047u,2048u,2049u,4096u,4097u}) {
      const size_t saltLength = length == 129 ? 128 : length;
      const unsigned beforeWipes=kdfWipes, beforeYields=kdfYields;
      assert(auroraWalletKdf(password,length,salt,saltLength,rounds,actual));
      assert(auroraPbkdf2Hmac(MBEDTLS_MD_SHA256,password,length,salt,saltLength,rounds,32,expected)==0);
      assert(!memcmp(actual,expected,32));
      assert(kdfWipes==beforeWipes+1 && kdfLive==kdfWipes);
      assert(kdfYields-beforeYields==(rounds-1)/2048);
    }
  }
  for (unsigned bad=0;bad<5;++bad) {
    memset(actual,0xa5,sizeof(actual));
    assert(!auroraWalletKdf(bad==0?nullptr:password,bad==1?size_t(INT_MAX)+1:1,
        bad==2?nullptr:salt,bad==3?size_t(INT_MAX)+1:1,bad==4?0:1,actual));
    for (uint8_t byte : actual) assert(byte==0);
  }
  assert(!auroraWalletKdf(password,1,salt,1,1,nullptr));
  secureZero(password,sizeof(password)); secureZero(salt,sizeof(salt));
  secureZero(expected,sizeof(expected)); secureZero(actual,sizeof(actual));
  puts("PASS: optimized KDF 42 differential cases, chunk accounting, live-state positive controls, state wiped before return, invalid-output wipe");
}

// Public fixtures only. The frozen key was generated independently with
// Python hashlib.pbkdf2_hmac('sha256', password, salt, 500000, 32).
static void testKdfPolicy() {
  testOptimizedKdf();
#if defined(AURORA_BOARD_CYD) && !defined(AURORA_BOARD_P4)
  static_assert(KDF_ITERATIONS == 120000, "CYD writer cost must be 120000");
#else
  static_assert(KDF_ITERATIONS == 500000, "P4 writer cost must stay 500000");
#endif
  static_assert(KDF_ITERATIONS_MIN == 10000 && KDF_ITERATIONS_MAX == 500000,
                "Existing files must remain readable on either board");
  constexpr const char *password = "AURORA-public-KDF-test-only";
  const uint8_t salt[16] = {'A','U','R','O','R','A','-','T','E','S','T','-','S','A','L','T'};
  constexpr uint8_t expected[32] = {
    0x36,0xb2,0x09,0xf8,0xba,0x41,0xb9,0x3b,0x54,0x38,0x1c,0x41,0x9b,0xd7,0xe4,0x6e,
    0x68,0xf2,0xb6,0x29,0x93,0x51,0x35,0x87,0x01,0x50,0x7f,0xa8,0x89,0x6c,0x43,0x54};
  uint8_t key[KEY_SIZE]{};
  assert(deriveKey(password, salt, sizeof(salt), 500000, key));
  assert(!memcmp(key, expected, sizeof(key)));
  secureZero(key, sizeof(key));
  const WalletExportData fixture{2,12,"Native Segwit","m/84'/0'/0'/0/0",
    "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about",
    "TEST ONLY","address-fixture","public-fixture","private-fixture","wif-fixture","descriptor-fixture",nullptr};
  for (bool v2 : {false, true}) {
    for (uint32_t rounds : {10000u, 120000u, 500000u}) {
      AuroraPayloadV2 payload{};
      assert(fillPayload(fixture, payload.wallet));
      if (v2) {
        memcpy(payload.wallet.magic, PAYLOAD_MAGIC_V2, sizeof(PAYLOAD_MAGIC_V2));
        assert(auroraPinCreate("1234", payload.pin)); // Legacy file, not a UI session.
      }
      const size_t length = v2 ? sizeof(payload) : sizeof(payload.wallet);
      std::vector<uint8_t> file(HEADER_SIZE + length + TAG_SIZE);
      auto *header = file.data();
      memcpy(header, v2 ? FILE_MAGIC_V2 : FILE_MAGIC, 8);
      header[8] = v2 ? FILE_VERSION_V2 : FILE_VERSION;
      header[9] = KDF_PBKDF2_HMAC_SHA256; header[10] = CIPHER_AES_256_GCM;
      putLe32(header + 12, rounds); memcpy(header + SALT_OFFSET, salt, sizeof(salt));
      memset(header + NONCE_OFFSET, 0x5a, NONCE_SIZE); // Deterministic public TEST ONLY.
      putLe16(header + 44, static_cast<uint16_t>(length));
      assert(deriveKey(password, salt, sizeof(salt), rounds, key));
      assert(aesGcmEncrypt(key, header + NONCE_OFFSET, header, HEADER_SIZE,
          reinterpret_cast<const uint8_t *>(&payload), length,
          header + HEADER_SIZE, header + HEADER_SIZE + length));
      secureZero(key, sizeof(key)); secureZero(&payload, sizeof(payload));
      testCard["/kdf-policy.aurora"] = file;
      AuroraWalletData opened{};
      assert(readAuroraWalletFile("kdf-policy", password, opened) == AuroraWalletReadResult::Ok);
      assert(!strcmp(opened.mnemonic, fixture.mnemonic));
      assert(opened.fileVersion == (v2 ? 2 : 1));
      wipeAuroraWalletData(opened);
      if (rounds == 500000) {
        for (uint32_t bad : {0u, 9999u, 500001u, 600000u, UINT32_MAX, 120000u}) {
          testCard["/kdf-policy.aurora"] = file;
          putLe32(testCard.at("/kdf-policy.aurora").data() + 12, bad);
          memset(&opened, 0xa5, sizeof(opened));
          const auto status = bad == 120000 ? AuroraWalletReadResult::AuthenticationFailed
                                           : AuroraWalletReadResult::InvalidFormat;
          assert(readAuroraWalletFile("kdf-policy", password, opened) == status);
          for (size_t i = 0; i < sizeof(opened); ++i)
            assert(reinterpret_cast<const uint8_t *>(&opened)[i] == 0);
        }
      }
      secureZero(file.data(), file.size());
      auto &stored = testCard.at("/kdf-policy.aurora");
      secureZero(stored.data(), stored.size()); testCard.erase("/kdf-policy.aurora");
    }
  }
  puts("PASS: independent 500000 vector; V1/V2 10000/120000/500000; bounds, downgrade rejection and failure-output wipe");
}
