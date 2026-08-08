#pragma once

#include <Arduino.h>

enum class WalletExportFormat : uint8_t {
  ElectrumPrivate,
  AuroraWallet
};

enum class WalletExportResult : uint8_t {
  Ok,
  InvalidName,
  InvalidData,
  UnsupportedFormat,
  WeakPassword,
  NoCard,
  AlreadyExists,
  OpenFailed,
  MemoryFailed,
  CryptoFailed,
  WriteFailed
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
};

constexpr size_t AURORA_WALLET_MIN_PASSWORD_LENGTH = 12;

const char *walletExportSuffix(WalletExportFormat format);

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
