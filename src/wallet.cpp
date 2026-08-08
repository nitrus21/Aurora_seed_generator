#include "wallet.h"
#include "hardware_rng.h"
#include "secure_memory.h"

#include <Hash.h>
#include <mbedtls/md.h>
#include <mbedtls/pkcs5.h>
#include <utility/trezor/bip39.h>

namespace {
constexpr char BECH32_CHARS[] = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";
constexpr char DESCRIPTOR_INPUT[] =
    "0123456789()[],'/*abcdefgh@:$%{}IJKLMNOPQRSTUVWXYZ&+-.;<=>?!^_|~"
    "ijklmnopqrstuvwxyzABCDEFGH`#\"\\ ";
constexpr char DESCRIPTOR_CHECKSUM[] = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";
constexpr uint8_t SECP256K1_ORDER[32] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,
    0xba, 0xae, 0xdc, 0xe6, 0xaf, 0x48, 0xa0, 0x3b,
    0xbf, 0xd2, 0x5e, 0x8c, 0xd0, 0x36, 0x41, 0x41};

bool isSupportedWordCount(uint8_t words) {
  return words == 12 || words == 15 || words == 18 || words == 21 || words == 24;
}

bool isAsciiPassphrase(const char *passphrase) {
  if (!passphrase) return true;
  const size_t length = strlen(passphrase);
  if (length > 63) return false;
  for (size_t i = 0; i < length; ++i) {
    const uint8_t c = static_cast<uint8_t>(passphrase[i]);
    if (c < 0x20 || c > 0x7e) return false;
  }
  return true;
}

bool scalarLessThanOrder(const uint8_t scalar[32]) {
  return memcmp(scalar, SECP256K1_ORDER, 32) < 0;
}

bool isStrictPrivateScalar(const uint8_t scalar[32]) {
  uint8_t nonZero = 0;
  for (size_t i = 0; i < 32; ++i) nonZero |= scalar[i];
  return nonZero != 0 && scalarLessThanOrder(scalar);
}

bool bytesToHex(const uint8_t *bytes, size_t length, char *out, size_t capacity) {
  static constexpr char HEX_CHARS[] = "0123456789abcdef";
  if (!bytes || !out || capacity < length * 2 + 1) return false;
  for (size_t i = 0; i < length; ++i) {
    out[i * 2] = HEX_CHARS[bytes[i] >> 4];
    out[i * 2 + 1] = HEX_CHARS[bytes[i] & 0x0f];
  }
  out[length * 2] = '\0';
  return true;
}

int compareWordSpan(const char *word, size_t length, const char *candidate) {
  const size_t candidateLength = strlen(candidate);
  const size_t commonLength = length < candidateLength ? length : candidateLength;
  const int comparison = memcmp(word, candidate, commonLength);
  if (comparison != 0) return comparison;
  if (length < candidateLength) return -1;
  if (length > candidateLength) return 1;
  return 0;
}

bool findBip39Word(const char *word, size_t length, uint16_t &index) {
  const char *const *wordlist = mnemonic_wordlist();
  if (!wordlist || !word || length == 0) return false;
  int low = 0;
  int high = 2047;
  while (low <= high) {
    const int middle = low + (high - low) / 2;
    const int comparison = compareWordSpan(word, length, wordlist[middle]);
    if (comparison == 0) {
      index = static_cast<uint16_t>(middle);
      return true;
    }
    if (comparison < 0) high = middle - 1;
    else low = middle + 1;
  }
  return false;
}

bool decodeMnemonicBits(const char *mnemonic, uint8_t words, uint8_t decoded[33]) {
  if (!mnemonic || !decoded || !isSupportedWordCount(words)) return false;
  memset(decoded, 0, 33);
  const char *cursor = mnemonic;
  size_t bitPosition = 0;
  for (uint8_t wordNumber = 0; wordNumber < words; ++wordNumber) {
    const char *end = strchr(cursor, ' ');
    if (!end) end = cursor + strlen(cursor);
    uint16_t index = 0;
    if (!findBip39Word(cursor, static_cast<size_t>(end - cursor), index)) return false;
    for (uint8_t bit = 0; bit < 11; ++bit, ++bitPosition) {
      if (index & (1u << (10 - bit))) decoded[bitPosition / 8] |= 1u << (7 - bitPosition % 8);
    }
    if (wordNumber + 1 < words) {
      if (*end != ' ' || end[1] == '\0' || end[1] == ' ') return false;
      cursor = end + 1;
    } else if (*end != '\0') {
      return false;
    }
  }
  return true;
}

bool validateGeneratedMnemonic(const char *mnemonic, const uint8_t *entropy,
                               size_t entropyLength, uint8_t words) {
  uint8_t decoded[33] = {};
  uint8_t hash[32] = {};
  bool valid = entropy && entropyLength == static_cast<size_t>(words * 4 / 3) &&
               decodeMnemonicBits(mnemonic, words, decoded) &&
               memcmp(decoded, entropy, entropyLength) == 0;
  if (valid) {
    sha256(entropy, entropyLength, hash);
    const uint8_t checksumBits = entropyLength / 4;
    const uint8_t mask = static_cast<uint8_t>(0xffu << (8 - checksumBits));
    valid = (decoded[entropyLength] & mask) == (hash[0] & mask);
  }
  secureZero(decoded, sizeof(decoded));
  secureZero(hash, sizeof(hash));
  return valid;
}

bool validateMnemonicChecksum(const char *mnemonic, uint8_t words) {
  uint8_t decoded[33] = {};
  uint8_t hash[32] = {};
  const size_t entropyLength = static_cast<size_t>(words) * 4 / 3;
  bool valid = decodeMnemonicBits(mnemonic, words, decoded);
  if (valid) {
    sha256(decoded, entropyLength, hash);
    const uint8_t checksumBits = static_cast<uint8_t>(entropyLength / 4);
    const uint8_t mask = static_cast<uint8_t>(0xffu << (8 - checksumBits));
    valid = (decoded[entropyLength] & mask) == (hash[0] & mask);
  }
  secureZero(decoded, sizeof(decoded));
  secureZero(hash, sizeof(hash));
  return valid;
}

bool seedFromMnemonic(const char *mnemonic, const char *passphrase, uint8_t seed[64]) {
  char salt[72] = "mnemonic";
  bool ok = false;
  mbedtls_md_context_t context;
  mbedtls_md_init(&context);
  const char *password = passphrase ? passphrase : "";

  if (!mnemonic || !seed) goto cleanup;
  if (!isAsciiPassphrase(password)) goto cleanup;
  if (strlcat(salt, password, sizeof(salt)) >= sizeof(salt)) goto cleanup;

  {
    const mbedtls_md_info_t *sha512 = mbedtls_md_info_from_type(MBEDTLS_MD_SHA512);
    if (!sha512 || mbedtls_md_setup(&context, sha512, 1) != 0) goto cleanup;
    if (mbedtls_pkcs5_pbkdf2_hmac(
            &context, reinterpret_cast<const unsigned char *>(mnemonic), strlen(mnemonic),
            reinterpret_cast<const unsigned char *>(salt), strlen(salt), 2048,
            64, seed) != 0) goto cleanup;
  }
  ok = true;

cleanup:
  mbedtls_md_free(&context);
  secureZero(salt, sizeof(salt));
  if (!ok) secureZero(seed, 64);
  return ok;
}

bool masterFromMnemonic(const char *mnemonic, const char *passphrase, HDPrivateKey &master) {
  uint8_t seed[64] = {};
  uint8_t masterMaterial[64] = {};
  bool ok = false;
  const mbedtls_md_info_t *sha512 = mbedtls_md_info_from_type(MBEDTLS_MD_SHA512);

  if (!sha512 || !seedFromMnemonic(mnemonic, passphrase, seed)) goto cleanup;
  if (mbedtls_md_hmac(sha512,
                      reinterpret_cast<const unsigned char *>("Bitcoin seed"), 12,
                      seed, sizeof(seed), masterMaterial) != 0) goto cleanup;
  if (!isStrictPrivateScalar(masterMaterial)) goto cleanup;
  master = HDPrivateKey(masterMaterial, masterMaterial + 32, 0, nullptr, 0,
                        &Mainnet, UNKNOWN_TYPE);
  ok = static_cast<bool>(master);

cleanup:
  secureZero(seed, sizeof(seed));
  secureZero(masterMaterial, sizeof(masterMaterial));
  return ok;
}

uint64_t descriptorPolyMod(uint64_t checksum, uint8_t value) {
  const uint8_t top = checksum >> 35;
  checksum = ((checksum & 0x7ffffffffULL) << 5) ^ value;
  if (top & 1) checksum ^= 0xf5dee51989ULL;
  if (top & 2) checksum ^= 0xa9fdca3312ULL;
  if (top & 4) checksum ^= 0x1bab10e32dULL;
  if (top & 8) checksum ^= 0x3706b1677aULL;
  if (top & 16) checksum ^= 0x644d626ffdULL;
  return checksum;
}

bool descriptorWithChecksum(const char *body, char *out, size_t capacity) {
  uint64_t checksum = 1;
  uint64_t classes = 0;
  uint8_t classCount = 0;
  for (const char *p = body; *p; ++p) {
    const char *position = strchr(DESCRIPTOR_INPUT, *p);
    if (!position) return false;
    const uint8_t value = static_cast<uint8_t>(position - DESCRIPTOR_INPUT);
    checksum = descriptorPolyMod(checksum, value & 31);
    classes = classes * 3 + (value >> 5);
    if (++classCount == 3) {
      checksum = descriptorPolyMod(checksum, classes);
      classes = 0;
      classCount = 0;
    }
  }
  if (classCount) checksum = descriptorPolyMod(checksum, classes);
  for (uint8_t i = 0; i < 8; ++i) checksum = descriptorPolyMod(checksum, 0);
  checksum ^= 1;

  const size_t bodyLength = strlen(body);
  if (bodyLength + 10 > capacity) return false;
  memcpy(out, body, bodyLength);
  out[bodyLength] = '#';
  for (uint8_t i = 0; i < 8; ++i) {
    out[bodyLength + 1 + i] = DESCRIPTOR_CHECKSUM[(checksum >> (5 * (7 - i))) & 31];
  }
  out[bodyLength + 9] = '\0';
  return true;
}

bool wrapDescriptor(AddressKind kind, const char *keyExpression, char *out, size_t capacity) {
  char body[216] = {};
  int length = 0;
  switch (kind) {
    case AddressKind::Legacy:
      length = snprintf(body, sizeof(body), "pkh(%s)", keyExpression); break;
    case AddressKind::NestedSegwit:
      length = snprintf(body, sizeof(body), "sh(wpkh(%s))", keyExpression); break;
    case AddressKind::NativeSegwit:
      length = snprintf(body, sizeof(body), "wpkh(%s)", keyExpression); break;
    case AddressKind::Taproot:
      length = snprintf(body, sizeof(body), "tr(%s)", keyExpression); break;
  }
  const bool ok = length > 0 && static_cast<size_t>(length) < sizeof(body) &&
                  descriptorWithChecksum(body, out, capacity);
  secureZero(body, sizeof(body));
  return ok;
}

uint32_t polymod(const uint8_t *values, size_t length) {
  uint32_t checksum = 1;
  constexpr uint32_t generators[5] = {
      0x3b6a57b2, 0x26508e6d, 0x1ea119fa, 0x3d4233dd, 0x2a1462b3};
  for (size_t i = 0; i < length; ++i) {
    const uint8_t top = checksum >> 25;
    checksum = ((checksum & 0x1ffffff) << 5) ^ values[i];
    for (uint8_t j = 0; j < 5; ++j) if ((top >> j) & 1) checksum ^= generators[j];
  }
  return checksum;
}

bool encodeSegwitV1(const uint8_t program[32], char *out, size_t capacity) {
  uint8_t values[90] = {};
  size_t count = 0;
  for (const char *p = "bc"; *p; ++p) values[count++] = *p >> 5;
  values[count++] = 0;
  for (const char *p = "bc"; *p; ++p) values[count++] = *p & 31;
  const size_t dataStart = count;
  values[count++] = 1;
  uint32_t accumulator = 0;
  int bits = 0;
  for (size_t i = 0; i < 32; ++i) {
    accumulator = (accumulator << 8) | program[i];
    bits += 8;
    while (bits >= 5) {
      bits -= 5;
      values[count++] = (accumulator >> bits) & 31;
    }
  }
  if (bits) values[count++] = (accumulator << (5 - bits)) & 31;
  const size_t dataLength = count - dataStart;
  for (uint8_t i = 0; i < 6; ++i) values[count++] = 0;
  const uint32_t checksum = polymod(values, count) ^ 0x2bc830a3;
  if (3 + dataLength + 6 + 1 > capacity) {
    secureZero(values, sizeof(values));
    return false;
  }
  strcpy(out, "bc1");
  size_t position = 3;
  for (size_t i = 0; i < dataLength; ++i) out[position++] = BECH32_CHARS[values[dataStart + i]];
  for (uint8_t i = 0; i < 6; ++i) out[position++] = BECH32_CHARS[(checksum >> (5 * (5 - i))) & 31];
  out[position] = '\0';
  secureZero(values, sizeof(values));
  return true;
}

const char *purposePath(AddressKind kind) {
  switch (kind) {
    case AddressKind::Legacy: return "m/44'/0'/0'/0/0";
    case AddressKind::NestedSegwit: return "m/49'/0'/0'/0/0";
    case AddressKind::NativeSegwit: return "m/84'/0'/0'/0/0";
    case AddressKind::Taproot: return "m/86'/0'/0'/0/0";
  }
  return "";
}

const char *accountPath(AddressKind kind) {
  switch (kind) {
    case AddressKind::Legacy: return "m/44'/0'/0'";
    case AddressKind::NestedSegwit: return "m/49'/0'/0'";
    case AddressKind::NativeSegwit: return "m/84'/0'/0'";
    case AddressKind::Taproot: return "m/86'/0'/0'";
  }
  return "";
}

const char *accountOriginSuffix(AddressKind kind) {
  switch (kind) {
    case AddressKind::Legacy: return "44h/0h/0h";
    case AddressKind::NestedSegwit: return "49h/0h/0h";
    case AddressKind::NativeSegwit: return "84h/0h/0h";
    case AddressKind::Taproot: return "86h/0h/0h";
  }
  return "";
}

ScriptType extendedKeyType(AddressKind kind) {
  if (kind == AddressKind::NestedSegwit) return P2SH_P2WPKH;
  if (kind == AddressKind::NativeSegwit) return P2WPKH;
  return UNKNOWN_TYPE;
}

bool populateWallet(const HDPrivateKey &master, AddressKind kind, WalletOutput &out);
}

WalletEngine::WalletEngine() {}

bool WalletEngine::bip39Word(const char *word) {
  if (!word || !word[0]) return false;
  uint16_t index = 0;
  return findBip39Word(word, strlen(word), index);
}

uint8_t WalletEngine::bip39Suggestions(const char *prefix, char *suggestions,
                                       size_t stride, uint8_t maximum) {
  if (!prefix || !prefix[0] || !suggestions ||
      stride < BIP39_WORD_CAPACITY || maximum == 0) {
    return 0;
  }
  const size_t prefixLength = strlen(prefix);
  if (prefixLength >= BIP39_WORD_CAPACITY) return 0;
  for (size_t i = 0; i < prefixLength; ++i) {
    if (prefix[i] < 'a' || prefix[i] > 'z') return 0;
  }

  const char *const *wordlist = mnemonic_wordlist();
  if (!wordlist) return 0;
  uint8_t count = 0;
  for (uint16_t i = 0; i < 2048 && count < maximum; ++i) {
    if (strncmp(wordlist[i], prefix, prefixLength) == 0) {
      strlcpy(suggestions + static_cast<size_t>(count) * stride,
              wordlist[i], stride);
      ++count;
    }
  }
  return count;
}

bool WalletEngine::create(uint8_t words, AddressKind kind, const char *passphrase,
                          const uint8_t touchEntropy[32], WalletOutput &out) {
  wipe(out);
  if (!isSupportedWordCount(words) || !touchEntropy || !isAsciiPassphrase(passphrase)) return false;

  const size_t entropyLength = (words * 11 - words / 3) / 8;
  uint8_t pool[64] = {};
  uint8_t entropy[32] = {};
  bool ok = false;

  hardwareRandomFill(pool, 32);
  memcpy(pool + 32, touchEntropy, 32);
  sha256(pool, sizeof(pool), entropy);

  {
    const char *generated = mnemonicFromEntropy(entropy, entropyLength);
    if (!generated || !validateGeneratedMnemonic(generated, entropy, entropyLength, words)) {
      mnemonic_clear();
      goto cleanup;
    }
    if (strlcpy(out.mnemonic, generated, sizeof(out.mnemonic)) >= sizeof(out.mnemonic)) {
      mnemonic_clear();
      goto cleanup;
    }
    mnemonic_clear();
  }

  {
    HDPrivateKey master;
    if (!masterFromMnemonic(out.mnemonic, passphrase, master)) goto cleanup;
    if (!populateWallet(master, kind, out)) goto cleanup;
  }

  out.kind = kind;
  out.valid = true;
  ok = true;

cleanup:
  mnemonic_clear();
  secureZero(pool, sizeof(pool));
  secureZero(entropy, sizeof(entropy));
  if (!ok) wipe(out);
  return ok;
}

bool WalletEngine::restore(const char *mnemonic, uint8_t words, AddressKind kind,
                           const char *passphrase, WalletOutput &out) {
  wipe(out);
  if (!mnemonic || !isSupportedWordCount(words) ||
      !isAsciiPassphrase(passphrase) || !validateMnemonicChecksum(mnemonic, words)) {
    return false;
  }
  if (strlcpy(out.mnemonic, mnemonic, sizeof(out.mnemonic)) >= sizeof(out.mnemonic)) {
    wipe(out);
    return false;
  }

  HDPrivateKey master;
  if (!masterFromMnemonic(out.mnemonic, passphrase, master) ||
      !populateWallet(master, kind, out)) {
    wipe(out);
    return false;
  }
  out.kind = kind;
  out.valid = true;
  return true;
}

bool WalletEngine::accountXprv(const WalletOutput &wallet,
                               const char *passphrase,
                               char *out, size_t outLen) {
  if (out && outLen) out[0] = '\0';
  if (!wallet.valid || !passphrase || !out || outLen < 112) {
    return false;
  }

  HDPrivateKey master;
  if (!masterFromMnemonic(wallet.mnemonic, passphrase, master)) return false;
  HDPrivateKey account = master.derive(accountPath(wallet.kind));
  if (!account) return false;
  account.type = extendedKeyType(wallet.kind);
  if (account.xprv(out, outLen) == 0) {
    secureZero(out, outLen);
    return false;
  }
  return true;
}

bool WalletEngine::taprootAddress(const PublicKey &internalKey, char *out, size_t outLength) {
  uint8_t x[32] = {};
  uint8_t tweak[32] = {};
  uint8_t outputX[32] = {};
  bool ok = false;
  if (internalKey.x(x, sizeof(x)) != sizeof(x)) goto cleanup;
  tagged_hash("TapTweak", x, sizeof(x), tweak);
  if (!scalarLessThanOrder(tweak)) goto cleanup;
  {
    ECPoint evenPoint;
    // uBitcoin parses a compressed SEC point and therefore reports 33 bytes,
    // even though this convenience method accepts a 32-byte x-only key.
    if (evenPoint.from_x(x, sizeof(x)) == 0 || !evenPoint) goto cleanup;
    uint8_t tweakNonZero = 0;
    for (uint8_t byte : tweak) tweakNonZero |= byte;
    ECPoint outputKey = evenPoint;
    if (tweakNonZero) {
      ECScalar scalar(tweak, sizeof(tweak));
      if (!scalar) goto cleanup;
      ECPoint tweakPoint = scalar * GeneratorPoint;
      if (!tweakPoint) goto cleanup;
      outputKey = evenPoint + tweakPoint;
    }
    if (!outputKey) goto cleanup;
    if (outputKey.x(outputX, sizeof(outputX)) != sizeof(outputX)) goto cleanup;
    ok = encodeSegwitV1(outputX, out, outLength);
  }

cleanup:
  secureZero(x, sizeof(x));
  secureZero(tweak, sizeof(tweak));
  secureZero(outputX, sizeof(outputX));
  return ok;
}

namespace {
bool populateWallet(const HDPrivateKey &master, AddressKind kind, WalletOutput &out) {
  uint8_t publicSec[33] = {};
  uint8_t fingerprint[4] = {};
  char publicKeyExpression[196] = {};
  bool ok = false;

  HDPrivateKey child = master.derive(purposePath(kind));
  HDPrivateKey account = master.derive(accountPath(kind));
  if (!child || !account) goto cleanup;

  {
    PublicKey publicKey = child.publicKey();
    if (!publicKey || publicKey.sec(publicSec, sizeof(publicSec)) != sizeof(publicSec)) goto cleanup;
    if (!bytesToHex(publicSec, sizeof(publicSec), out.publicKey, sizeof(out.publicKey))) goto cleanup;
  }
  if (child.wif(out.privateWif, sizeof(out.privateWif)) == 0) goto cleanup;

  account.type = UNKNOWN_TYPE;
  {
    HDPublicKey standard = account.xpub();
    if (!standard || standard.xpub(out.accountStandardXpub, sizeof(out.accountStandardXpub)) == 0) goto cleanup;
  }
  account.type = extendedKeyType(kind);
  {
    HDPublicKey selected = account.xpub();
    if (!selected || selected.xpub(out.accountXpub, sizeof(out.accountXpub)) == 0) goto cleanup;
  }

  master.fingerprint(fingerprint);
  {
    char fingerprintHex[9] = {};
    if (!bytesToHex(fingerprint, sizeof(fingerprint), fingerprintHex, sizeof(fingerprintHex))) goto cleanup;
    const int originLength = snprintf(out.keyOrigin, sizeof(out.keyOrigin), "[%s/%s]",
                                      fingerprintHex, accountOriginSuffix(kind));
    secureZero(fingerprintHex, sizeof(fingerprintHex));
    if (originLength <= 0 || static_cast<size_t>(originLength) >= sizeof(out.keyOrigin)) goto cleanup;
  }

  {
    const int expressionLength = snprintf(publicKeyExpression, sizeof(publicKeyExpression),
                                          "%s%s", out.keyOrigin,
                                          out.accountStandardXpub);
    if (expressionLength <= 0 ||
        static_cast<size_t>(expressionLength) >= sizeof(publicKeyExpression)) goto cleanup;
  }
  if (!wrapDescriptor(kind, publicKeyExpression, out.accountDescriptor,
                      sizeof(out.accountDescriptor))) goto cleanup;

  {
    const int expressionLength = snprintf(publicKeyExpression, sizeof(publicKeyExpression),
                                          "%s%s/0/*", out.keyOrigin,
                                          out.accountStandardXpub);
    if (expressionLength <= 0 ||
        static_cast<size_t>(expressionLength) >= sizeof(publicKeyExpression)) goto cleanup;
  }
  if (!wrapDescriptor(kind, publicKeyExpression, out.watchDescriptor,
                      sizeof(out.watchDescriptor))) goto cleanup;

  {
    const int expressionLength = snprintf(publicKeyExpression, sizeof(publicKeyExpression),
                                          "%s%s/1/*", out.keyOrigin,
                                          out.accountStandardXpub);
    if (expressionLength <= 0 ||
        static_cast<size_t>(expressionLength) >= sizeof(publicKeyExpression)) goto cleanup;
  }
  if (!wrapDescriptor(kind, publicKeyExpression, out.changeDescriptor,
                      sizeof(out.changeDescriptor))) goto cleanup;

  {
    const int expressionLength = snprintf(publicKeyExpression, sizeof(publicKeyExpression),
                                          "%s%s/<0;1>/*", out.keyOrigin,
                                          out.accountStandardXpub);
    if (expressionLength <= 0 ||
        static_cast<size_t>(expressionLength) >= sizeof(publicKeyExpression)) goto cleanup;
  }
  if (!wrapDescriptor(kind, publicKeyExpression, out.multipathDescriptor,
                      sizeof(out.multipathDescriptor))) goto cleanup;
  if (!wrapDescriptor(kind, out.privateWif, out.privateDescriptor,
                      sizeof(out.privateDescriptor))) goto cleanup;

  if (strlcpy(out.path, purposePath(kind), sizeof(out.path)) >= sizeof(out.path)) goto cleanup;
  if (kind == AddressKind::Legacy) {
    if (child.legacyAddress(out.address, sizeof(out.address)) == 0) goto cleanup;
  } else if (kind == AddressKind::NestedSegwit) {
    if (child.nestedSegwitAddress(out.address, sizeof(out.address)) == 0) goto cleanup;
  } else if (kind == AddressKind::NativeSegwit) {
    if (child.segwitAddress(out.address, sizeof(out.address)) == 0) goto cleanup;
  } else if (!WalletEngine::taprootAddress(child.publicKey(), out.address, sizeof(out.address))) {
    goto cleanup;
  }
  ok = true;

cleanup:
  secureZero(publicSec, sizeof(publicSec));
  secureZero(fingerprint, sizeof(fingerprint));
  secureZero(publicKeyExpression, sizeof(publicKeyExpression));
  return ok;
}
}

WalletSelfTest WalletEngine::selfTest() {
  static constexpr char MNEMONIC[] =
      "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about";
  static constexpr uint8_t BIP39_SEED[64] = {
      0xc5, 0x52, 0x57, 0xc3, 0x60, 0xc0, 0x7c, 0x72, 0x02, 0x9a, 0xeb, 0xc1, 0xb5, 0x3c, 0x05, 0xed,
      0x03, 0x62, 0xad, 0xa3, 0x8e, 0xad, 0x3e, 0x3e, 0x9e, 0xfa, 0x37, 0x08, 0xe5, 0x34, 0x95, 0x53,
      0x1f, 0x09, 0xa6, 0x98, 0x75, 0x99, 0xd1, 0x82, 0x64, 0xc1, 0xe1, 0xc9, 0x2f, 0x2c, 0xf1, 0x41,
      0x63, 0x0c, 0x7a, 0x3c, 0x4a, 0xb7, 0xc8, 0x1b, 0x2f, 0x00, 0x16, 0x98, 0xe7, 0x46, 0x3b, 0x04};
  struct Vector {
    AddressKind kind;
    const char *address;
    const char *extendedPrefix;
    const char *accountExtendedKey;
    WalletSelfTest buildError;
    WalletSelfTest addressError;
    WalletSelfTest extendedError;
    WalletSelfTest standardError;
    WalletSelfTest descriptorError;
  };
  static constexpr Vector VECTORS[] = {
      {AddressKind::Legacy, "1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA", "xpub", nullptr,
       WalletSelfTest::LegacyBuild, WalletSelfTest::LegacyAddress,
       WalletSelfTest::LegacyExtendedKey, WalletSelfTest::LegacyStandardKey,
       WalletSelfTest::LegacyDescriptor},
      {AddressKind::NestedSegwit, "37VucYSaXLCAsxYyAPfbSi9eh4iEcbShgf", "ypub", nullptr,
       WalletSelfTest::NestedBuild, WalletSelfTest::NestedAddress,
       WalletSelfTest::NestedExtendedKey, WalletSelfTest::NestedStandardKey,
       WalletSelfTest::NestedDescriptor},
      {AddressKind::NativeSegwit, "bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu",
       "zpub",
       "zpub6rFR7y4Q2AijBEqTUquhVz398htDFrtymD9xYYfG1m4wAcvPhXNfE3EfH1r1ADqtfSdVCToUG868RvUUkgDKf31mGDtKsAYz2oz2AGutZYs",
       WalletSelfTest::NativeBuild, WalletSelfTest::NativeAddress,
       WalletSelfTest::NativeExtendedKey, WalletSelfTest::NativeStandardKey,
       WalletSelfTest::NativeDescriptor},
      {AddressKind::Taproot, "bc1p5cyxnuxmeuwuvkwfem96lqzszd02n6xdcjrs20cac6yqjjwudpxqkedrcr",
       "xpub",
       "xpub6BgBgsespWvERF3LHQu6CnqdvfEvtMcQjYrcRzx53QJjSxarj2afYWcLteoGVky7D3UKDP9QyrLprQ3VCECoY49yfdDEHGCtMMj92pReUsQ",
       WalletSelfTest::TaprootBuild, WalletSelfTest::TaprootAddress,
       WalletSelfTest::TaprootExtendedKey, WalletSelfTest::TaprootStandardKey,
       WalletSelfTest::TaprootDescriptor}};

  HDPrivateKey master;
  WalletOutput output{};
  uint8_t zeroEntropy[32] = {};
  uint8_t testSeed[64] = {};
  char descriptor[64] = {};
  char accountXprv[128] = {};
  char suggestions[3][BIP39_WORD_CAPACITY] = {};
  WalletSelfTest result = WalletSelfTest::Ok;
  if (!masterFromMnemonic(MNEMONIC, "", master)) {
    result = WalletSelfTest::MasterKey;
    goto cleanup;
  }

  if (!seedFromMnemonic(MNEMONIC, "TREZOR", testSeed) ||
      memcmp(testSeed, BIP39_SEED, sizeof(testSeed)) != 0) {
    result = WalletSelfTest::Bip39Seed;
    goto cleanup;
  }
  if (!bip39Word("abandon") || bip39Word("aband") ||
      bip39Suggestions("aban", reinterpret_cast<char *>(suggestions),
                       BIP39_WORD_CAPACITY, 3) != 1 ||
      strcmp(suggestions[0], "abandon") != 0) {
    result = WalletSelfTest::Bip39Autocomplete;
    goto cleanup;
  }
  if (!descriptorWithChecksum("raw(deadbeef)", descriptor, sizeof(descriptor)) ||
      strcmp(descriptor, "raw(deadbeef)#89f8spxm") != 0) {
    result = WalletSelfTest::DescriptorChecksum;
    goto cleanup;
  }

  for (const Vector &vector : VECTORS) {
    wipe(output);
    if (!populateWallet(master, vector.kind, output)) {
      result = vector.buildError; goto cleanup;
    }
    if (strcmp(output.address, vector.address) != 0) {
      result = vector.addressError; goto cleanup;
    }
    {
      PrivateKey importedWif;
      char importedAddress[sizeof(output.address)] = {};
      bool wifMatches = importedWif.fromWIF(output.privateWif) != 0;
      if (wifMatches) {
        if (vector.kind == AddressKind::Legacy) {
          wifMatches = importedWif.legacyAddress(importedAddress, sizeof(importedAddress)) != 0;
        } else if (vector.kind == AddressKind::NestedSegwit) {
          wifMatches = importedWif.nestedSegwitAddress(importedAddress, sizeof(importedAddress)) != 0;
        } else if (vector.kind == AddressKind::NativeSegwit) {
          wifMatches = importedWif.segwitAddress(importedAddress, sizeof(importedAddress)) != 0;
        } else {
          wifMatches = taprootAddress(importedWif.publicKey(), importedAddress,
                                      sizeof(importedAddress));
        }
      }
      wifMatches = wifMatches && strcmp(importedAddress, output.address) == 0;
      secureZero(importedAddress, sizeof(importedAddress));
      if (!wifMatches) {
        result = WalletSelfTest::PrivateWif; goto cleanup;
      }
    }
    if (strncmp(output.accountXpub, vector.extendedPrefix, 4) != 0 ||
        (vector.accountExtendedKey && strcmp(output.accountXpub, vector.accountExtendedKey) != 0)) {
      result = vector.extendedError; goto cleanup;
    }
    {
      HDPrivateKey account = master.derive(accountPath(vector.kind));
      account.type = extendedKeyType(vector.kind);
      if (!account || account.xprv(accountXprv, sizeof(accountXprv)) == 0 ||
          accountXprv[0] != vector.extendedPrefix[0] ||
          strncmp(accountXprv + 1, "prv", 3) != 0) {
        result = vector.extendedError; goto cleanup;
      }
      secureZero(accountXprv, sizeof(accountXprv));
    }
    if (strncmp(output.accountStandardXpub, "xpub", 4) != 0) {
      result = vector.standardError; goto cleanup;
    }
    if (!strstr(output.watchDescriptor, output.accountStandardXpub)) {
      result = vector.descriptorError; goto cleanup;
    }
    if (!strstr(output.accountDescriptor, output.accountStandardXpub) ||
        !strstr(output.changeDescriptor, "/1/*") ||
        !strstr(output.multipathDescriptor, "/<0;1>/*")) {
      result = vector.descriptorError; goto cleanup;
    }
  }

  for (size_t length = 16, testNumber = 0; length <= 32; length += 4, ++testNumber) {
    const uint8_t words = static_cast<uint8_t>((length * 8 + length / 4) / 11);
    const char *mnemonic = mnemonicFromEntropy(zeroEntropy, length);
    if (!mnemonic || !validateGeneratedMnemonic(mnemonic, zeroEntropy, length, words) ||
        (length == 16 && strcmp(mnemonic, MNEMONIC) != 0)) {
      result = static_cast<WalletSelfTest>(
          static_cast<uint8_t>(WalletSelfTest::Mnemonic12) + testNumber);
      mnemonic_clear();
      goto cleanup;
    }
    mnemonic_clear();
  }

cleanup:
  mnemonic_clear();
  wipe(output);
  secureZero(zeroEntropy, sizeof(zeroEntropy));
  secureZero(testSeed, sizeof(testSeed));
  secureZero(descriptor, sizeof(descriptor));
  secureZero(accountXprv, sizeof(accountXprv));
  secureZero(suggestions, sizeof(suggestions));
  return result;
}

void WalletEngine::wipe(WalletOutput &out) {
  secureZero(&out, sizeof(out));
}
