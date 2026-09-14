#include "pin_security.h"
#include "platform/crypto.h"
#include "hardware_rng.h"
#include "secure_memory.h"
#include <string.h>

bool auroraPinValid(const char *pin) {
  if (!pin) return false;
  size_t length = 0;
  while (length <= 8 && pin[length]) {
    if (pin[length] < '0' || pin[length] > '9') return false;
    ++length;
  }
  return length >= 4 && length <= 8;
}
bool auroraPinRecordValid(const AuroraPinRecord &record) {
  uint8_t present = 0;
  for (uint8_t value : record.verifier) present |= value;
  return present != 0;
}
namespace {
bool derivePin(const char *pin, const AuroraPinRecord &record, uint8_t (&out)[64]) {
  // Domain separation from the independent file-password KDF.
  uint8_t salt[32] = {'A','U','R','O','R','A','-','P','I','N','-','V','2',0,0,0};
  memcpy(salt + 16, record.salt, sizeof(record.salt));
  const bool ok = auroraPbkdf2Hmac(MBEDTLS_MD_SHA512,
      reinterpret_cast<const uint8_t *>(pin), strlen(pin), salt, sizeof(salt),
      AURORA_PIN_ITERATIONS, sizeof(out), out) == 0;
  secureZero(salt, sizeof(salt));
  return ok;
}
}
bool auroraPinCreate(const char *pin, AuroraPinRecord &record) {
  secureZero(&record, sizeof(record));
  if (!auroraPinValid(pin)) return false;
  hardwareRandomFill(record.salt, sizeof(record.salt));
  if (!derivePin(pin, record, record.verifier)) {
    secureZero(&record, sizeof(record));
    return false;
  }
  return true;
}
bool auroraPinVerify(const char *pin, const AuroraPinRecord &record) {
  if (!auroraPinValid(pin) || !auroraPinRecordValid(record)) return false;
  uint8_t derived[64]{};
  const bool ok = derivePin(pin, record, derived);
  uint8_t difference = 0;
  for (size_t i = 0; i < sizeof(derived); ++i) difference |= derived[i] ^ record.verifier[i];
  secureZero(derived, sizeof(derived));
  return ok && difference == 0;
}
void AuroraPinGuard::begin(const AuroraPinRecord &record) {
  clear();
  if (!auroraPinRecordValid(record)) return;
  record_ = record;
  enabled_ = true;
}
void AuroraPinGuard::clear() {
  secureZero(&record_, sizeof(record_));
  failures_ = 0;
  enabled_ = false;
}
bool AuroraPinGuard::attempt(const char *pin) {
  if (!enabled_ || blocked()) return false;
  if (auroraPinVerify(pin, record_)) return true;
  ++failures_;
  return false;
}
