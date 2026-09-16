// Chunk-owned SHA256 backend for the pinned PBKDF2 context only.
// Trezor uses numeric big-endian words; the IDF engine takes byte-ordered words.
#include <stdint.h>
#include "sha/sha_core.h"
#include "security_memory.h"
#include "utility/trezor/pbkdf2.h"

typedef struct { uint32_t state[8]; uint32_t block[16]; } KdfWork;
#if defined(AURORA_KDF_TEST)
extern void auroraP4KdfWorkAudit(const void *work, size_t length);
#endif

static void compress(const uint32_t *state, const uint32_t *block,
                     uint32_t *output, KdfWork *work) {
    for (unsigned i=0;i<8;++i) work->state[i]=__builtin_bswap32(state[i]);
    for (unsigned i=0;i<16;++i) work->block[i]=__builtin_bswap32(block[i]);
    esp_sha_write_digest_state(SHA2_256, work->state);
    esp_sha_block(SHA2_256, work->block, false);
    esp_sha_read_digest_state(SHA2_256, work->state);
    for (unsigned i=0;i<8;++i) output[i]=__builtin_bswap32(work->state[i]);
}

// Same loop and first-round convention as the pinned Trezor Update function.
// Workspace and accelerator stay owned for this bounded chunk (<= 2048 rounds).
// Nothing is allocated/released per compression; no TLS/global key cache.
void auroraP4KdfUpdate(PBKDF2_HMAC_SHA256_CTX *context, uint32_t iterations) {
    if (!context || !iterations || iterations > 2048 ||
        (context->first != 0 && context->first != 1))
        auroraSecurityPanic();
    KdfWork work = {0};
    esp_sha_acquire_hardware();
    esp_sha_set_mode(SHA2_256);
    for (uint32_t i=context->first;i<iterations;++i) {
        compress(context->idig,context->g,context->g,&work);
        compress(context->odig,context->g,context->g,&work);
        for (unsigned j=0;j<8;++j) context->f[j]^=context->g[j];
    }
    context->first=0;
    // The pinned overlay resets SHA (and its DMA) before unlocking.
    esp_sha_release_hardware();
    auroraSecureZero(&work,sizeof(work));
#if defined(AURORA_KDF_TEST)
    auroraP4KdfWorkAudit(&work,sizeof(work));
#endif
}

#if defined(AURORA_KDF_TEST)
void auroraP4KdfSha256(const uint32_t *state, const uint32_t *block, uint32_t *output) {
    KdfWork work;
    esp_sha_acquire_hardware();
    esp_sha_set_mode(SHA2_256);
    compress(state,block,output,&work);
    // The pinned overlay resets the SHA engine BEFORE releasing its lock.
    esp_sha_release_hardware();
    auroraSecureZero(&work, sizeof(work));
    auroraP4KdfWorkAudit(&work,sizeof(work));
}
#endif
