#pragma once

// Production default. Diagnostics may contain fixed event codes/timings only,
// never wallet data, passwords, paths supplied by users, or memory dumps.
#ifndef AURORA_DEBUG
#define AURORA_DEBUG 0
#endif
#if AURORA_DEBUG != 0 && AURORA_DEBUG != 1
#error "AURORA_DEBUG must be 0 (production) or 1 (diagnostics)"
#endif

#if AURORA_DEBUG
#include "platform/runtime.h"
#define AURORA_DIAG(...) do { Serial.printf(__VA_ARGS__); } while (0)
#else
// Arguments are not evaluated, formatted or copied into temporary buffers.
#define AURORA_DIAG(...) do { } while (0)
#endif
