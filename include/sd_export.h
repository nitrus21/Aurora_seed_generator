#pragma once

#include "platform/runtime.h"
#include "pin_security.h"

enum class WalletExportFormat : uint8_t {
  ElectrumPrivate,
  SparrowPrivate,
  AuroraWallet
};

// Umbrel/LND recovery files deliberately reuse the authenticated Aurora V1
// container but carry only the BIP32 root pair. AEZEED words and passphrases
// are never serialized. Older firmware rejects this reserved subtype safely.
constexpr uint8_t AURORA_WALLET_KIND_UMBREL = 0xff;
constexpr uint8_t AURORA_WALLET_WORDS_UMBREL = 0;
constexpr char AURORA_WALLET_TYPE_UMBREL[] = "umbrel-lnd-bip32";
constexpr char AURORA_WALLET_PATH_UMBREL[] = "m";

enum class WalletExportResult : uint8_t {
  Ok,
  InvalidName,
  InvalidData,
  UnsupportedFormat,
  WeakPassword,
  InvalidPin,
  NoCard,
  AlreadyExists,
  OpenFailed,
  MemoryFailed,
  CryptoFailed,
  WriteFailed
#if defined(AURORA_BOARD_P4)
  , FinalizeFailed
#endif
};

enum class AuroraWalletReadResult : uint8_t {
  Ok,
  InvalidName,
  WeakPassword,
  NoCard,
  NotFound,
  OpenFailed,
  InvalidFormat,
  MemoryFailed,
  AuthenticationFailed,
  ReadFailed
};

enum class AuroraWalletListResult : uint8_t {
  Ok,
  NoCard,
  OpenFailed,
  NoFiles,
  BufferTooSmall
};

struct WalletExportData {
  uint8_t addressKind;
  uint8_t wordCount;
  const char *addressType;
  const char *derivationPath;
  const char *mnemonic;
  const char *passphrase;
  const char *address;
  const char *accountXpub;
  const char *accountXprv;
  const char *privateWif;
  const char *receiveDescriptor;
  const AuroraPinRecord *pin; // Keep this an aggregate under the CYD's C++11.
};

struct AuroraWalletData {
  uint8_t addressKind;
  uint8_t wordCount;
  char addressType[40];
  char derivationPath[32];
  char mnemonic[256];
  char passphrase[64];
  char address[96];
  char accountXpub[128];
  char accountXprv[128];
  char privateWif[64];
  char receiveDescriptor[224];
  uint8_t fileVersion;
  AuroraPinRecord pin;
};

// Only a public fingerprint leaves the codec. The password-derived key is
// always erased inside the read/write call; no session-key API is exposed.
AuroraWalletReadResult readAuroraWalletFileChecked(const char *baseName,
    const char *filePassword, AuroraWalletData &data, uint8_t fingerprint[32],
    const uint8_t *expectedFingerprint = nullptr);

// Authenticates a closed-file read-back. Failure preserves the encrypted backup.
WalletExportResult writeAuroraWalletFileVerified(const char *baseName,
    const char *filePassword, const WalletExportData &data, char *writtenPath,
    size_t writtenPathLength, uint8_t fingerprint[32]);

constexpr size_t AURORA_WALLET_MIN_PASSWORD_LENGTH = 12;

const char *walletExportSuffix(WalletExportFormat format);

// Fresh, read-only mount/root check. An empty readable card is accepted;
// never format a card or create a probe file. Writes still check their result.
bool auroraSdReady();

WalletExportResult writeWalletExportFile(WalletExportFormat format,
                                         const char *baseName,
                                         const char *filePassword,
                                         const WalletExportData &data,
                                         char *writtenPath,
                                         size_t writtenPathLength);

AuroraWalletReadResult readAuroraWalletFile(const char *baseName,
                                            const char *filePassword,
                                            AuroraWalletData &data);

AuroraWalletListResult listAuroraWalletFiles(char *options,
                                             size_t optionsLength,
                                             uint16_t &fileCount);

void wipeAuroraWalletData(AuroraWalletData &data);
bool auroraWalletCryptoSelfTest();
