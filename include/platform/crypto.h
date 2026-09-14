#pragma once
#include <stddef.h>
#include <stdint.h>
#include <mbedtls/md.h>
#include <mbedtls/pkcs5.h>
#include <mbedtls/version.h>

// Same PBKDF2-HMAC construction on Arduino's Mbed TLS 2 and IDF's Mbed TLS 3.
// Only the library entry point changes, never salt, rounds, digest or output.
inline int auroraPbkdf2Hmac(mbedtls_md_type_t digest,
                          const uint8_t *password, size_t passwordLength,
                          const uint8_t *salt, size_t saltLength,
                          unsigned iterations, uint32_t outputLength, uint8_t *output) {
#if MBEDTLS_VERSION_NUMBER >= 0x03060000
  return mbedtls_pkcs5_pbkdf2_hmac_ext(digest, password, passwordLength,
                                     salt, saltLength, iterations, outputLength, output);
#else
  mbedtls_md_context_t context;
  mbedtls_md_init(&context);
  const mbedtls_md_info_t *info = mbedtls_md_info_from_type(digest);
  int result = info ? mbedtls_md_setup(&context, info, 1) : MBEDTLS_ERR_MD_BAD_INPUT_DATA;
  if (result == 0) result = mbedtls_pkcs5_pbkdf2_hmac(&context, password, passwordLength,
                            salt, saltLength, iterations, outputLength, output);
  mbedtls_md_free(&context);
  return result;
#endif
}
