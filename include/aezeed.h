#pragma once

#include <stddef.h>
#include <stdint.h>

enum class AezeedResult : uint8_t {
  Ok = 0,
  InvalidFormat,
  UnknownWord,
  UnsupportedVersion,
  InvalidChecksum,
  InvalidPassphrase,
  MemoryFailed,
  CryptoFailed
};

struct AezeedDecoded {
  uint8_t internalVersion;
  uint16_t birthdayDays;
  uint8_t entropy[16];
};

class AezeedEngine {
 public:
  // Decodes an LND aezeed (24 English words). An empty passphrase uses the
  // aezeed-defined default "aezeed" value. The scrypt parameters are the
  // official N=32768, r=8, p=1 and are deliberately not weakened.
  static AezeedResult decode(const char *mnemonic, const char *passphrase,
                             AezeedDecoded &out);
  static void wipe(AezeedDecoded &out);
};
