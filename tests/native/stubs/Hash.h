#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <cassert>
#include <cstdint>
#include <vector>

// Real SHA-256 / HMAC via Windows CNG, standing in for uBitcoin's same API.
inline void nativeDigest(const uint8_t *data, size_t size, uint8_t out[32],
                         const uint8_t *key = nullptr, size_t keySize = 0) {
  BCRYPT_ALG_HANDLE algorithm = nullptr;
  BCRYPT_HASH_HANDLE hash = nullptr;
  assert(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr,
      key ? BCRYPT_ALG_HANDLE_HMAC_FLAG : 0) == 0);
  assert(BCryptCreateHash(algorithm, &hash, nullptr, 0, const_cast<PUCHAR>(key),
      static_cast<ULONG>(keySize), 0) == 0);
  assert(BCryptHashData(hash, const_cast<PUCHAR>(data), static_cast<ULONG>(size), 0) == 0);
  assert(BCryptFinishHash(hash, out, 32, 0) == 0);
  BCryptDestroyHash(hash);
  BCryptCloseAlgorithmProvider(algorithm, 0);
}
class SHA256 {
 public:
  void begin() { bytes.clear(); }
  size_t write(const uint8_t *data, size_t size) {
    bytes.insert(bytes.end(), data, data + size);
    return size;
  }
  size_t end(uint8_t out[32]) {
    nativeDigest(bytes.data(), bytes.size(), out);
    return 32;
  }
 private:
  std::vector<uint8_t> bytes;
};
inline int sha256Hmac(const uint8_t *key, size_t keySize, const uint8_t *data,
                      size_t size, uint8_t out[32]) {
  nativeDigest(data, size, out, key, keySize);
  return 32;
}
