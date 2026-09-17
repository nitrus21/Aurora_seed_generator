#pragma once
#include <stddef.h>
#include <stdint.h>
#include "pin_security.h"

// Aurora Wallet V1 is a device-independent wire format. Layout/algorithm changes
// require a format migration. Writer cost is policy stored in the authenticated
// header; different costs never change the layout or reader compatibility.
namespace AuroraWalletFormat {
constexpr uint8_t FILE_MAGIC[8] = {'A', 'U', 'R', 'O', 'R', 'A', 'W', '1'};
constexpr uint8_t PAYLOAD_MAGIC[8] = {'A', 'U', 'R', 'D', 'A', 'T', '0', '1'};
constexpr uint8_t FILE_VERSION = 1;
constexpr uint8_t KDF_PBKDF2_HMAC_SHA256 = 1;
constexpr uint8_t CIPHER_AES_256_GCM = 1;
// New exports only; existing files retain their authenticated iteration count.
#if defined(AURORA_BOARD_CYD) && !defined(AURORA_BOARD_P4)
// User-selected CYD latency trade-off (1.9.9); no change to P4 or reader bounds.
constexpr uint32_t KDF_ITERATIONS = 120000;
#else
constexpr uint32_t KDF_ITERATIONS = 500000;
#endif
constexpr uint32_t KDF_ITERATIONS_MIN = 10000;
constexpr uint32_t KDF_ITERATIONS_MAX = 500000;
static_assert(KDF_ITERATIONS >= KDF_ITERATIONS_MIN && KDF_ITERATIONS <= KDF_ITERATIONS_MAX,
              "Writer KDF must remain readable by the compatible V1/V2 readers");
constexpr size_t HEADER_SIZE = 46;
constexpr size_t SALT_OFFSET = 16;
constexpr size_t SALT_SIZE = 16;
constexpr size_t NONCE_OFFSET = 32;
constexpr size_t NONCE_SIZE = 12;
constexpr size_t TAG_SIZE = 16;
constexpr size_t KEY_SIZE = 32;
// Wire layouts stay frozen. P4 password-only files reuse V1 (same AES-GCM and
// password KDF as V2, without the obsolete PIN record). Both UIs write V1.
constexpr uint8_t FILE_MAGIC_V2[8] = {'A','U','R','O','R','A','W','2'};
constexpr uint8_t PAYLOAD_MAGIC_V2[8] = {'A','U','R','D','A','T','0','2'};
constexpr uint8_t FILE_VERSION_V2 = 2;
// V1/V2 use PBKDF2-HMAC-SHA-256 with the authenticated header's iteration count.
// Legacy 120000-round files remain readable. The PIN never changes an encryption key.

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
struct AuroraPayloadV2 {
  AuroraPayloadV1 wallet;
  AuroraPinRecord pin;
};
#pragma pack(pop)
static_assert(sizeof(AuroraPayloadV2) == 1138, "Aurora Wallet V2 payload size changed");
static_assert(offsetof(AuroraPayloadV2, pin) == 1058, "Aurora Wallet V2 PIN offset changed");
static_assert(sizeof(AuroraPayloadV1) == 1058, "Aurora Wallet V1 payload size changed");
static_assert(offsetof(AuroraPayloadV1, mnemonic) == 98, "Aurora Wallet V1 mnemonic offset changed");
static_assert(offsetof(AuroraPayloadV1, receiveDescriptor) == 834, "Aurora Wallet V1 descriptor offset changed");
}
