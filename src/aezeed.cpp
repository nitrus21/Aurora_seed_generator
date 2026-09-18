#include "aezeed.h"

// AEZ/BLAKE2b (CC0/public domain), bsaes ct32 (MIT), and scrypt ROMix
// (Colin Percival, BSD-2-Clause). See THIRD_PARTY_NOTICES.md.

#include "platform/crypto.h"
#include "platform/runtime.h"
#include "secure_memory.h"

#include <mbedtls/md.h>
#if defined(AURORA_NATIVE_TEST)
extern "C" const char *const *mnemonic_wordlist(void);
#else
#include <utility/trezor/bip39.h>
#endif

#include <stdlib.h>
#include <string.h>

#if defined(AURORA_BOARD_P4) && !defined(AURORA_NATIVE_TEST)
#include <esp_heap_caps.h>
#endif

namespace {
constexpr size_t AEZEED_BYTES = 33;
constexpr size_t CIPHERTEXT_BYTES = 23;
constexpr size_t AEZEED_SALT_OFFSET = 24;
constexpr size_t AEZEED_CHECKSUM_OFFSET = 29;
constexpr uint8_t AEZEED_EXTERNAL_VERSION = 0;
constexpr uint8_t AEZEED_INTERNAL_VERSION_LEGACY = 0;
constexpr uint8_t AEZEED_INTERNAL_VERSION_TAPROOT = 1;
constexpr uint32_t SCRYPT_N = 32768;
constexpr size_t SCRYPT_R = 8;
// Exact ROMix with a factor-2 time/memory tradeoff: store every even V state
// and recompute an odd state from its predecessor when needed. This preserves
// LND's N/r/p output bit-for-bit while fitting comfortably in 32 MiB PSRAM.
constexpr size_t SCRYPT_V_BYTES = 128 * SCRYPT_R * (SCRYPT_N / 2);

uint32_t load32le(const uint8_t *p) {
  return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

void store32le(uint8_t *p, uint32_t value) {
  p[0] = static_cast<uint8_t>(value);
  p[1] = static_cast<uint8_t>(value >> 8);
  p[2] = static_cast<uint8_t>(value >> 16);
  p[3] = static_cast<uint8_t>(value >> 24);
}

uint64_t load64le(const uint8_t *p) {
  uint64_t value = 0;
  for (unsigned i = 0; i < 8; ++i) value |= static_cast<uint64_t>(p[i]) << (8 * i);
  return value;
}

uint32_t crc32c(const uint8_t *data, size_t length) {
  uint32_t crc = 0xffffffffu;
  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (unsigned bit = 0; bit < 8; ++bit)
      crc = (crc >> 1) ^ (0x82f63b78u & (0u - (crc & 1u)));
  }
  return ~crc;
}

int compareWord(const char *word, size_t length, const char *candidate) {
  const size_t candidateLength = strlen(candidate);
  const size_t common = length < candidateLength ? length : candidateLength;
  const int result = memcmp(word, candidate, common);
  if (result) return result;
  return length < candidateLength ? -1 : (length > candidateLength ? 1 : 0);
}

bool wordIndex(const char *word, size_t length, uint16_t &index) {
  const char *const *words = mnemonic_wordlist();
  int low = 0, high = 2047;
  while (words && low <= high) {
    const int middle = low + (high - low) / 2;
    const int comparison = compareWord(word, length, words[middle]);
    if (!comparison) { index = static_cast<uint16_t>(middle); return true; }
    if (comparison < 0) high = middle - 1; else low = middle + 1;
  }
  return false;
}

AezeedResult mnemonicBytes(const char *mnemonic, uint8_t out[AEZEED_BYTES]) {
  if (!mnemonic) return AezeedResult::InvalidFormat;
  memset(out, 0, AEZEED_BYTES);
  const char *cursor = mnemonic;
  unsigned bitPosition = 0;
  for (unsigned count = 0; count < 24; ++count) {
    while (*cursor == ' ') ++cursor;
    const char *start = cursor;
    while (*cursor && *cursor != ' ') ++cursor;
    const size_t length = static_cast<size_t>(cursor - start);
    uint16_t index = 0;
    if (!length || !wordIndex(start, length, index)) return AezeedResult::UnknownWord;
    for (int bit = 10; bit >= 0; --bit, ++bitPosition) {
      const uint8_t value = static_cast<uint8_t>((index >> bit) & 1u);
      out[bitPosition >> 3] |= static_cast<uint8_t>(value << (7 - (bitPosition & 7)));
    }
  }
  while (*cursor == ' ') ++cursor;
  return *cursor ? AezeedResult::InvalidFormat : AezeedResult::Ok;
}

// Compact public-domain BLAKE2b reference, used by AEZ's key extraction.
struct Blake2b {
  uint8_t buffer[128];
  uint64_t h[8];
  uint64_t count[2];
  size_t used;
};

constexpr uint64_t BLAKE_IV[8] = {
    0x6A09E667F3BCC908ULL,0xBB67AE8584CAA73BULL,0x3C6EF372FE94F82BULL,0xA54FF53A5F1D36F1ULL,
    0x510E527FADE682D1ULL,0x9B05688C2B3E6C1FULL,0x1F83D9ABFB41BD6BULL,0x5BE0CD19137E2179ULL};
constexpr uint8_t BLAKE_SIGMA[12][16] = {
  {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15},{14,10,4,8,9,15,13,6,1,12,0,2,11,7,5,3},
  {11,8,12,0,5,2,15,13,10,14,3,6,7,1,9,4},{7,9,3,1,13,12,11,14,2,6,5,10,4,0,15,8},
  {9,0,5,7,2,4,10,15,14,1,11,12,6,8,3,13},{2,12,6,10,0,11,8,3,4,13,7,5,15,14,1,9},
  {12,5,1,15,14,13,4,10,0,7,6,3,9,2,8,11},{13,11,7,14,12,1,3,9,5,0,15,4,8,6,2,10},
  {6,15,14,9,11,3,0,8,12,2,13,7,1,4,10,5},{10,2,8,4,7,6,1,5,15,11,9,14,3,12,13,0},
  {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15},{14,10,4,8,9,15,13,6,1,12,0,2,11,7,5,3}};

uint64_t rotr64(uint64_t x, unsigned n) { return (x >> n) | (x << (64 - n)); }
void blakeG(uint64_t v[16], int a, int b, int c, int d, uint64_t x, uint64_t y) {
  v[a]+=v[b]+x; v[d]=rotr64(v[d]^v[a],32); v[c]+=v[d]; v[b]=rotr64(v[b]^v[c],24);
  v[a]+=v[b]+y; v[d]=rotr64(v[d]^v[a],16); v[c]+=v[d]; v[b]=rotr64(v[b]^v[c],63);
}
void blakeCompress(Blake2b &ctx, bool last) {
  uint64_t v[16], m[16];
  for (int i=0;i<8;++i) { v[i]=ctx.h[i]; v[i+8]=BLAKE_IV[i]; m[i]=load64le(ctx.buffer+8*i); m[i+8]=load64le(ctx.buffer+8*(i+8)); }
  v[12]^=ctx.count[0]; v[13]^=ctx.count[1]; if(last) v[14]=~v[14];
  for (int i=0;i<12;++i) {
    const uint8_t *s=BLAKE_SIGMA[i];
    blakeG(v,0,4,8,12,m[s[0]],m[s[1]]); blakeG(v,1,5,9,13,m[s[2]],m[s[3]]);
    blakeG(v,2,6,10,14,m[s[4]],m[s[5]]); blakeG(v,3,7,11,15,m[s[6]],m[s[7]]);
    blakeG(v,0,5,10,15,m[s[8]],m[s[9]]); blakeG(v,1,6,11,12,m[s[10]],m[s[11]]);
    blakeG(v,2,7,8,13,m[s[12]],m[s[13]]); blakeG(v,3,4,9,14,m[s[14]],m[s[15]]);
  }
  for(int i=0;i<8;++i) ctx.h[i]^=v[i]^v[i+8];
  secureZero(v,sizeof(v)); secureZero(m,sizeof(m));
}
void blake2b48(const uint8_t *input, size_t length, uint8_t out[48]) {
  Blake2b ctx{}; for(int i=0;i<8;++i) ctx.h[i]=BLAKE_IV[i]; ctx.h[0]^=0x01010030;
  while(length) {
    const size_t take=(128-ctx.used)<length?(128-ctx.used):length;
    memcpy(ctx.buffer+ctx.used,input,take); ctx.used+=take; input+=take; length-=take;
    if(ctx.used==128 && length) { ctx.count[0]+=128; if(ctx.count[0]<128) ++ctx.count[1]; blakeCompress(ctx,false); ctx.used=0; }
  }
  ctx.count[0]+=ctx.used; if(ctx.count[0]<ctx.used) ++ctx.count[1];
  memset(ctx.buffer+ctx.used,0,128-ctx.used); blakeCompress(ctx,true);
  for(size_t i=0;i<48;++i) out[i]=static_cast<uint8_t>(ctx.h[i>>3]>>(8*(i&7)));
  secureZero(&ctx,sizeof(ctx));
}

void xorBlock(const uint8_t *a,const uint8_t *b,uint8_t *out) { for(int i=0;i<16;++i) out[i]=a[i]^b[i]; }
void xor4(const uint8_t *a,const uint8_t *b,const uint8_t *c,const uint8_t *d,uint8_t *out) { for(int i=0;i<16;++i) out[i]=a[i]^b[i]^c[i]^d[i]; }

void ortho(uint32_t q[8]) {
  uint32_t a,b;
#define SWAPN(i,j,lo,hi,n) a=q[i]; b=q[j]; q[i]=(a&(lo))|((b&(lo))<<(n)); q[j]=((a&(hi))>>(n))|(b&(hi))
  SWAPN(0,1,0x55555555u,0xAAAAAAAAu,1); SWAPN(2,3,0x55555555u,0xAAAAAAAAu,1);
  SWAPN(4,5,0x55555555u,0xAAAAAAAAu,1); SWAPN(6,7,0x55555555u,0xAAAAAAAAu,1);
  SWAPN(0,2,0x33333333u,0xCCCCCCCCu,2); SWAPN(1,3,0x33333333u,0xCCCCCCCCu,2);
  SWAPN(4,6,0x33333333u,0xCCCCCCCCu,2); SWAPN(5,7,0x33333333u,0xCCCCCCCCu,2);
  SWAPN(0,4,0x0F0F0F0Fu,0xF0F0F0F0u,4); SWAPN(1,5,0x0F0F0F0Fu,0xF0F0F0F0u,4);
  SWAPN(2,6,0x0F0F0F0Fu,0xF0F0F0F0u,4); SWAPN(3,7,0x0F0F0F0Fu,0xF0F0F0F0u,4);
#undef SWAPN
}

// Boyar-Peralta bitsliced AES S-box (constant-time).
void aesSbox(uint32_t q[8]) {
  uint32_t x0=q[7],x1=q[6],x2=q[5],x3=q[4],x4=q[3],x5=q[2],x6=q[1],x7=q[0];
  uint32_t y14=x3^x5,y13=x0^x6,y9=x0^x3,y8=x0^x5,t0=x1^x2,y1=t0^x7,y4=y1^x3;
  uint32_t y12=y13^y14,y2=y1^x0,y5=y1^x6,y3=y5^y8,t1=x4^y12,y15=t1^x5,y20=t1^x1;
  uint32_t y6=y15^x7,y10=y15^t0,y11=y20^y9,y7=x7^y11,y17=y10^y11,y19=y10^y8;
  uint32_t y16=t0^y11,y21=y13^y16,y18=x0^y16;
  uint32_t t2=y12&y15,t3=y3&y6,t4=t3^t2,t5=y4&x7,t6=t5^t2,t7=y13&y16,t8=y5&y1,t9=t8^t7;
  uint32_t t10=y2&y7,t11=t10^t7,t12=y9&y11,t13=y14&y17,t14=t13^t12,t15=y8&y10,t16=t15^t12;
  uint32_t t17=t4^t14,t18=t6^t16,t19=t9^t14,t20=t11^t16,t21=t17^y20,t22=t18^y19;
  uint32_t t23=t19^y21,t24=t20^y18,t25=t21^t22,t26=t21&t23,t27=t24^t26,t28=t25&t27;
  uint32_t t29=t28^t22,t30=t23^t24,t31=t22^t26,t32=t31&t30,t33=t32^t24,t34=t23^t33;
  uint32_t t35=t27^t33,t36=t24&t35,t37=t36^t34,t38=t27^t36,t39=t29&t38,t40=t25^t39;
  uint32_t t41=t40^t37,t42=t29^t33,t43=t29^t40,t44=t33^t37,t45=t42^t41;
  uint32_t z0=t44&y15,z1=t37&y6,z2=t33&x7,z3=t43&y16,z4=t40&y1,z5=t29&y7;
  uint32_t z6=t42&y11,z7=t45&y17,z8=t41&y10,z9=t44&y12,z10=t37&y3,z11=t33&y4;
  uint32_t z12=t43&y13,z13=t40&y5,z14=t29&y2,z15=t42&y9,z16=t45&y14,z17=t41&y8;
  uint32_t t46=z15^z16,t47=z10^z11,t48=z5^z13,t49=z9^z10,t50=z2^z12,t51=z2^z5;
  uint32_t t52=z7^z8,t53=z0^z3,t54=z6^z7,t55=z16^z17,t56=z12^t48,t57=t50^t53;
  uint32_t t58=z4^t46,t59=z3^t54,t60=t46^t57,t61=z14^t57,t62=t52^t58,t63=t49^t58;
  uint32_t t64=z4^t59,t65=t61^t62,t66=z1^t63,s0=t59^t63,s6=t56^(~t62),s7=t48^(~t60);
  uint32_t t67=t64^t65,s3=t53^t66,s4=t51^t66,s5=t47^t65,s1=t64^(~s3),s2=t55^(~t67);
  q[7]=s0;q[6]=s1;q[5]=s2;q[4]=s3;q[3]=s4;q[2]=s5;q[1]=s6;q[0]=s7;
}

uint32_t rotr16(uint32_t x) { return (x<<16)|(x>>16); }
void shiftRows(uint32_t q[8]) { for(int i=0;i<8;++i) { const uint32_t x=q[i]; q[i]=(x&0xff)|((x&0xfc00)>>2)|((x&0x0300)<<6)|((x&0xf00000)>>4)|((x&0x0f0000)<<4)|((x&0xc0000000)>>6)|((x&0x3f000000)<<2); } }
void mixColumns(uint32_t q[8]) {
  uint32_t r[8]; for(int i=0;i<8;++i) r[i]=(q[i]>>8)|(q[i]<<24);
  const uint32_t q0=q[0],q1=q[1],q2=q[2],q3=q[3],q4=q[4],q5=q[5],q6=q[6],q7=q[7];
  q[0]=q7^r[7]^r[0]^rotr16(q0^r[0]); q[1]=q0^r[0]^q7^r[7]^r[1]^rotr16(q1^r[1]);
  q[2]=q1^r[1]^r[2]^rotr16(q2^r[2]); q[3]=q2^r[2]^q7^r[7]^r[3]^rotr16(q3^r[3]);
  q[4]=q3^r[3]^q7^r[7]^r[4]^rotr16(q4^r[4]); q[5]=q4^r[4]^r[5]^rotr16(q5^r[5]);
  q[6]=q5^r[5]^r[6]^rotr16(q6^r[6]); q[7]=q6^r[6]^r[7]^rotr16(q7^r[7]);
}
void rkeyOrtho(uint32_t q[8],const uint8_t key[16]) { for(int i=0;i<4;++i) q[2*i]=q[2*i+1]=load32le(key+4*i); ortho(q); for(int i=0;i<4;++i){uint32_t x=(q[2*i]&0x55555555u)|(q[2*i+1]&0xAAAAAAAAu),y=x; x&=0x55555555u;q[2*i]=x|(x<<1);y&=0xAAAAAAAAu;q[2*i+1]=y|(y>>1);} }
void aesRound(uint32_t q[8],const uint32_t key[8]) { aesSbox(q);shiftRows(q);mixColumns(q);for(int i=0;i<8;++i)q[i]^=key[i]; }
void loadBlock(uint32_t q[8],const uint8_t src[16]) { q[0]=load32le(src);q[2]=load32le(src+4);q[4]=load32le(src+8);q[6]=load32le(src+12);q[1]=q[3]=q[5]=q[7]=0;ortho(q); }
void storeBlock(uint8_t dst[16],uint32_t q[8]) { ortho(q);store32le(dst,q[0]);store32le(dst+4,q[2]);store32le(dst+8,q[4]);store32le(dst+12,q[6]); }

struct AezState {
  uint8_t I[2][16]{},J[3][16]{},L[8][16]{};
  uint32_t keys[32]{};
};
void doubleBlock(uint8_t block[16]) { const uint8_t high=block[0]>>7; for(int i=0;i<15;++i)block[i]=static_cast<uint8_t>((block[i]<<1)|(block[i+1]>>7)); block[15]=static_cast<uint8_t>((block[15]<<1)^(0x87u&(0u-high))); }
void aes4(AezState &s,const uint8_t j[16],const uint8_t i[16],const uint8_t l[16],const uint8_t src[16],uint8_t dst[16]) { uint32_t q[8];xor4(j,i,l,src,dst);loadBlock(q,dst);aesRound(q,s.keys+8);aesRound(q,s.keys);aesRound(q,s.keys+16);aesRound(q,s.keys+24);storeBlock(dst,q);secureZero(q,sizeof(q)); }
void initAez(AezState &s,const uint8_t key[32]) { uint8_t extracted[48];blake2b48(key,32,extracted);memcpy(s.I[0],extracted,16);memcpy(s.I[1],s.I[0],16);doubleBlock(s.I[1]);memcpy(s.J[0],extracted+16,16);memcpy(s.J[1],s.J[0],16);doubleBlock(s.J[1]);memcpy(s.J[2],s.J[1],16);doubleBlock(s.J[2]);memcpy(s.L[1],extracted+32,16);memcpy(s.L[2],s.L[1],16);doubleBlock(s.L[2]);xorBlock(s.L[2],s.L[1],s.L[3]);memcpy(s.L[4],s.L[2],16);doubleBlock(s.L[4]);xorBlock(s.L[4],s.L[1],s.L[5]);memcpy(s.L[6],s.L[3],16);doubleBlock(s.L[6]);xorBlock(s.L[6],s.L[1],s.L[7]);for(int n=0;n<3;++n)rkeyOrtho(s.keys+8*n,extracted+16*n);secureZero(extracted,sizeof(extracted)); }

void aezHash(AezState &s,const uint8_t ad[6],uint8_t delta[16]) {
  uint8_t buffer[16]{},sum[16]{},j[16]; buffer[15]=32;
  xorBlock(s.J[0],s.J[1],j); aes4(s,j,s.I[1],s.L[1],buffer,sum);
  memset(buffer,0,16);buffer[0]=0x80;aes4(s,s.J[2],s.I[0],s.L[0],buffer,buffer);xorBlock(sum,buffer,sum);
  uint8_t j4[16];memcpy(j4,s.J[2],16);xorBlock(j4,s.J[0],j); // 5J
  memset(buffer,0,16);memcpy(buffer,ad,6);buffer[6]=0x80;aes4(s,j,s.I[0],s.L[0],buffer,buffer);xorBlock(sum,buffer,delta);
  secureZero(buffer,sizeof(buffer));secureZero(sum,sizeof(sum));secureZero(j,sizeof(j));secureZero(j4,sizeof(j4));
}
void aezTinyDecrypt(AezState &s,const uint8_t delta[16],const uint8_t in[CIPHERTEXT_BYTES],uint8_t out[CIPHERTEXT_BYTES]) {
  constexpr unsigned length=CIPHERTEXT_BYTES, half=(length+1)/2, rounds=8;
  static constexpr uint8_t ZERO[16]{};
  uint8_t buffer[32]{},left[16]{},right[16]{},tmp[16];
  uint8_t mask=0xf0,pad=0x08;
  memcpy(left,in,half);memcpy(right,in+length/2,half);
  for(unsigned k=0;k<length/2;++k) right[k]=static_cast<uint8_t>((right[k]<<4)|(right[k+1]>>4));
  right[length/2]<<=4;
  for(unsigned k=0,j=rounds-1;k<rounds/2;++k,j-=2) {
    memset(buffer,0,16);memcpy(buffer,right,half);buffer[length/2]=static_cast<uint8_t>((buffer[length/2]&mask)|pad);xorBlock(buffer,delta,buffer);buffer[15]^=static_cast<uint8_t>(j);aes4(s,ZERO,s.I[1],s.L[6],buffer,tmp);xorBlock(left,tmp,left);
    memset(buffer,0,16);memcpy(buffer,left,half);buffer[length/2]=static_cast<uint8_t>((buffer[length/2]&mask)|pad);xorBlock(buffer,delta,buffer);buffer[15]^=static_cast<uint8_t>(j-1);aes4(s,ZERO,s.I[1],s.L[6],buffer,tmp);xorBlock(right,tmp,right);
  }
  memcpy(buffer,right,length/2);memcpy(buffer+length/2,left,half);
  for(unsigned k=length-1;k>length/2;--k) buffer[k]=static_cast<uint8_t>((buffer[k]>>4)|(buffer[k-1]<<4));
  buffer[length/2]=static_cast<uint8_t>((left[0]>>4)|(right[length/2]&0xf0));
  memcpy(out,buffer,length);
  secureZero(buffer,sizeof(buffer));secureZero(left,sizeof(left));secureZero(right,sizeof(right));secureZero(tmp,sizeof(tmp));
}

void blockXor(uint32_t *dest,const uint32_t *src,size_t bytes) { for(size_t i=0;i<bytes/4;++i)dest[i]^=src[i]; }
uint32_t rotl32(uint32_t x,unsigned n) { return (x<<n)|(x>>(32-n)); }
void salsa20_8(uint32_t b[16]) {
  uint32_t x[16];memcpy(x,b,64);
  for(unsigned i=0;i<8;i+=2) {
#define R(a,bits) rotl32((a),(bits))
    x[4]^=R(x[0]+x[12],7);x[8]^=R(x[4]+x[0],9);x[12]^=R(x[8]+x[4],13);x[0]^=R(x[12]+x[8],18);
    x[9]^=R(x[5]+x[1],7);x[13]^=R(x[9]+x[5],9);x[1]^=R(x[13]+x[9],13);x[5]^=R(x[1]+x[13],18);
    x[14]^=R(x[10]+x[6],7);x[2]^=R(x[14]+x[10],9);x[6]^=R(x[2]+x[14],13);x[10]^=R(x[6]+x[2],18);
    x[3]^=R(x[15]+x[11],7);x[7]^=R(x[3]+x[15],9);x[11]^=R(x[7]+x[3],13);x[15]^=R(x[11]+x[7],18);
    x[1]^=R(x[0]+x[3],7);x[2]^=R(x[1]+x[0],9);x[3]^=R(x[2]+x[1],13);x[0]^=R(x[3]+x[2],18);
    x[6]^=R(x[5]+x[4],7);x[7]^=R(x[6]+x[5],9);x[4]^=R(x[7]+x[6],13);x[5]^=R(x[4]+x[7],18);
    x[11]^=R(x[10]+x[9],7);x[8]^=R(x[11]+x[10],9);x[9]^=R(x[8]+x[11],13);x[10]^=R(x[9]+x[8],18);
    x[12]^=R(x[15]+x[14],7);x[13]^=R(x[12]+x[15],9);x[14]^=R(x[13]+x[12],13);x[15]^=R(x[14]+x[13],18);
#undef R
  }
  for(int i=0;i<16;++i)b[i]+=x[i];secureZero(x,sizeof(x));
}
void blockMix(const uint32_t *input,uint32_t *output,uint32_t *x,size_t r) { memcpy(x,input+(2*r-1)*16,64);for(size_t i=0;i<2*r;i+=2){blockXor(x,input+i*16,64);salsa20_8(x);memcpy(output+i*8,x,64);blockXor(x,input+i*16+16,64);salsa20_8(x);memcpy(output+i*8+r*16,x,64);} }
uint64_t integerify(const uint32_t *b,size_t r) { const uint32_t *x=b+(2*r-1)*16;return (static_cast<uint64_t>(x[1])<<32)|x[0]; }
void smix(uint8_t *b,uint32_t *v,uint32_t *xy) { uint32_t *x=xy,*y=xy+32*SCRYPT_R,*z=xy+64*SCRYPT_R,*reconstructed=z+16;for(size_t k=0;k<32*SCRYPT_R;++k)x[k]=load32le(b+4*k);for(uint64_t i=0;i<SCRYPT_N;i+=2){memcpy(v+(i/2)*32*SCRYPT_R,x,128*SCRYPT_R);blockMix(x,y,z,SCRYPT_R);blockMix(y,x,z,SCRYPT_R);
#if !defined(AURORA_NATIVE_TEST)
    if((i&511u)==0) delay(1);
#endif
  }for(uint64_t i=0;i<SCRYPT_N;i+=2){uint64_t j=integerify(x,SCRYPT_R)&(SCRYPT_N-1);const uint32_t *state=v+(j/2)*32*SCRYPT_R;if(j&1u){blockMix(state,reconstructed,z,SCRYPT_R);state=reconstructed;}blockXor(x,state,128*SCRYPT_R);blockMix(x,y,z,SCRYPT_R);j=integerify(y,SCRYPT_R)&(SCRYPT_N-1);state=v+(j/2)*32*SCRYPT_R;if(j&1u){blockMix(state,reconstructed,z,SCRYPT_R);state=reconstructed;}blockXor(y,state,128*SCRYPT_R);blockMix(y,x,z,SCRYPT_R);
#if !defined(AURORA_NATIVE_TEST)
    if((i&511u)==0) delay(1);
#endif
  }for(size_t k=0;k<32*SCRYPT_R;++k)store32le(b+4*k,x[k]);}

enum class ScryptResult : uint8_t { Ok, MemoryFailed, CryptoFailed };

ScryptResult scryptAezeed(const uint8_t *password,size_t passwordLength,const uint8_t salt[5],uint8_t key[32]) {
  uint8_t *b=static_cast<uint8_t *>(malloc(1024));uint32_t *xy=static_cast<uint32_t *>(malloc(3136));
#if defined(AURORA_BOARD_P4) && !defined(AURORA_NATIVE_TEST)
  uint32_t *v=static_cast<uint32_t *>(heap_caps_malloc(SCRYPT_V_BYTES,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
#else
  uint32_t *v=static_cast<uint32_t *>(malloc(SCRYPT_V_BYTES));
#endif
  ScryptResult result=(b&&xy&&v)?ScryptResult::Ok:ScryptResult::MemoryFailed;
  if(result==ScryptResult::Ok&&auroraPbkdf2Hmac(MBEDTLS_MD_SHA256,password,passwordLength,salt,5,1,1024,b)!=0)result=ScryptResult::CryptoFailed;
  if(result==ScryptResult::Ok)smix(b,v,xy);
  if(result==ScryptResult::Ok&&auroraPbkdf2Hmac(MBEDTLS_MD_SHA256,password,passwordLength,b,1024,1,32,key)!=0)result=ScryptResult::CryptoFailed;
  if(v){secureZero(v,SCRYPT_V_BYTES);
#if defined(AURORA_BOARD_P4) && !defined(AURORA_NATIVE_TEST)
    heap_caps_free(v);
#else
    free(v);
#endif
  }
  if(xy){secureZero(xy,3136);free(xy);}if(b){secureZero(b,1024);free(b);}return result;
}
} // namespace

AezeedResult AezeedEngine::decode(const char *mnemonic,const char *passphrase,AezeedDecoded &out) {
  wipe(out);uint8_t encoded[AEZEED_BYTES]{},key[32]{},plain[CIPHERTEXT_BYTES]{};AezState state{};
  AezeedResult result=mnemonicBytes(mnemonic,encoded);if(result!=AezeedResult::Ok)goto cleanup;
  if(encoded[0]!=AEZEED_EXTERNAL_VERSION){result=AezeedResult::UnsupportedVersion;goto cleanup;}
  {const uint32_t expected=(static_cast<uint32_t>(encoded[29])<<24)|(static_cast<uint32_t>(encoded[30])<<16)|(static_cast<uint32_t>(encoded[31])<<8)|encoded[32];if(crc32c(encoded,AEZEED_CHECKSUM_OFFSET)!=expected){result=AezeedResult::InvalidChecksum;goto cleanup;}}
  {const char *effective=(passphrase&&passphrase[0])?passphrase:"aezeed";const size_t length=strlen(effective);if(length>63){result=AezeedResult::InvalidFormat;goto cleanup;}const ScryptResult scryptResult=scryptAezeed(reinterpret_cast<const uint8_t *>(effective),length,encoded+AEZEED_SALT_OFFSET,key);if(scryptResult!=ScryptResult::Ok){result=scryptResult==ScryptResult::MemoryFailed?AezeedResult::MemoryFailed:AezeedResult::CryptoFailed;goto cleanup;}}
  {uint8_t ad[6]={encoded[0],encoded[24],encoded[25],encoded[26],encoded[27],encoded[28]},delta[16];initAez(state,key);aezHash(state,ad,delta);aezTinyDecrypt(state,delta,encoded+1,plain);secureZero(delta,sizeof(delta));secureZero(ad,sizeof(ad));}
  {uint8_t invalid=plain[19]|plain[20]|plain[21]|plain[22];if(invalid){result=AezeedResult::InvalidPassphrase;goto cleanup;}}
  if(plain[0]!=AEZEED_INTERNAL_VERSION_LEGACY&&plain[0]!=AEZEED_INTERNAL_VERSION_TAPROOT){result=AezeedResult::UnsupportedVersion;goto cleanup;}
  out.internalVersion=plain[0];out.birthdayDays=static_cast<uint16_t>((plain[1]<<8)|plain[2]);memcpy(out.entropy,plain+3,16);result=AezeedResult::Ok;
cleanup:
  secureZero(encoded,sizeof(encoded));secureZero(key,sizeof(key));secureZero(plain,sizeof(plain));secureZero(&state,sizeof(state));if(result!=AezeedResult::Ok)wipe(out);return result;
}

void AezeedEngine::wipe(AezeedDecoded &out) { secureZero(&out,sizeof(out)); }
