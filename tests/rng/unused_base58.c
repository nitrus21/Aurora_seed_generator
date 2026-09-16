// MSVC lacks C99 variable-length arrays used by the pinned Base58 codec.
// Neither codec is reachable from these point-multiplication tests. Abort if
// that assumption changes rather than simulating a successful encoding.
#include "base58.h"
#include <stdlib.h>
int base58_encode_check(const uint8_t *data, int datalen, HasherType type, char *str, int strsize) {
  abort();
}
int base58_decode_check(const char *str, HasherType type, uint8_t *data, int datalen) {
  abort();
}
