#pragma once
#include <stddef.h>
#include <stdint.h>

// An interface lock, NOT a second encryption factor. This verifier belongs
// inside the authenticated, encrypted wallet payload and never stores the PIN.
struct AuroraPinRecord {
  uint8_t salt[16];
  uint8_t verifier[64];
};
static_assert(sizeof(AuroraPinRecord) == 80, "PIN wire record changed");
constexpr uint32_t AURORA_PIN_ITERATIONS = 10000;
bool auroraPinValid(const char *pin);
bool auroraPinRecordValid(const AuroraPinRecord &record);
bool auroraPinCreate(const char *pin, AuroraPinRecord &record);
bool auroraPinVerify(const char *pin, const AuroraPinRecord &record);

// Failure count is cumulative within a loaded session, including across
// cancelled dialogs and successful authorizations. Only closing clears it.
class AuroraPinGuard {
 public:
  void begin(const AuroraPinRecord &record);
  void clear();
  bool enabled() const { return enabled_; }
  bool blocked() const { return failures_ >= 3; }
  uint8_t failures() const { return failures_; }
  const AuroraPinRecord &record() const { return record_; }
  bool attempt(const char *pin);
 private:
  AuroraPinRecord record_{};
  uint8_t failures_ = 0;
  bool enabled_ = false;
};
