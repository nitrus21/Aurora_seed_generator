#pragma once
#include "platform/runtime.h"
#include <Bitcoin.h>

enum class AddressKind : uint8_t { Legacy, NestedSegwit, NativeSegwit, Taproot };

enum class WalletSelfTest : uint8_t {
  Ok = 0,
  MasterKey = 1,
  Bip39Seed = 2,
  DescriptorChecksum = 3,
  Bip39Autocomplete = 4,
  PrivateWif = 5,
  LegacyBuild = 10,
  LegacyAddress = 11,
  LegacyExtendedKey = 12,
  LegacyStandardKey = 13,
  LegacyDescriptor = 14,
  NestedBuild = 20,
  NestedAddress = 21,
  NestedExtendedKey = 22,
  NestedStandardKey = 23,
  NestedDescriptor = 24,
  NativeBuild = 30,
  NativeAddress = 31,
  NativeExtendedKey = 32,
  NativeStandardKey = 33,
  NativeDescriptor = 34,
  TaprootBuild = 40,
  TaprootAddress = 41,
  TaprootExtendedKey = 42,
  TaprootStandardKey = 43,
  TaprootDescriptor = 44,
  Mnemonic12 = 51,
  Mnemonic15 = 52,
  Mnemonic18 = 53,
  Mnemonic21 = 54,
  Mnemonic24 = 55,
  AuroraWalletCrypto = 60,
  AezeedRootKey = 61,
  AezeedPublicDerivation = 62
};

struct WalletOutput {
  char mnemonic[256];
  char address[96];
  char publicKey[80];
  char privateWif[64];
  char privateDescriptor[128];
  char accountXpub[128];
  char accountStandardXpub[128];
  char keyOrigin[48];
  char accountDescriptor[224];
  char watchDescriptor[224];
  char changeDescriptor[224];
  char multipathDescriptor[224];
  char path[32];
  AddressKind kind;
  bool valid;
};

class WalletEngine {
 public:
  static constexpr size_t BIP39_WORD_CAPACITY = 9;
  WalletEngine();
  bool create(uint8_t words, AddressKind kind, const char *passphrase,
              const uint8_t touchEntropy[32], WalletOutput &out);
  bool restore(const char *mnemonic, uint8_t words, AddressKind kind,
               const char *passphrase, WalletOutput &out);
  bool accountXprv(const WalletOutput &wallet, const char *passphrase,
                   char *out, size_t outLen);
  bool rootXprvFromSeed(const uint8_t *seed, size_t seedLength,
                        char *out, size_t outLen);
  bool rootXpubFromXprv(const char *xprv, char *out, size_t outLen);
  // Derive only public display data from a transient BIP32 root. The root and
  // every derived private object stay local to this call and are destroyed
  // before it returns; no WIF, descriptor or credential is retained in out.
  bool publicWalletFromRootXprv(const char *xprv, AddressKind kind,
                                WalletOutput &out);
  // Build public-only account data from a transient BIP39 phrase. No mnemonic,
  // WIF, descriptor or passphrase is copied into the returned structure.
  bool publicWalletFromMnemonic(const char *mnemonic, uint8_t words,
                                const char *passphrase, AddressKind kind,
                                WalletOutput &out);
  // Derive a receive child (branch 0) from an account-level public key only.
  // This deliberately accepts no private key and is limited to the first 20
  // receive indices exposed by the P4 interface.
  bool publicChildFromAccountXpub(const char *accountXpub, AddressKind kind,
                                  uint8_t index, char *address, size_t addressLen,
                                  char *publicKey, size_t publicKeyLen);
  WalletSelfTest selfTest();
  void wipe(WalletOutput &out);
  static bool bip39Word(const char *word);
  static uint8_t bip39Suggestions(const char *prefix, char *suggestions,
                                  size_t stride, uint8_t maximum);
  static bool taprootAddress(const PublicKey &internalKey, char *out, size_t outLen);
};
