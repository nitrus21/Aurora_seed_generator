#pragma once
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef void (*AuroraEmergencyWipe)(void);
void auroraSecuritySetEmergencyWipe(AuroraEmergencyWipe callback);
void auroraSecureZero(void *ptr, size_t len);
__attribute__((noreturn)) void auroraSecurityPanic(void);

// Boot only, before display/sensors/storage or any secret input. Does not erase
// user files or live SDK allocations. The counts describe owned heap payloads,
// not a forensic guarantee about all physical memory.
bool auroraStartupMemoryScrub(size_t *internalBytes, size_t *externalBytes);
#ifdef __cplusplus
}
#endif
