#include "sd_export.h"

#include "board_config.h"
#include "hardware_rng.h"
#include "secure_memory.h"
#include "version.h"

#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <mbedtls/gcm.h>
#include <mbedtls/md.h>
#include <mbedtls/pkcs5.h>
#include <new>

namespace {
constexpr uint8_t FILE_MAGIC[8] = {'A', 'U', 'R', 'O', 'R', 'A', 'W', '1'};
constexpr uint8_t PAYLOAD_MAGIC[8] = {'A', 'U', 'R', 'D', 'A', 'T', '0', '1'};
constexpr uint8_t FILE_VERSION = 1;
constexpr uint8_t KDF_PBKDF2_HMAC_SHA256 = 1;
constexpr uint8_t CIPHER_AES_256_GCM = 1;
constexpr uint32_t KDF_ITERATIONS = 120000;
constexpr uint32_t KDF_ITERATIONS_MIN = 10000;
constexpr uint32_t KDF_ITERATIONS_MAX = 500000;
constexpr size_t HEADER_SIZE = 46;
constexpr size_t SALT_OFFSET = 16;
constexpr size_t SALT_SIZE = 16;
constexpr size_t NONCE_OFFSET = 32;
constexpr size_t NONCE_SIZE = 12;
constexpr size_t TAG_SIZE = 16;
constexpr size_t KEY_SIZE = 32;

#pragma pack(push, 1)
struct AuroraPayloadV1 {
  uint8_t magic[8];
  uint8_t addressKind;
  uint8_t wordCount;
  char firmwareVersion[16];
  char addressType[40];
  char derivationPath[32];
  char mnemonic[256];
  char passphrase[64];
  char address[96];
  char accountXpub[128];
  char accountXprv[128];
  char privateWif[64];
  char receiveDescriptor[224];
};
#pragma pack(pop)

static_assert(sizeof(AuroraPayloadV1) < 65536, "Aurora Wallet payload is too large");

bool validBaseName(const char *name) {
  if (!name) return false;
  const size_t length = strlen(name);
  if (length == 0 || length > 24) return false;
  for (size_t i = 0; i < length; ++i) {
    const char c = name[i];
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || c == '_' || c == '-')) {
      return false;
    }
  }
  return true;
}

bool equalsIgnoreCase(char left, char right) {
  if (left >= 'A' && left <= 'Z') left = static_cast<char>(left - 'A' + 'a');
  if (right >= 'A' && right <= 'Z') right = static_cast<char>(right - 'A' + 'a');
  return left == right;
}

bool auroraFileBaseName(const char *path, char (&baseName)[25]) {
  if (!path) return false;
  const char *name = strrchr(path, '/');
  name = name ? name + 1 : path;
  const size_t length = strlen(name);
  static constexpr char SUFFIX[] = ".aurora";
  constexpr size_t SUFFIX_LENGTH = sizeof(SUFFIX) - 1;
  if (length <= SUFFIX_LENGTH || length - SUFFIX_LENGTH >= sizeof(baseName)) return false;
  for (size_t i = 0; i < SUFFIX_LENGTH; ++i) {
    if (!equalsIgnoreCase(name[length - SUFFIX_LENGTH + i], SUFFIX[i])) return false;
  }
  const size_t baseLength = length - SUFFIX_LENGTH;
  memcpy(baseName, name, baseLength);
  baseName[baseLength] = '\0';
  return validBaseName(baseName);
}

bool validPassword(const char *password) {
  if (!password) return false;
  const size_t length = strlen(password);
  if (length < AURORA_WALLET_MIN_PASSWORD_LENGTH || length > 63) return false;
  for (size_t i = 0; i < length; ++i) {
    const uint8_t c = static_cast<uint8_t>(password[i]);
    if (c < 0x20 || c > 0x7e) return false;
  }
  return true;
}

bool present(const char *value) {
  return value && value[0];
}

bool supportedWordCount(uint8_t words) {
  return words == 12 || words == 15 || words == 18 || words == 21 || words == 24;
}

template <size_t N>
bool copyChecked(char (&destination)[N], const char *source, bool allowEmpty = false) {
  if (!source || (!allowEmpty && !source[0])) return false;
  return strlcpy(destination, source, N) < N;
}

template <size_t N>
bool terminated(const char (&value)[N]) {
  return memchr(value, '\0', N) != nullptr;
}

void putLe16(uint8_t *destination, uint16_t value) {
  destination[0] = static_cast<uint8_t>(value);
  destination[1] = static_cast<uint8_t>(value >> 8);
}

uint16_t getLe16(const uint8_t *source) {
  return static_cast<uint16_t>(source[0]) |
         static_cast<uint16_t>(source[1]) << 8;
}

void putLe32(uint8_t *destination, uint32_t value) {
  destination[0] = static_cast<uint8_t>(value);
  destination[1] = static_cast<uint8_t>(value >> 8);
  destination[2] = static_cast<uint8_t>(value >> 16);
  destination[3] = static_cast<uint8_t>(value >> 24);
}

uint32_t getLe32(const uint8_t *source) {
  return static_cast<uint32_t>(source[0]) |
         static_cast<uint32_t>(source[1]) << 8 |
         static_cast<uint32_t>(source[2]) << 16 |
         static_cast<uint32_t>(source[3]) << 24;
}

bool deriveKey(const char *password, const uint8_t *salt, size_t saltLength,
               uint32_t iterations, uint8_t key[KEY_SIZE]) {
  if (!password || !salt || saltLength == 0 || !key || iterations == 0) return false;
  bool ok = false;
  mbedtls_md_context_t context;
  mbedtls_md_init(&context);
  const mbedtls_md_info_t *sha256 = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (sha256 && mbedtls_md_setup(&context, sha256, 1) == 0 &&
      mbedtls_pkcs5_pbkdf2_hmac(
          &context, reinterpret_cast<const unsigned char *>(password), strlen(password),
          salt, saltLength, iterations, KEY_SIZE, key) == 0) {
    ok = true;
  }
  mbedtls_md_free(&context);
  if (!ok) secureZero(key, KEY_SIZE);
  return ok;
}

bool aesGcmEncrypt(const uint8_t key[KEY_SIZE], const uint8_t nonce[NONCE_SIZE],
                   const uint8_t *aad, size_t aadLength, const uint8_t *plain,
                   size_t length, uint8_t *cipher, uint8_t tag[TAG_SIZE]) {
  mbedtls_gcm_context context;
  mbedtls_gcm_init(&context);
  const bool ok = mbedtls_gcm_setkey(&context, MBEDTLS_CIPHER_ID_AES,
                                    key, KEY_SIZE * 8) == 0 &&
                  mbedtls_gcm_crypt_and_tag(
                      &context, MBEDTLS_GCM_ENCRYPT, length, nonce, NONCE_SIZE,
                      aad, aadLength, plain, cipher, TAG_SIZE, tag) == 0;
  mbedtls_gcm_free(&context);
  return ok;
}

bool aesGcmDecrypt(const uint8_t key[KEY_SIZE], const uint8_t nonce[NONCE_SIZE],
                   const uint8_t *aad, size_t aadLength, const uint8_t *cipher,
                   size_t length, const uint8_t tag[TAG_SIZE], uint8_t *plain) {
  mbedtls_gcm_context context;
  mbedtls_gcm_init(&context);
  const bool ok = mbedtls_gcm_setkey(&context, MBEDTLS_CIPHER_ID_AES,
                                    key, KEY_SIZE * 8) == 0 &&
                  mbedtls_gcm_auth_decrypt(
                      &context, length, nonce, NONCE_SIZE, aad, aadLength,
                      tag, TAG_SIZE, cipher, plain) == 0;
  mbedtls_gcm_free(&context);
  if (!ok) secureZero(plain, length);
  return ok;
}

bool writeText(File &file, const char *text) {
  if (!text) return false;
  const size_t length = strlen(text);
  return file.write(reinterpret_cast<const uint8_t *>(text), length) == length;
}

bool writeJsonString(File &file, const char *text) {
  if (!text || !writeText(file, "\"")) return false;
  for (const unsigned char *p = reinterpret_cast<const unsigned char *>(text); *p; ++p) {
    if (*p == '\"' || *p == '\\') {
      const uint8_t escaped[2] = {'\\', *p};
      if (file.write(escaped, sizeof(escaped)) != sizeof(escaped)) return false;
    } else if (*p < 0x20) {
      char escaped[7] = {};
      snprintf(escaped, sizeof(escaped), "\\u%04x", *p);
      if (!writeText(file, escaped)) return false;
    } else if (file.write(p, 1) != 1) {
      return false;
    }
  }
  return writeText(file, "\"");
}

bool writeElectrum(File &file, const WalletExportData &data) {
  return writeText(file, "{\n  \"keystore\": {\n    \"xpub\": ") &&
         writeJsonString(file, data.accountXpub) &&
         writeText(file, ",\n    \"xprv\": ") &&
         writeJsonString(file, data.accountXprv) &&
         writeText(file,
             ",\n    \"type\": \"bip32\",\n    \"pw_hash_version\": 1\n"
             "  },\n  \"wallet_type\": \"standard\",\n"
             "  \"use_encryption\": false,\n  \"seed_type\": \"bip39\"\n}\n");
}

bool fillPayload(const WalletExportData &data, AuroraPayloadV1 &payload) {
  memset(&payload, 0, sizeof(payload));
  memcpy(payload.magic, PAYLOAD_MAGIC, sizeof(payload.magic));
  payload.addressKind = data.addressKind;
  payload.wordCount = data.wordCount;
  return data.addressKind <= 3 && supportedWordCount(data.wordCount) &&
         copyChecked(payload.firmwareVersion, AURORA_FIRMWARE_VERSION) &&
         copyChecked(payload.addressType, data.addressType) &&
         copyChecked(payload.derivationPath, data.derivationPath) &&
         copyChecked(payload.mnemonic, data.mnemonic) &&
         copyChecked(payload.passphrase, data.passphrase, true) &&
         copyChecked(payload.address, data.address) &&
         copyChecked(payload.accountXpub, data.accountXpub) &&
         copyChecked(payload.accountXprv, data.accountXprv) &&
         copyChecked(payload.privateWif, data.privateWif) &&
         copyChecked(payload.receiveDescriptor, data.receiveDescriptor);
}

bool validPayload(const AuroraPayloadV1 &payload) {
  return memcmp(payload.magic, PAYLOAD_MAGIC, sizeof(payload.magic)) == 0 &&
         payload.addressKind <= 3 && supportedWordCount(payload.wordCount) &&
         terminated(payload.firmwareVersion) && terminated(payload.addressType) &&
         terminated(payload.derivationPath) && terminated(payload.mnemonic) &&
         terminated(payload.passphrase) && terminated(payload.address) &&
         terminated(payload.accountXpub) && terminated(payload.accountXprv) &&
         terminated(payload.privateWif) && terminated(payload.receiveDescriptor) &&
         present(payload.addressType) && present(payload.derivationPath) &&
         present(payload.mnemonic) && present(payload.address) &&
         present(payload.accountXpub) && present(payload.accountXprv) &&
         present(payload.privateWif) && present(payload.receiveDescriptor);
}

bool writeAuroraWallet(File &file, const char *password,
                       const WalletExportData &data) {
  AuroraPayloadV1 payload{};
  uint8_t header[HEADER_SIZE] = {};
  uint8_t key[KEY_SIZE] = {};
  uint8_t tag[TAG_SIZE] = {};
  uint8_t *cipher = nullptr;
  bool ok = false;

  if (!fillPayload(data, payload)) goto cleanup;
  cipher = new (std::nothrow) uint8_t[sizeof(payload)];
  if (!cipher) goto cleanup;

  memcpy(header, FILE_MAGIC, sizeof(FILE_MAGIC));
  header[8] = FILE_VERSION;
  header[9] = KDF_PBKDF2_HMAC_SHA256;
  header[10] = CIPHER_AES_256_GCM;
  putLe32(header + 12, KDF_ITERATIONS);
  hardwareRandomFill(header + SALT_OFFSET, SALT_SIZE);
  hardwareRandomFill(header + NONCE_OFFSET, NONCE_SIZE);
  putLe16(header + 44, static_cast<uint16_t>(sizeof(payload)));

  delay(1);
  if (!deriveKey(password, header + SALT_OFFSET, SALT_SIZE,
                 KDF_ITERATIONS, key)) goto cleanup;
  if (!aesGcmEncrypt(key, header + NONCE_OFFSET, header, sizeof(header),
                     reinterpret_cast<const uint8_t *>(&payload), sizeof(payload),
                     cipher, tag)) goto cleanup;
  ok = file.write(header, sizeof(header)) == sizeof(header) &&
       file.write(cipher, sizeof(payload)) == sizeof(payload) &&
       file.write(tag, sizeof(tag)) == sizeof(tag);

cleanup:
  secureZero(&payload, sizeof(payload));
  secureZero(header, sizeof(header));
  secureZero(key, sizeof(key));
  secureZero(tag, sizeof(tag));
  if (cipher) {
    secureZero(cipher, sizeof(AuroraPayloadV1));
    delete[] cipher;
  }
  return ok;
}

bool validElectrumData(const WalletExportData &data) {
  return present(data.accountXpub) && present(data.accountXprv);
}

bool validAuroraData(const WalletExportData &data) {
  return data.addressKind <= 3 && supportedWordCount(data.wordCount) &&
         present(data.addressType) && present(data.derivationPath) &&
         present(data.mnemonic) && data.passphrase && present(data.address) &&
         present(data.accountXpub) && present(data.accountXprv) &&
         present(data.privateWif) && present(data.receiveDescriptor);
}

bool beginSd(SPIClass &sdSpi) {
  pinMode(AURORA_SD_CS, OUTPUT);
  digitalWrite(AURORA_SD_CS, HIGH);
  sdSpi.begin(AURORA_SD_CLK, AURORA_SD_MISO, AURORA_SD_MOSI, AURORA_SD_CS);
  return SD.begin(AURORA_SD_CS, sdSpi, AURORA_SD_FREQUENCY) &&
         SD.cardType() != CARD_NONE;
}

void endSd(SPIClass &sdSpi) {
  SD.end();
  sdSpi.end();
}
}

const char *walletExportSuffix(WalletExportFormat format) {
  switch (format) {
    case WalletExportFormat::ElectrumPrivate: return "-electrum.json";
    case WalletExportFormat::AuroraWallet: return ".aurora";
  }
  return "";
}

WalletExportResult writeWalletExportFile(WalletExportFormat format,
                                         const char *baseName,
                                         const char *filePassword,
                                         const WalletExportData &data,
                                         char *writtenPath,
                                         size_t writtenPathLength) {
  if (writtenPath && writtenPathLength) writtenPath[0] = '\0';
  if (!validBaseName(baseName)) return WalletExportResult::InvalidName;
  if (format == WalletExportFormat::ElectrumPrivate && !validElectrumData(data)) {
    return WalletExportResult::InvalidData;
  }
  if (format == WalletExportFormat::AuroraWallet) {
    if (!validAuroraData(data)) return WalletExportResult::InvalidData;
    if (!validPassword(filePassword)) return WalletExportResult::WeakPassword;
  }

  char path[56] = {};
  const int pathLength = snprintf(path, sizeof(path), "/%s%s", baseName,
                                  walletExportSuffix(format));
  if (pathLength <= 0 || static_cast<size_t>(pathLength) >= sizeof(path)) {
    return WalletExportResult::InvalidName;
  }

  SPIClass sdSpi(HSPI);
  if (!beginSd(sdSpi)) {
    endSd(sdSpi);
    return WalletExportResult::NoCard;
  }
  if (SD.exists(path)) {
    endSd(sdSpi);
    return WalletExportResult::AlreadyExists;
  }

  File file = SD.open(path, FILE_WRITE);
  if (!file) {
    endSd(sdSpi);
    return WalletExportResult::OpenFailed;
  }

  const bool ok = format == WalletExportFormat::ElectrumPrivate
                      ? writeElectrum(file, data)
                      : writeAuroraWallet(file, filePassword, data);
  file.flush();
  file.close();
  if (!ok) SD.remove(path);
  endSd(sdSpi);

  if (!ok) return WalletExportResult::WriteFailed;
  if (writtenPath && writtenPathLength) strlcpy(writtenPath, path, writtenPathLength);
  return WalletExportResult::Ok;
}

AuroraWalletReadResult readAuroraWalletFile(const char *baseName,
                                            const char *filePassword,
                                            AuroraWalletData &data) {
  wipeAuroraWalletData(data);
  if (!validBaseName(baseName)) return AuroraWalletReadResult::InvalidName;
  if (!validPassword(filePassword)) return AuroraWalletReadResult::WeakPassword;

  char path[56] = {};
  const int pathLength = snprintf(path, sizeof(path), "/%s.aurora", baseName);
  if (pathLength <= 0 || static_cast<size_t>(pathLength) >= sizeof(path)) {
    return AuroraWalletReadResult::InvalidName;
  }

  SPIClass sdSpi(HSPI);
  if (!beginSd(sdSpi)) {
    endSd(sdSpi);
    return AuroraWalletReadResult::NoCard;
  }
  if (!SD.exists(path)) {
    endSd(sdSpi);
    return AuroraWalletReadResult::NotFound;
  }

  File file = SD.open(path, FILE_READ);
  if (!file) {
    endSd(sdSpi);
    return AuroraWalletReadResult::OpenFailed;
  }

  uint8_t header[HEADER_SIZE] = {};
  uint8_t tag[TAG_SIZE] = {};
  uint8_t key[KEY_SIZE] = {};
  AuroraPayloadV1 payload{};
  uint8_t *cipher = nullptr;
  AuroraWalletReadResult result = AuroraWalletReadResult::ReadFailed;

  if (file.read(header, sizeof(header)) != sizeof(header)) goto cleanup_file;
  if (memcmp(header, FILE_MAGIC, sizeof(FILE_MAGIC)) != 0 ||
      header[8] != FILE_VERSION || header[9] != KDF_PBKDF2_HMAC_SHA256 ||
      header[10] != CIPHER_AES_256_GCM || header[11] != 0) {
    result = AuroraWalletReadResult::InvalidFormat;
    goto cleanup_file;
  }
  {
    const uint32_t iterations = getLe32(header + 12);
    const uint16_t cipherLength = getLe16(header + 44);
    const size_t expectedSize = HEADER_SIZE + cipherLength + TAG_SIZE;
    if (iterations < KDF_ITERATIONS_MIN || iterations > KDF_ITERATIONS_MAX ||
        cipherLength != sizeof(payload) || file.size() != expectedSize) {
      result = AuroraWalletReadResult::InvalidFormat;
      goto cleanup_file;
    }
    cipher = new (std::nothrow) uint8_t[cipherLength];
    if (!cipher) {
      result = AuroraWalletReadResult::MemoryFailed;
      goto cleanup_file;
    }
    if (file.read(cipher, cipherLength) != cipherLength ||
        file.read(tag, sizeof(tag)) != sizeof(tag)) {
      result = AuroraWalletReadResult::ReadFailed;
      goto cleanup_file;
    }
    file.close();
    endSd(sdSpi);
    delay(1);
    if (!deriveKey(filePassword, header + SALT_OFFSET, SALT_SIZE, iterations, key) ||
        !aesGcmDecrypt(key, header + NONCE_OFFSET, header, sizeof(header),
                       cipher, cipherLength, tag,
                       reinterpret_cast<uint8_t *>(&payload))) {
      result = AuroraWalletReadResult::AuthenticationFailed;
      goto cleanup_memory;
    }
  }

  if (!validPayload(payload)) {
    result = AuroraWalletReadResult::InvalidFormat;
    goto cleanup_memory;
  }
  data.addressKind = payload.addressKind;
  data.wordCount = payload.wordCount;
  strlcpy(data.addressType, payload.addressType, sizeof(data.addressType));
  strlcpy(data.derivationPath, payload.derivationPath, sizeof(data.derivationPath));
  strlcpy(data.mnemonic, payload.mnemonic, sizeof(data.mnemonic));
  strlcpy(data.passphrase, payload.passphrase, sizeof(data.passphrase));
  strlcpy(data.address, payload.address, sizeof(data.address));
  strlcpy(data.accountXpub, payload.accountXpub, sizeof(data.accountXpub));
  strlcpy(data.accountXprv, payload.accountXprv, sizeof(data.accountXprv));
  strlcpy(data.privateWif, payload.privateWif, sizeof(data.privateWif));
  strlcpy(data.receiveDescriptor, payload.receiveDescriptor, sizeof(data.receiveDescriptor));
  result = AuroraWalletReadResult::Ok;
  goto cleanup_memory;

cleanup_file:
  file.close();
  endSd(sdSpi);
cleanup_memory:
  secureZero(header, sizeof(header));
  secureZero(tag, sizeof(tag));
  secureZero(key, sizeof(key));
  secureZero(&payload, sizeof(payload));
  if (cipher) {
    secureZero(cipher, sizeof(AuroraPayloadV1));
    delete[] cipher;
  }
  if (result != AuroraWalletReadResult::Ok) wipeAuroraWalletData(data);
  return result;
}

AuroraWalletListResult listAuroraWalletFiles(char *options,
                                             size_t optionsLength,
                                             uint16_t &fileCount) {
  fileCount = 0;
  if (!options || optionsLength == 0) return AuroraWalletListResult::BufferTooSmall;
  options[0] = '\0';

  SPIClass sdSpi(HSPI);
  if (!beginSd(sdSpi)) {
    endSd(sdSpi);
    return AuroraWalletListResult::NoCard;
  }

  File root = SD.open("/");
  if (!root || !root.isDirectory()) {
    if (root) root.close();
    endSd(sdSpi);
    return AuroraWalletListResult::OpenFailed;
  }

  size_t used = 0;
  bool bufferTooSmall = false;
  File entry = root.openNextFile();
  while (entry) {
    if (!entry.isDirectory()) {
      char baseName[25] = {};
      if (auroraFileBaseName(entry.name(), baseName)) {
        const size_t length = strlen(baseName);
        const size_t separator = fileCount ? 1 : 0;
        if (used + separator + length + 1 > optionsLength) {
          bufferTooSmall = true;
        } else {
          if (separator) options[used++] = '\n';
          memcpy(options + used, baseName, length);
          used += length;
          options[used] = '\0';
          ++fileCount;
        }
      }
      secureZero(baseName, sizeof(baseName));
    }
    entry.close();
    entry = root.openNextFile();
  }
  root.close();
  endSd(sdSpi);

  if (bufferTooSmall) return AuroraWalletListResult::BufferTooSmall;
  if (fileCount == 0) return AuroraWalletListResult::NoFiles;
  return AuroraWalletListResult::Ok;
}

void wipeAuroraWalletData(AuroraWalletData &data) {
  secureZero(&data, sizeof(data));
}

bool auroraWalletCryptoSelfTest() {
  static constexpr uint8_t PBKDF2_EXPECTED[32] = {
      0x12, 0x0f, 0xb6, 0xcf, 0xfc, 0xf8, 0xb3, 0x2c,
      0x43, 0xe7, 0x22, 0x52, 0x56, 0xc4, 0xf8, 0x37,
      0xa8, 0x65, 0x48, 0xc9, 0x2c, 0xcc, 0x35, 0x48,
      0x08, 0x05, 0x98, 0x7c, 0xb7, 0x0b, 0xe1, 0x7b};
  static constexpr uint8_t GCM_CIPHER[16] = {
      0xce, 0xa7, 0x40, 0x3d, 0x4d, 0x60, 0x6b, 0x6e,
      0x07, 0x4e, 0xc5, 0xd3, 0xba, 0xf3, 0x9d, 0x18};
  static constexpr uint8_t GCM_TAG[16] = {
      0xd0, 0xd1, 0xc8, 0xa7, 0x99, 0x99, 0x6b, 0xf0,
      0x26, 0x5b, 0x98, 0xb5, 0xd4, 0x8a, 0xb9, 0x19};
  uint8_t derived[KEY_SIZE] = {};
  uint8_t zeroKey[KEY_SIZE] = {};
  uint8_t nonce[NONCE_SIZE] = {};
  uint8_t plain[16] = {};
  uint8_t cipher[16] = {};
  uint8_t tag[TAG_SIZE] = {};
  uint8_t decrypted[16] = {};
  bool ok = deriveKey("password", reinterpret_cast<const uint8_t *>("salt"),
                      4, 1, derived) &&
            memcmp(derived, PBKDF2_EXPECTED, sizeof(derived)) == 0 &&
            aesGcmEncrypt(zeroKey, nonce, nullptr, 0, plain, sizeof(plain),
                          cipher, tag) &&
            memcmp(cipher, GCM_CIPHER, sizeof(cipher)) == 0 &&
            memcmp(tag, GCM_TAG, sizeof(tag)) == 0 &&
            aesGcmDecrypt(zeroKey, nonce, nullptr, 0, cipher, sizeof(cipher),
                          tag, decrypted) &&
            memcmp(decrypted, plain, sizeof(plain)) == 0;
  tag[0] ^= 1;
  ok = ok && !aesGcmDecrypt(zeroKey, nonce, nullptr, 0, cipher, sizeof(cipher),
                            tag, decrypted);
  secureZero(derived, sizeof(derived));
  secureZero(zeroKey, sizeof(zeroKey));
  secureZero(nonce, sizeof(nonce));
  secureZero(plain, sizeof(plain));
  secureZero(cipher, sizeof(cipher));
  secureZero(tag, sizeof(tag));
  secureZero(decrypted, sizeof(decrypted));
  return ok;
}
