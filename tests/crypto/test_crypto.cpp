#include "Arduino.h"
#include <array>
#include <cassert>
#include "../../src/sd_export.cpp"
#include "../../src/pin_security.cpp"

int main() {
  assert(testCard.empty() && auroraSdReady() && testCard.empty());
  testCardReady=false; assert(!auroraSdReady());
  testCardReady=true; testRootReadable=false; assert(!auroraSdReady());
  testRootReadable=true; assert(auroraSdReady() && testMounts==testUnmounts);
  assert(auroraWalletCryptoSelfTest()); // Frozen PBKDF2-SHA256 and AES-256-GCM vectors.
  const auto *password = reinterpret_cast<const uint8_t *>("password");
  const auto *salt = reinterpret_cast<const uint8_t *>("salt");
  constexpr const char *mnemonic = "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about";
  constexpr uint8_t seedExpected[64] = {
      0xc5,0x52,0x57,0xc3,0x60,0xc0,0x7c,0x72,0x02,0x9a,0xeb,0xc1,0xb5,0x3c,0x05,0xed,
      0x03,0x62,0xad,0xa3,0x8e,0xad,0x3e,0x3e,0x9e,0xfa,0x37,0x08,0xe5,0x34,0x95,0x53,
      0x1f,0x09,0xa6,0x98,0x75,0x99,0xd1,0x82,0x64,0xc1,0xe1,0xc9,0x2f,0x2c,0xf1,0x41,
      0x63,0x0c,0x7a,0x3c,0x4a,0xb7,0xc8,0x1b,0x2f,0x00,0x16,0x98,0xe7,0x46,0x3b,0x04};
  uint8_t seed[64]{};
  assert(auroraPbkdf2Hmac(MBEDTLS_MD_SHA512, reinterpret_cast<const uint8_t *>(mnemonic), strlen(mnemonic),
      reinterpret_cast<const uint8_t *>("mnemonicTREZOR"), 14, 2048, 64, seed) == 0);
  assert(!memcmp(seed, seedExpected, sizeof(seed)));
  for (auto digest : {MBEDTLS_MD_SHA256, MBEDTLS_MD_SHA512}) {
    uint8_t current[64]{}, legacy[64]{};
    assert(auroraPbkdf2Hmac(digest, password, 8, salt, 4, 2048, 64, current) == 0);
    mbedtls_md_context_t context; mbedtls_md_init(&context);
    assert(mbedtls_md_setup(&context, mbedtls_md_info_from_type(digest), 1) == 0);
    assert(mbedtls_pkcs5_pbkdf2_hmac(&context, password, 8, salt, 4, 2048, 64, legacy) == 0);
    mbedtls_md_free(&context);
    assert(!memcmp(current, legacy, 64));
  }
  AuroraPinRecord pin{}, another{};
  assert(auroraPinCreate("01234567",pin));
  assert(auroraPinCreate("01234567",another));
  assert(memcmp(&pin,&another,sizeof(pin)));
  assert(auroraPinVerify("01234567",pin) && !auroraPinVerify("1234567",pin));
  for(const char *bad : {"", "123", "123456789", "12a4", "-123", "12 4"}) assert(!auroraPinValid(bad));
  assert(auroraPinValid("0000") && auroraPinValid("99999999"));
  AuroraPinGuard guard; guard.begin(pin);
  assert(!guard.attempt("0000") && guard.failures()==1);
  assert(guard.attempt("01234567") && guard.failures()==1);
  assert(!guard.attempt("1111") && !guard.attempt("2222") && guard.blocked());
  assert(!guard.attempt("01234567")); guard.clear(); assert(!guard.enabled());
  const WalletExportData fixture{2, 12, "Native Segwit", "m/84'/0'/0'/0/0",
      "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about",
      "TEST ONLY", "address-fixture", "public-fixture", "private-fixture", "wif-fixture", "descriptor-fixture", &pin};
  constexpr const char *filePassword = "a-test-password-only";
  char path[80]{}; AuroraWalletData restored{};
  testCardReady=false;
  assert(writeWalletExportFile(WalletExportFormat::AuroraWallet,"absent",filePassword,fixture,path,sizeof(path))==WalletExportResult::NoCard);
  assert(testCard.empty()); testCardReady=true;
  assert(writeWalletExportFile(WalletExportFormat::AuroraWallet, "test", filePassword, fixture, path, sizeof(path)) == WalletExportResult::Ok);
  assert(testCard.at("/test.aurora").size() == 1200);
  const auto original = testCard.at("/test.aurora");
  assert(readAuroraWalletFile("test", filePassword, restored) == AuroraWalletReadResult::Ok);
  assert(!strcmp(restored.mnemonic, fixture.mnemonic) && !strcmp(restored.passphrase, fixture.passphrase));
  assert(!strcmp(restored.accountXprv, fixture.accountXprv) && restored.addressKind == 2 && restored.wordCount == 12);
  assert(restored.fileVersion==2 && auroraPinVerify("01234567",restored.pin));
  assert(original[9]==KDF_PBKDF2_HMAC_SHA256 && getLe32(original.data()+12)==120000);
  assert(auroraPinCreate("4321",another));
  auto otherFile=fixture; otherFile.pin=&another;
  assert(writeWalletExportFile(WalletExportFormat::AuroraWallet,"other",filePassword,otherFile,path,sizeof(path))==WalletExportResult::Ok);
  assert(readAuroraWalletFile("other",filePassword,restored)==AuroraWalletReadResult::Ok);
  assert(!strcmp(restored.mnemonic,fixture.mnemonic) && !strcmp(restored.accountXprv,fixture.accountXprv));
  assert(auroraPinVerify("4321",restored.pin) && !auroraPinVerify("01234567",restored.pin));
  // V1 has no board lock. A payload tagged with the released CYD version is
  // accepted by the same reader used for the P4 (all other fields unchanged).
  AuroraPayloadV1 releasedPayload{}; assert(fillPayload(fixture, releasedPayload));
  strlcpy(releasedPayload.firmwareVersion, "1.7.6", sizeof(releasedPayload.firmwareVersion));
  uint8_t releasedKey[KEY_SIZE]{};
  assert(deriveKey(filePassword, original.data() + SALT_OFFSET, SALT_SIZE, KDF_ITERATIONS, releasedKey));
  auto &released = testCard.at("/test.aurora");
  released.resize(HEADER_SIZE+sizeof(releasedPayload)+TAG_SIZE);
  memcpy(released.data(),FILE_MAGIC,sizeof(FILE_MAGIC)); released[8]=FILE_VERSION;
  putLe16(released.data()+44,sizeof(releasedPayload));
  assert(aesGcmEncrypt(releasedKey, released.data() + NONCE_OFFSET, released.data(), HEADER_SIZE,
      reinterpret_cast<const uint8_t *>(&releasedPayload), sizeof(releasedPayload),
      released.data() + HEADER_SIZE, released.data() + HEADER_SIZE + sizeof(releasedPayload)));
  assert(readAuroraWalletFile("test", filePassword, restored) == AuroraWalletReadResult::Ok);
  assert(!strcmp(restored.mnemonic, fixture.mnemonic));
  assert(restored.fileVersion==1 && !auroraPinRecordValid(restored.pin));
  testCard["/test.aurora"] = original;
  assert(writeWalletExportFile(WalletExportFormat::AuroraWallet, "test", filePassword, fixture, path, sizeof(path)) == WalletExportResult::AlreadyExists);
  assert(testCard.at("/test.aurora") == original);
  assert(readAuroraWalletFile("test", "a-wrong-password", restored) == AuroraWalletReadResult::AuthenticationFailed);
  const std::array<uint8_t, sizeof(restored)> zeros{};
  assert(!memcmp(&restored, zeros.data(), zeros.size()));
  for(size_t offset : {size_t(0),size_t(8),size_t(9),size_t(10),size_t(11),size_t(44)}) {
    testCard["/test.aurora"]=original; testCard["/test.aurora"][offset]^=0x40;
    assert(readAuroraWalletFile("test",filePassword,restored)==AuroraWalletReadResult::InvalidFormat);
    assert(!memcmp(&restored,zeros.data(),zeros.size()));
  }
  AuroraPayloadV2 invalidPayload{}; assert(fillPayload(fixture,invalidPayload.wallet));
  memcpy(invalidPayload.wallet.magic,PAYLOAD_MAGIC_V2,sizeof(PAYLOAD_MAGIC_V2));
  testCard["/test.aurora"]=original;
  auto &noPin=testCard["/test.aurora"];
  assert(aesGcmEncrypt(releasedKey,noPin.data()+NONCE_OFFSET,noPin.data(),HEADER_SIZE,
      reinterpret_cast<const uint8_t *>(&invalidPayload),sizeof(invalidPayload),
      noPin.data()+HEADER_SIZE,noPin.data()+HEADER_SIZE+sizeof(invalidPayload)));
  assert(readAuroraWalletFile("test",filePassword,restored)==AuroraWalletReadResult::InvalidFormat);
  assert(!memcmp(&restored,zeros.data(),zeros.size()));
  for (size_t offset : {size_t(16), size_t(32), size_t(100), original.size() - 1}) {
    testCard["/test.aurora"] = original; testCard["/test.aurora"][offset] ^= 1;
    assert(readAuroraWalletFile("test", filePassword, restored) == AuroraWalletReadResult::AuthenticationFailed);
    assert(!memcmp(&restored, zeros.data(), zeros.size()));
  }
  testCard["/test.aurora"] = original; testCard["/test.aurora"].pop_back();
  assert(readAuroraWalletFile("test", filePassword, restored) == AuroraWalletReadResult::InvalidFormat);
  testCard["/test.aurora"] = original;
  auto missingPin=fixture; missingPin.pin=nullptr;
  assert(writeWalletExportFile(WalletExportFormat::AuroraWallet,"no-pin",filePassword,missingPin,path,sizeof(path))==WalletExportResult::InvalidPin);
  assert(!testCard.count("/no-pin.aurora"));
  testSyncOk = false;
  assert(writeWalletExportFile(WalletExportFormat::AuroraWallet, "failure", filePassword, fixture, path, sizeof(path)) == WalletExportResult::WriteFailed);
  assert(!testCard.count("/failure.aurora") && testCard.at("/test.aurora") == original);
  puts("PASS: read-only SD presence/root check accepts empty media, rejects absent/unreadable media, no-card export creates nothing");
  puts("PASS: unchanged wallet/file KDF vectors, V1 read and V2 round trip, independent PIN and 3-attempt guard, tamper/wrong-password rejection, no overwrite, failed-sync cleanup");
}
