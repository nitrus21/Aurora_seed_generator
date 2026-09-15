#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <algorithm>

// Real pinned function bodies are extracted by the runner. Small test doubles
// observe lifecycle/error behavior only; algorithm KATs are separate.
static unsigned wipe78, mdWipes;
static size_t lastWipeLength;
static void memzero(void *p, size_t n) {
    lastWipeLength=n;
    volatile uint8_t *bytes=static_cast<volatile uint8_t *>(p);
    for(size_t i=0;i<n;++i) bytes[i]=0;
    for(size_t i=0;i<n;++i) assert(bytes[i]==0);
    if(n==78) ++wipe78;
}
enum { UNKNOWN_TYPE, P2WPKH, P2SH_P2WPKH, P2WSH, P2SH_P2WSH };
struct Network { uint8_t xprv[4],zprv[4],yprv[4],Zprv[4],Yprv[4]; };
struct SerializeStream {
    uint8_t bytes[78]{}; size_t size=0, capacity=78;
    size_t available() const { return capacity-size; }
    void write(uint8_t b) { assert(available()); bytes[size++]=b; }
};
struct HDPrivateKey {
    int type=UNKNOWN_TYPE;
    const Network *network;
    uint8_t depth=3,parentFingerprint[4]{},chainCode[32]{},num[32]{};
    uint32_t childNumber=0x01020304;
    size_t to_bytes(uint8_t *,size_t) const;
    size_t to_stream(SerializeStream *,size_t) const;
    int xprv(char *,size_t) const;
};
static uint8_t serialized[78];
static int encodeResult=111;
static int toBase58Check(const uint8_t *bytes,size_t n,char *,size_t) {
    assert(n==78); memcpy(serialized,bytes,n); return encodeResult;
}
#include "hd_cleanup_functions.inc"

struct md_info { size_t size=32,block_size=64; };
struct mbedtls_md_context_t { const md_info *md_info; void *hmac_ctx; };
constexpr int MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED=-1, MBEDTLS_ERR_MD_BAD_INPUT_DATA=-2;
constexpr size_t MBEDTLS_MD_MAX_SIZE=64;
static unsigned mdPhase,failPhase;
static int mdStep() { return ++mdPhase==failPhase ? -100-static_cast<int>(mdPhase) : 0; }
static int mbedtls_md_finish(mbedtls_md_context_t *,unsigned char *p) {
    memset(p,0x5a,32); return mdStep();
}
static int mbedtls_md_starts(mbedtls_md_context_t *) { return mdStep(); }
static int mbedtls_md_update(mbedtls_md_context_t *,const unsigned char *,size_t) { return mdStep(); }
static void mbedtls_platform_zeroize(void *p,size_t n) { ++mdWipes; memzero(p,n); }
#include "md_cleanup_function.inc"

int main() {
    Network net{{1,2,3,4},{5,6,7,8},{9,10,11,12},{13,14,15,16},{17,18,19,20}};
    HDPrivateKey key; key.network=&net;
    for(unsigned i=0;i<32;++i) { key.chainCode[i]=i+21; key.num[i]=i+81; }
    uint8_t expected[78]{}; memcpy(expected,net.xprv,4); expected[4]=key.depth;
    expected[9]=1; expected[10]=2; expected[11]=3; expected[12]=4;
    memcpy(expected+13,key.chainCode,32); memcpy(expected+46,key.num,32);
    for(size_t length : {size_t(0),size_t(17),size_t(78),size_t(100)}) {
        uint8_t output[100]; memset(output,0xa5,sizeof(output));
        const unsigned before=wipe78;
        const size_t n=key.to_bytes(output,length);
        assert(n==std::min(length,size_t(78)) && !memcmp(output,expected,n));
        assert(wipe78==before+1);
    }
    for(size_t offset : {size_t(0),size_t(10),size_t(77),size_t(78),size_t(90)}) {
        SerializeStream stream; stream.capacity=9;
        const unsigned before=wipe78;
        const size_t n=key.to_stream(&stream,offset);
        const size_t count=offset>=78 ? 0 : std::min(size_t(9),78-offset);
        assert(n==count && !memcmp(stream.bytes,expected+std::min(offset,size_t(78)),n));
        assert(wipe78==before+2);
    }
    for(int result : {0,111}) {
        char output[112]{}; encodeResult=result;
        const unsigned before=wipe78;
        assert(key.xprv(output,sizeof(output))==result && !memcmp(serialized,expected,78));
        assert(wipe78==before+2);
    }
    md_info info; uint8_t pads[128]{}; mbedtls_md_context_t ctx{&info,pads};
    for(unsigned failure=0;failure<=5;++failure) {
        uint8_t output[64]{}; mdPhase=0; failPhase=failure;
        const unsigned before=mdWipes;
        const int result=mbedtls_md_hmac_finish(&ctx,output);
        assert(result==(failure ? -100-static_cast<int>(failure) : 0));
        assert(mdWipes==before+1 && lastWipeLength==MBEDTLS_MD_MAX_SIZE);
    }
    uint8_t output[64]{}; const unsigned before=mdWipes;
    assert(mbedtls_md_hmac_finish(nullptr,output)==MBEDTLS_ERR_MD_BAD_INPUT_DATA);
    ctx.md_info=nullptr;
    assert(mbedtls_md_hmac_finish(&ctx,output)==MBEDTLS_ERR_MD_BAD_INPUT_DATA);
    ctx.md_info=&info; ctx.hmac_ctx=nullptr;
    assert(mbedtls_md_hmac_finish(&ctx,output)==MBEDTLS_ERR_MD_BAD_INPUT_DATA);
    assert(mdWipes==before+3);
    puts("PASS: raw HD serialization outputs and wipes, xprv failure/success, all HMAC finish errors wipe.");
}
