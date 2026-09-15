#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>
#include "secure_memory.h"
using esp_partition_subtype_t=int;
constexpr int ESP_OK=0, ESP_PARTITION_TYPE_DATA=1, MALLOC_CAP_8BIT=1, MALLOC_CAP_INTERNAL=2;
struct esp_partition_t { uint32_t address=0x3f0000, size=0x10000; bool encrypted=false; } partition;
static bool missing=false, readError=false, eraseError=false, allocationError=false;
static unsigned erased=0, readCount=0;
static std::vector<uint8_t> flash(0x10000,0xff);
static std::map<void *,size_t> owned;
const esp_partition_t *esp_partition_find_first(int type,int sub,const char *name) {
  assert(type==1 && sub==0x40 && !strcmp(name,"aurora_scrub"));
  return missing?nullptr:&partition;
}
int esp_partition_read(const esp_partition_t *p,size_t offset,void *out,size_t n) {
  assert(p==&partition && offset+n<=flash.size()); ++readCount;
  if(readError) return -1;
  memcpy(out,flash.data()+offset,n); return ESP_OK;
}
int esp_partition_erase_range(const esp_partition_t *p,size_t offset,size_t n) {
  assert(p==&partition && offset==0 && n==0x10000); ++erased;
  if(eraseError) return -1;
  memset(flash.data(),0xff,flash.size()); return ESP_OK;
}
size_t heap_caps_get_largest_free_block(int caps) { assert(caps==3); return owned.empty()?1024:0; }
void *heap_caps_malloc(size_t size,int) {
  if(allocationError) return nullptr;
  void *p=malloc(size); assert(p); memset(p,0xa5,size); owned[p]=size; return p;
}
void heap_caps_free(void *p) {
  assert(owned.count(p));
  for(size_t n=0;n<owned[p];++n) assert(static_cast<uint8_t *>(p)[n]==0);
  owned.erase(p); free(p);
}
#include "boot_function.inc"
int main() {
  assert(auroraCydBootCleanup() && erased==0 && owned.empty());
  memset(flash.data(),0xa5,flash.size());
  assert(auroraCydBootCleanup() && erased==1 && owned.empty());
  assert(auroraCydBootCleanup() && erased==1); // No repeated erase wear on clean flash.
  const unsigned before=readCount;
  missing=true; assert(!auroraCydBootCleanup()); missing=false;
  partition.address=0; assert(!auroraCydBootCleanup()); partition.address=0x3f0000;
  partition.size=0x20000; assert(!auroraCydBootCleanup()); partition.size=0x10000;
  partition.encrypted=true; assert(!auroraCydBootCleanup()); partition.encrypted=false;
  assert(readCount==before && erased==1);
  readError=true; assert(!auroraCydBootCleanup()); readError=false;
  flash[0]=0; eraseError=true; assert(!auroraCydBootCleanup()); eraseError=false;
  assert(auroraCydBootCleanup());
  allocationError=true; assert(!auroraCydBootCleanup()); allocationError=false;
  assert(owned.empty());
  puts("PASS: actual boot cleanup, exact partition gate, dirty erase/readback, clean no-write boot, read/erase/allocation failure blocks use, owned heap wipe");
}
