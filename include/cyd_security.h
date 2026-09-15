#pragma once
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
// Runs before the welcome screen. Never erases SD or unrelated flash regions.
bool auroraCydBootCleanup(void);
// LVGL terminal failure only; no allocation, GUI or filesystem operations.
#if defined(_MSC_VER)
__declspec(noreturn) void auroraCydUiFailure(void);
#else
void auroraCydUiFailure(void) __attribute__((noreturn));
#endif
void auroraCydWipeApplication(void);
#ifdef __cplusplus
}
#endif
