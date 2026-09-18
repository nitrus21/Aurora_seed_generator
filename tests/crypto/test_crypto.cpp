#include "Arduino.h"
#include <array>
#include <cassert>
#include "../../.pio/libdeps/esp32-2432S028R/uBitcoin/src/utility/trezor/bip39_english.h"
extern "C" const char *const *mnemonic_wordlist(void) { return wordlist; }
#include "../../src/aezeed.cpp"
#include "../../src/sd_export.cpp"
#include "../../src/pin_security.cpp"
#include "test_kdf_policy.h"

template <typename T>
static bool allZero(const T &value) {
  const auto *bytes = reinterpret_cast<const uint8_t *>(&value);
  for (size_t i = 0; i < sizeof(value); ++i) if (bytes[i]) return false;
  return true;
}

static void fingerprintFixture(const std::vector<uint8_t> &file, uint8_t output[32]) {
  assert(mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
                    file.data(), file.size(), output) == 0);
}

static void testSessionAccess(const WalletExportData &fixture, const char *password) {
  AuroraWalletData opened{};
  uint8_t access[32]{};
  uint8_t expectedFingerprint[32]{};
  uint8_t independentKey[32]{};
  const auto original = testCard.at("/test.aurora");
  fingerprintFixture(original, expectedFingerprint);
  assert(deriveKey(password, original.data() + SALT_OFFSET, SALT_SIZE,
                   getLe32(original.data() + 12), independentKey));
  assert(readAuroraWalletFileChecked("test", password, opened, access) == AuroraWalletReadResult::Ok);
  assert(!memcmp(access, expectedFingerprint, sizeof(expectedFingerprint)));
  assert(!strcmp(opened.accountXprv, fixture.accountXprv));
  wipeAuroraWalletData(opened);
  assert(readAuroraWalletFileChecked("test", password, opened, nullptr, access) == AuroraWalletReadResult::Ok);
  assert(!strcmp(opened.mnemonic, fixture.mnemonic));

  assert(readAuroraWalletFileChecked("test", "wrong-password-only", opened, nullptr, access) == AuroraWalletReadResult::AuthenticationFailed);
  assert(allZero(opened));
  expectedFingerprint[0] ^= 1;
  assert(readAuroraWalletFileChecked("test", password, opened, nullptr, expectedFingerprint) == AuroraWalletReadResult::AuthenticationFailed);
  assert(allZero(opened));
  assert(readAuroraWalletFileChecked("test", nullptr, opened, nullptr, access) == AuroraWalletReadResult::WeakPassword);
  assert(allZero(opened));

  // A different valid file, even with the same password, cannot replace the
  // file bound to an already-open public session.
  char path[80]{};
  assert(writeWalletExportFile(WalletExportFormat::AuroraWallet, "replacement", password, fixture, path, sizeof(path)) == WalletExportResult::Ok);
  testCard["/test.aurora"] = testCard.at("/replacement.aurora");
  assert(readAuroraWalletFileChecked("test", password, opened, nullptr, access) == AuroraWalletReadResult::AuthenticationFailed);
  assert(allZero(opened));
  // Fingerprint matching is not a replacement for authenticating the GCM tag.
  for (size_t offset : {size_t(16), size_t(32), size_t(100), original.size() - 1}) {
    testCard["/test.aurora"] = original;
    testCard["/test.aurora"][offset] ^= 1;
    fingerprintFixture(testCard.at("/test.aurora"), expectedFingerprint);
    assert(readAuroraWalletFileChecked("test", password, opened, nullptr, expectedFingerprint) == AuroraWalletReadResult::AuthenticationFailed);
    assert(allZero(opened));
  }
  for (size_t offset : {size_t(0), size_t(8), size_t(9), size_t(10), size_t(11), size_t(12), size_t(44)}) {
    testCard["/test.aurora"] = original;
    testCard["/test.aurora"][offset] ^= 0x80;
    if (offset == 12) putLe32(testCard["/test.aurora"].data() + 12, KDF_ITERATIONS_MAX + 1);
    assert(readAuroraWalletFileChecked("test", password, opened, nullptr, access) == AuroraWalletReadResult::InvalidFormat);
    assert(allZero(opened));
  }
  testCard["/test.aurora"] = original; testCard["/test.aurora"].pop_back();
  assert(readAuroraWalletFileChecked("test", password, opened, nullptr, access) == AuroraWalletReadResult::InvalidFormat);
  assert(allZero(opened));
  testCard["/test.aurora"] = original; testCard["/test.aurora"].push_back(0);
  assert(readAuroraWalletFileChecked("test", password, opened, nullptr, access) == AuroraWalletReadResult::InvalidFormat);
  assert(allZero(opened));
  testCard["/test.aurora"] = original;
  testAppendAfterHeader = true;
  assert(readAuroraWalletFileChecked("test", password, opened, nullptr, access) == AuroraWalletReadResult::InvalidFormat);
  assert(allZero(opened));
  testCard["/test.aurora"] = original;

  // Every failure must erase pre-existing outputs, including failures before
  // mounting/reading the card. No file key is returned by the public API.
  struct Failure { const char *name; const char *password; AuroraWalletReadResult result; };
  for (const auto failure : {
      Failure{"../test", password, AuroraWalletReadResult::InvalidName},
      Failure{"test", "short", AuroraWalletReadResult::WeakPassword},
      Failure{"missing", password, AuroraWalletReadResult::NotFound},
      Failure{"test", "not-the-right-password", AuroraWalletReadResult::AuthenticationFailed}}) {
    memset(&opened, 0xa5, sizeof(opened)); memset(&access, 0xa5, sizeof(access));
    assert(readAuroraWalletFileChecked(failure.name, failure.password, opened, access) == failure.result);
    assert(allZero(opened) && allZero(access));
  }
  for (unsigned fault = 0; fault < 3; ++fault) {
    memset(&opened, 0xa5, sizeof(opened)); memset(&access, 0xa5, sizeof(access));
    testCardReady = fault != 0; testReadOpenOk = fault != 1;
    testReadLimit = fault == 2 ? 8 : static_cast<size_t>(-1);
    const auto expected = fault == 0 ? AuroraWalletReadResult::NoCard :
                          fault == 1 ? AuroraWalletReadResult::OpenFailed : AuroraWalletReadResult::ReadFailed;
    assert(readAuroraWalletFileChecked("test", password, opened, access) == expected);
    assert(allZero(opened) && allZero(access));
    testCardReady = true; testReadOpenOk = true; testReadLimit = static_cast<size_t>(-1);
  }

  assert(writeAuroraWalletFileVerified("session-write", password, fixture, path, sizeof(path), access) == WalletExportResult::Ok);
  assert(!strcmp(path, "/session-write.aurora"));
  fingerprintFixture(testCard.at(path), expectedFingerprint);
  assert(!memcmp(access, expectedFingerprint, sizeof(expectedFingerprint)));
  assert(readAuroraWalletFileChecked("session-write", password, opened, nullptr, access) == AuroraWalletReadResult::Ok);
  assert(!strcmp(opened.mnemonic, fixture.mnemonic));
  const auto written = testCard.at(path);
  assert(writeAuroraWalletFileVerified("session-write", password, fixture, path, sizeof(path), access) == WalletExportResult::AlreadyExists);
  assert(allZero(access) && !path[0] && testCard.at("/session-write.aurora") == written);

  testBeforeReadOpen = [](const char *filePath) { testCard.at(filePath).back() ^= 1; };
  memset(&access, 0xa5, sizeof(access));
  assert(writeAuroraWalletFileVerified("session-tamper", password, fixture, path, sizeof(path), access) == WalletExportResult::WriteFailed);
  assert(allZero(access) && !path[0] && testCard.count("/session-tamper.aurora"));
  testBeforeReadOpen = nullptr;
  testReadOpenOk = false;
  memset(&access, 0xa5, sizeof(access));
  assert(writeAuroraWalletFileVerified("session-open-error", password, fixture, path, sizeof(path), access) == WalletExportResult::WriteFailed);
  assert(allZero(access) && !path[0] && testCard.count("/session-open-error.aurora"));
  testReadOpenOk = true;

  // Substitution by another authentic wallet with the same password must also
  // fail read-back validation, without deleting either encrypted backup.
  auto different = fixture; different.mnemonic = "public different test mnemonic";
  assert(writeWalletExportFile(WalletExportFormat::AuroraWallet, "different", password, different, path, sizeof(path)) == WalletExportResult::Ok);
  testBeforeReadOpen = [](const char *filePath) { testCard.at(filePath) = testCard.at("/different.aurora"); };
  memset(&access, 0xa5, sizeof(access));
  assert(writeAuroraWalletFileVerified("session-substituted", password, fixture, path, sizeof(path), access) == WalletExportResult::WriteFailed);
  assert(allZero(access) && !path[0] && testCard.count("/session-substituted.aurora") && testCard.count("/different.aurora"));
  testBeforeReadOpen = nullptr;
  assert(testCard.at("/test.aurora") == original);
  assert(testMounts == testUnmounts);
  wipeAuroraWalletData(opened); secureZero(access,sizeof(access));
  secureZero(expectedFingerprint, sizeof(expectedFingerprint));
  secureZero(independentKey, sizeof(independentKey));
  assert(allZero(access) && allZero(opened));
  puts("PASS: password-only reread, no key output, exact SHA256 binding, wrong-password/tag/replacement/format rejection, failure-output wipe, authenticated read-back and preservation of written backups");
}
int main() {
  testKdfPolicy();
  {
    uint8_t scryptKey[32]{};
    constexpr uint8_t expectedKey[32]={0x40,0x4b,0x5a,0xb5,0x54,0x04,0xc3,0xc3,0x4d,0x3b,0x13,0x0b,0x07,0x26,0x2d,0xe7,0xbe,0xd1,0x84,0x1f,0x22,0x19,0x3b,0xd7,0xfc,0x0c,0x38,0x3f,0x07,0x26,0x76,0xfb};
    assert(scryptAezeed(reinterpret_cast<const uint8_t *>("aezeed"),6,reinterpret_cast<const uint8_t *>("salt1"),scryptKey)==ScryptResult::Ok);
    assert(!memcmp(scryptKey,expectedKey,sizeof(expectedKey)));
    uint8_t extracted[48]{};
    constexpr uint8_t expectedExtracted[48]={0xe7,0xa3,0x7c,0x73,0xe0,0x5b,0x7e,0x44,0xc1,0xb7,0x6c,0x25,0x08,0xd9,0x0e,0x2a,0x96,0xca,0xad,0x37,0x9b,0x2a,0x38,0xa3,0x1e,0x1d,0x9f,0xa7,0xcf,0x71,0x25,0x4b,0x5f,0xfe,0xa9,0x85,0x83,0xb7,0x13,0xa4,0x97,0x00,0x94,0x89,0xcf,0xba,0xf9,0x0f};
    blake2b48(scryptKey,sizeof(scryptKey),extracted);
    assert(!memcmp(extracted,expectedExtracted,sizeof(expectedExtracted)));
    uint8_t zeroBlock[16]{},roundBlock[16]{};uint32_t q[8]{},zeroKey[8]{};
    loadBlock(q,zeroBlock);aesRound(q,zeroKey);storeBlock(roundBlock,q);
    for(uint8_t byte:roundBlock)assert(byte==0x63);
    // Production-parameter vector (N=32768), generated independently with
    // libscrypt plus the public-domain AEZ v5 reference implementation.
    constexpr const char *words =
        "above judge emerge veteran reform crunch system all snap please shoulder vault "
        "hurt city quarter cover enlist swear success suggest drink wagon enrich body";
    constexpr uint8_t expectedEntropy[16] = {
        0x81,0xb6,0x37,0xd8,0x63,0x59,0xe6,0x96,
        0x0d,0xe7,0x95,0xe4,0x1e,0x0b,0x4c,0xfd};
    AezeedDecoded decoded{};
    assert(AezeedEngine::decode(words,"",decoded)==AezeedResult::Ok);
    assert(decoded.internalVersion==0 && decoded.birthdayDays==0);
    assert(!memcmp(decoded.entropy,expectedEntropy,sizeof(expectedEntropy)));
    assert(AezeedEngine::decode(words,"wrong",decoded)==AezeedResult::InvalidPassphrase);
    const AezeedDecoded empty{};
    assert(!memcmp(&decoded,&empty,sizeof(decoded)));

    // Independently generated with Node's scrypt and the public AEZ v5
    // reference-compatible implementation. LND now creates internal version
    // 1 seeds (KeyDerivationVersionTaproot), while external AEZEED remains 0.
    constexpr const char *taprootWords =
        "able garment pave cook junk lab toe major great husband sun letter left vivid "
        "abandon remain empower process success suggest drink lock fitness kitten";
    constexpr uint8_t taprootEntropy[16] = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
        0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    assert(AezeedEngine::decode(taprootWords,"",decoded)==AezeedResult::Ok);
    assert(decoded.internalVersion==1 && decoded.birthdayDays==6000);
    assert(!memcmp(decoded.entropy,taprootEntropy,sizeof(taprootEntropy)));
    AezeedEngine::wipe(decoded);
    assert(!memcmp(&decoded,&empty,sizeof(decoded)));

    constexpr const char *unknownInternalVersionWords =
        "ability cotton because large auto push hospital faculty suspect cluster behind group "
        "chapter word birth jazz own twist success suggest drink you session purpose";
    assert(AezeedEngine::decode(unknownInternalVersionWords,"",decoded)==AezeedResult::UnsupportedVersion);
    assert(!memcmp(&decoded,&empty,sizeof(decoded)));
  }
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
  assert(original[9]==KDF_PBKDF2_HMAC_SHA256 && getLe32(original.data()+12)==KDF_ITERATIONS);
  testSessionAccess(fixture, filePassword);
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
  assert(deriveKey(filePassword, original.data() + SALT_OFFSET, SALT_SIZE, 120000, releasedKey));
  auto &released = testCard.at("/test.aurora");
  released.resize(HEADER_SIZE+sizeof(releasedPayload)+TAG_SIZE);
  memcpy(released.data(),FILE_MAGIC,sizeof(FILE_MAGIC)); released[8]=FILE_VERSION;
  putLe32(released.data()+12,120000);
  putLe16(released.data()+44,sizeof(releasedPayload));
  assert(aesGcmEncrypt(releasedKey, released.data() + NONCE_OFFSET, released.data(), HEADER_SIZE,
      reinterpret_cast<const uint8_t *>(&releasedPayload), sizeof(releasedPayload),
      released.data() + HEADER_SIZE, released.data() + HEADER_SIZE + sizeof(releasedPayload)));
  assert(readAuroraWalletFile("test", filePassword, restored) == AuroraWalletReadResult::Ok);
  assert(!strcmp(restored.mnemonic, fixture.mnemonic));
  assert(restored.fileVersion==1 && !auroraPinRecordValid(restored.pin));
  uint8_t legacyAccess[32]{};
  assert(readAuroraWalletFileChecked("test", filePassword, restored, legacyAccess) == AuroraWalletReadResult::Ok);
  uint8_t legacyFingerprint[32]{}; fingerprintFixture(released, legacyFingerprint);
  assert(!memcmp(legacyAccess, legacyFingerprint, sizeof(legacyFingerprint)));
  assert(readAuroraWalletFileChecked("test", filePassword, restored, nullptr, legacyAccess) == AuroraWalletReadResult::Ok);
  assert(restored.fileVersion == 1 && !strcmp(restored.mnemonic, fixture.mnemonic));
  secureZero(legacyAccess,sizeof(legacyAccess)); secureZero(legacyFingerprint, sizeof(legacyFingerprint));
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
  secureZero(releasedKey, sizeof(releasedKey));
  assert(deriveKey(filePassword, noPin.data()+SALT_OFFSET, SALT_SIZE,
                   getLe32(noPin.data()+12), releasedKey));
  assert(aesGcmEncrypt(releasedKey,noPin.data()+NONCE_OFFSET,noPin.data(),HEADER_SIZE,
      reinterpret_cast<const uint8_t *>(&invalidPayload),sizeof(invalidPayload),
      noPin.data()+HEADER_SIZE,noPin.data()+HEADER_SIZE+sizeof(invalidPayload)));
  assert(readAuroraWalletFile("test",filePassword,restored)==AuroraWalletReadResult::InvalidFormat);
  assert(!memcmp(&restored,zeros.data(),zeros.size()));
#if defined(AURORA_BOARD_P4)
  uint8_t invalidFingerprint[32]{}; fingerprintFixture(noPin, invalidFingerprint);
  assert(readAuroraWalletFileChecked("test", filePassword, restored, nullptr, invalidFingerprint) == AuroraWalletReadResult::InvalidFormat);
  assert(allZero(restored));
  uint8_t invalidAccess[32]{}; memset(&invalidAccess, 0xa5, sizeof(invalidAccess));
  assert(readAuroraWalletFileChecked("test", filePassword, restored, invalidAccess) == AuroraWalletReadResult::InvalidFormat);
  assert(allZero(restored) && allZero(invalidAccess));
  secureZero(invalidAccess,sizeof(invalidAccess)); secureZero(invalidFingerprint, sizeof(invalidFingerprint));
#endif
  for (size_t offset : {size_t(16), size_t(32), size_t(100), original.size() - 1}) {
    testCard["/test.aurora"] = original; testCard["/test.aurora"][offset] ^= 1;
    assert(readAuroraWalletFile("test", filePassword, restored) == AuroraWalletReadResult::AuthenticationFailed);
    assert(!memcmp(&restored, zeros.data(), zeros.size()));
  }
  testCard["/test.aurora"] = original; testCard["/test.aurora"].pop_back();
  assert(readAuroraWalletFile("test", filePassword, restored) == AuroraWalletReadResult::InvalidFormat);
  testCard["/test.aurora"] = original;
  auto missingPin=fixture; missingPin.pin=nullptr;
  uint8_t pinlessFingerprint[32]{};
  assert(writeAuroraWalletFileVerified("no-pin",filePassword,missingPin,path,sizeof(path),pinlessFingerprint)==WalletExportResult::Ok);
  assert(readAuroraWalletFileChecked("no-pin",filePassword,restored,nullptr,pinlessFingerprint)==AuroraWalletReadResult::Ok);
  assert(restored.fileVersion==1 && allZero(restored.pin));
  assert(!strcmp(restored.mnemonic,missingPin.mnemonic));
  assert(testCard.at("/no-pin.aurora").size()==HEADER_SIZE+sizeof(AuroraPayloadV1)+TAG_SIZE);
  assert(getLe32(testCard.at("/no-pin.aurora").data()+12)==KDF_ITERATIONS);
  wipeAuroraWalletData(restored);
  const WalletExportData umbrel{
      AURORA_WALLET_KIND_UMBREL,AURORA_WALLET_WORDS_UMBREL,
      AURORA_WALLET_TYPE_UMBREL,AURORA_WALLET_PATH_UMBREL,
      "","","birthday-days:4242",
      "xpub661MyMwAqRbc-public-umbrel-fixture",
      "xprv9s21ZrQH143K3-private-umbrel-fixture","","",nullptr};
  uint8_t umbrelFingerprint[32]{};
  assert(writeAuroraWalletFileVerified("umbrel",filePassword,umbrel,path,sizeof(path),umbrelFingerprint)==WalletExportResult::Ok);
  assert(!strcmp(path,"/umbrel.aurora") && !allZero(umbrelFingerprint));
  assert(readAuroraWalletFileChecked("umbrel",filePassword,restored,nullptr,umbrelFingerprint)==AuroraWalletReadResult::Ok);
  assert(restored.fileVersion==1 && restored.addressKind==AURORA_WALLET_KIND_UMBREL &&
         restored.wordCount==0 && !strcmp(restored.addressType,AURORA_WALLET_TYPE_UMBREL) &&
         !strcmp(restored.derivationPath,"m") && !strcmp(restored.address,"birthday-days:4242") &&
         !strcmp(restored.accountXpub,umbrel.accountXpub) &&
         !strcmp(restored.accountXprv,umbrel.accountXprv) &&
         !restored.mnemonic[0] && !restored.passphrase[0] &&
         !restored.privateWif[0] && !restored.receiveDescriptor[0]);
  wipeAuroraWalletData(restored);
  assert(writeWalletExportFile(WalletExportFormat::SparrowPrivate,"umbrel",nullptr,umbrel,path,sizeof(path))==WalletExportResult::Ok);
  assert(!strcmp(path,"/umbrel-sparrow.txt"));
  const std::string expectedSparrow=std::string(umbrel.accountXprv)+"\n";
  assert(testCard.at(path)==std::vector<uint8_t>(expectedSparrow.begin(),expectedSparrow.end()));
  auto invalidUmbrel=umbrel; invalidUmbrel.mnemonic="AEZEED words must never be serialized";
  assert(writeWalletExportFile(WalletExportFormat::AuroraWallet,"bad-umbrel",filePassword,invalidUmbrel,path,sizeof(path))==WalletExportResult::InvalidData);
  assert(writeWalletExportFile(WalletExportFormat::SparrowPrivate,"bad-sparrow",nullptr,fixture,path,sizeof(path))==WalletExportResult::InvalidData);
  assert(!testCard.count("/bad-umbrel.aurora") && !testCard.count("/bad-sparrow-sparrow.txt"));
  secureZero(umbrelFingerprint,sizeof(umbrelFingerprint));
  puts("PASS: Umbrel subtype stores only authenticated root BIP32 material; AEZEED words/passphrase stay absent; Sparrow export is exact plaintext xprv");
  testSyncOk = false;
  assert(writeWalletExportFile(WalletExportFormat::AuroraWallet, "failure", filePassword, fixture, path, sizeof(path)) == WalletExportResult::WriteFailed);
  assert(!testCard.count("/failure.aurora") && testCard.at("/test.aurora") == original);
#if defined(AURORA_BOARD_P4)
  testSyncOk = true;
  for (bool closeFailure : {true, false}) {
    testCloseOk = !closeFailure;
    testEndOk = closeFailure;
    const char *base = closeFailure ? "close-error" : "unmount-error";
    uint8_t fingerprint[32]; memset(fingerprint, 0xA5, sizeof(fingerprint));
    strcpy(path, "stale-path");
    assert(writeAuroraWalletFileVerified(base, filePassword, fixture, path, sizeof(path), fingerprint)
           == WalletExportResult::FinalizeFailed);
    assert(path[0] == 0 && allZero(fingerprint));
    assert(testCard.count(std::string("/") + base + ".aurora"));
    assert(writeWalletExportFile(WalletExportFormat::ElectrumPrivate, base, nullptr, fixture, path, sizeof(path))
           == WalletExportResult::FinalizeFailed);
    assert(path[0] == 0 && testCard.count(std::string("/") + base + "-electrum.json"));
    testCloseOk = testEndOk = true;
    AuroraWalletData check{};
    assert(readAuroraWalletFile(base, filePassword, check) == AuroraWalletReadResult::Ok);
    wipeAuroraWalletData(check);
    assert(writeWalletExportFile(WalletExportFormat::AuroraWallet, base, filePassword, fixture, path, sizeof(path))
           == WalletExportResult::AlreadyExists);
  }
  assert(testCard.at("/test.aurora") == original);
  puts("PASS: P4 close/unmount failures never return success for Aurora/Electrum; uncertain backups preserved, outputs cleared, no overwrite");
#endif
  puts("PASS: read-only SD presence/root check accepts empty media, rejects absent/unreadable media, no-card export creates nothing");
  printf("PASS: selected writer KDF %lu, legacy V1 120000 read and V2 round trip, legacy PIN record, tamper/wrong-password rejection, no overwrite, failed-sync cleanup\n", static_cast<unsigned long>(KDF_ITERATIONS));
}
