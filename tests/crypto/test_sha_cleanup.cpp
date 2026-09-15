#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <initializer_list>
#include <cassert>
extern "C" {
#include "utility/trezor/sha2.h"
#include "utility/trezor/hmac.h"
}

// Only synthetic public inputs, in this test process. No device access.
static void *mainFiber;
static uintptr_t stackAddress;
static uint32_t block256[16], state256[8], out256[8];
static uint64_t block512[16], state512[8], out512[8];
static unsigned mode;
static uint32_t rr32(uint32_t x, unsigned n) { return (x >> n) | (x << (32-n)); }
static uint64_t rr64(uint64_t x, unsigned n) { return (x >> n) | (x << (64-n)); }
static uint32_t s032(uint32_t x) { return rr32(x,7)^rr32(x,18)^(x>>3); }
static uint32_t s132(uint32_t x) { return rr32(x,17)^rr32(x,19)^(x>>10); }
static uint64_t s064(uint64_t x) { return rr64(x,1)^rr64(x,8)^(x>>7); }
static uint64_t s164(uint64_t x) { return rr64(x,19)^rr64(x,61)^(x>>6); }

__declspec(noinline) static void transform() {
    // Keep returned crypto frames away from the live SwitchToFiber frame.
    volatile unsigned char pad[16384]{};
    stackAddress = reinterpret_cast<uintptr_t>(&pad[0]);
    if (mode == 256) sha256_Transform(state256,block256,out256);
    else sha512_Transform(state512,block512,out512);
    _ReadWriteBarrier();
    if (pad[0]) std::abort();
}
static VOID CALLBACK worker(void *) {
    transform();
    SecureZeroMemory(block256,sizeof(block256));
    SecureZeroMemory(state256,sizeof(state256));
    SecureZeroMemory(out256,sizeof(out256));
    SecureZeroMemory(block512,sizeof(block512));
    SecureZeroMemory(state512,sizeof(state512));
    SecureZeroMemory(out512,sizeof(out512));
    SwitchToFiber(mainFiber);
}
static void expectHex(const uint8_t *actual, size_t size, const char *expected) {
    static const char digits[] = "0123456789abcdef";
    assert(strlen(expected) == size*2);
    for(size_t i=0; i<size; ++i) {
        assert(expected[2*i] == digits[actual[i]>>4]);
        assert(expected[2*i+1] == digits[actual[i]&15]);
    }
}
static void vectors() {
    uint8_t digest[64]{};
    sha256_Raw(reinterpret_cast<const uint8_t *>("abc"),3,digest);
    expectHex(digest,32,"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    sha512_Raw(reinterpret_cast<const uint8_t *>("abc"),3,digest);
    expectHex(digest,64,"ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f");
    uint8_t key[20]; memset(key,0x0b,sizeof(key));
    ubtc_hmac_sha256(key,sizeof(key),reinterpret_cast<const uint8_t *>("Hi There"),8,digest);
    expectHex(digest,32,"b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");
    ubtc_hmac_sha512(key,sizeof(key),reinterpret_cast<const uint8_t *>("Hi There"),8,digest);
    expectHex(digest,64,"87aa7cdea5ef619d4ff0b4241a1d6cb02379f4e2ce4ec2787ad0b30545e17cdedaa833b7d6b8a702038b274eaea3f4e4be9d914eeb61f1702e696c203a126854");
}
int main(int argc, char **argv) {
    const bool expectResidue = argc == 2 && !strcmp(argv[1],"--expect-unpatched-residue");
    vectors();
    mainFiber=ConvertThreadToFiber(nullptr);
    if(!mainFiber) return 2;
    for(unsigned algorithm : {256u,512u}) {
        mode=algorithm;
        uint32_t schedule256[64]{}; uint64_t schedule512[80]{};
        for(unsigned i=0;i<16;++i) {
            block256[i]=schedule256[i]=0x1d2c3b4au+0x04030201u*i;
            block512[i]=schedule512[i]=0x1827364554637281ull+0x01030507090b0d0full*i;
        }
        for(unsigned i=16;i<64;++i) schedule256[i]=schedule256[i-16]+s032(schedule256[i-15])+schedule256[i-7]+s132(schedule256[i-2]);
        for(unsigned i=16;i<80;++i) schedule512[i]=schedule512[i-16]+s064(schedule512[i-15])+schedule512[i-7]+s164(schedule512[i-2]);
        void *fiber=CreateFiberEx(128*1024,256*1024,0,worker,nullptr);
        if(!fiber) return 3;
        SwitchToFiber(fiber);
        unsigned char snapshot[12288]; SIZE_T read=0;
        if(!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void *>(stackAddress-8192),snapshot,sizeof(snapshot),&read)) return 4;
        const void *needle=mode==256?static_cast<void *>(schedule256+48):static_cast<void *>(schedule512+64);
        const size_t length=mode==256?64:128;
        size_t hits=0;
        for(size_t pos=0;pos+length<=read;++pos) if(!memcmp(snapshot+pos,needle,length)) ++hits;
        printf("SHA-%u optimized returned stack: schedule copies=%zu (expected %s)\n",mode,hits,expectResidue?"present":"absent");
        DeleteFiber(fiber);
        if(expectResidue != (hits != 0)) return 5;
    }
    puts("PASS: SHA-256/512 and HMAC vectors, returned-stack schedule checks.");
    return 0;
}
