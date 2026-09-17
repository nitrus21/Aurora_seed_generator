#pragma once
#include <cstdio>
struct DiagnosticSerial {
  template<typename... T> void printf(const char *format, T... args) {
    std::printf(format, args...);
  }
};
inline DiagnosticSerial Serial;
