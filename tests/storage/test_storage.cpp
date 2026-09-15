#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "platform/storage.h"
#include "bsp/esp-bsp.h"
#include "esp_vfs_fat.h"
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#include "secure_memory.h"
#include "secure_lvgl_memory.h"

// Actual storage.cpp and CRT FILE buffering, simulated SDK mount/power, and
// tracked allocator hooks. Public disposable fixtures only, no wallet/device.
static std::map<void *,size_t> allocations;
static std::map<FILE *,void *> streams;
static bool failAllocation=false, failSetvbuf=false, failFdopen=false;
static bool failClose=false, failFlush=false, failSync=false;
static bool failPower=false, failMount=false, failUnmount=false;
static unsigned allocationCount=0, freeCount=0, powerCount=0, mountCount=0;
static sdmmc_card_t testCard;

extern "C" void *auroraUiAlloc(size_t length) {
  assert(length==512);
  if(std::exchange(failAllocation,false)) return nullptr;
  void *buffer=malloc(length); assert(buffer);
  memset(buffer,0xA5,length); allocations[buffer]=length; ++allocationCount;
  return buffer;
}
extern "C" void auroraUiFree(void *buffer) {
  if(!buffer) return;
  assert(allocations.count(buffer));
  for(auto [stream,payload]:streams) assert(payload!=buffer); // fclose happened first.
  for(size_t i=0;i<allocations.at(buffer);++i) assert(static_cast<uint8_t *>(buffer)[i]==0);
  allocations.erase(buffer); ++freeCount; free(buffer);
}
static int testSetvbuf(FILE *stream,char *buffer,int mode,size_t size) {
  assert(mode==_IOFBF && size==512 && allocations.count(buffer));
  for(size_t i=0;i<size;++i) assert(buffer[i]==0);
  if(std::exchange(failSetvbuf,false)) return -1;
  const int result=setvbuf(stream,buffer,mode,size);
  if(!result) streams[stream]=buffer;
  return result;
}
static FILE *testFdopen(int descriptor,const char *mode) {
  if(std::exchange(failFdopen,false)) return nullptr;
  return _fdopen(descriptor,mode);
}
static int testFclose(FILE *stream) {
  if(streams.count(stream)) assert(allocations.count(streams.at(stream)));
  const int result=fclose(stream);
  streams.erase(stream);
  return std::exchange(failClose,false)?EOF:result;
}
static int testFflush(FILE *stream) {
  return std::exchange(failFlush,false)?EOF:fflush(stream);
}
int fsync(int descriptor) { return std::exchange(failSync,false)?-1:_commit(descriptor); }
int sd_pwr_ctrl_new_on_chip_ldo(const sd_pwr_ctrl_ldo_config_t *config,sd_pwr_ctrl_handle_t *out) {
  assert(config->ldo_chan_id==4);
  if(failPower) return -1;
  ++powerCount; *out=&testCard; return ESP_OK;
}
int sd_pwr_ctrl_del_on_chip_ldo(sd_pwr_ctrl_handle_t power) {
  assert(power==&testCard && powerCount>0); --powerCount; return ESP_OK;
}
int esp_vfs_fat_sdmmc_mount(const char *path,const sdmmc_host_t *host,const sdmmc_slot_config_t *slot,
    const esp_vfs_fat_sdmmc_mount_config_t *config,sdmmc_card_t **card) {
  assert(!strcmp(path,BSP_SD_MOUNT_POINT) && host->pwr_ctrl_handle==&testCard);
  assert(slot->width==4 && !config->format_if_mount_failed && config->max_files==5);
  if(failMount) return -1;
  ++mountCount; *card=&testCard; return ESP_OK;
}
int esp_vfs_fat_sdcard_unmount(const char *,sdmmc_card_t *card) {
  assert(card==&testCard && mountCount>0);
  if(failUnmount) return -1;
  --mountCount; return ESP_OK;
}
#define setvbuf testSetvbuf
#define fdopen testFdopen
#define fclose testFclose
#define fflush testFflush
#include "../../targets/waveshare_p4/main/storage.cpp"
#undef setvbuf
#undef fdopen
#undef fclose
#undef fflush

static std::string bytes(const char *name) {
  FILE *file=fopen(name,"rb"); assert(file);
  std::string result; char buffer[256]; size_t read;
  while((read=fread(buffer,1,sizeof(buffer),file))!=0) result.append(buffer,read);
  assert(!ferror(file)); fclose(file); return result;
}
static void noBuffers() { assert(allocations.empty() && streams.empty() && allocationCount==freeCount); }
int main() {
  // POSIX on the target is binary by default; match this on the Windows CRT.
  assert(_set_fmode(_O_BINARY)==0);
  const std::filesystem::path card(BSP_SD_MOUNT_POINT);
  // Unique test output directory is created by run.py; refuse stale fixtures.
  assert(!std::filesystem::exists(card)); std::filesystem::create_directory(card);
  AuroraStorage storage;
  assert(!storage.open("/before-mount.json",true));
  failPower=true; assert(!storage.begin() && !powerCount); failPower=false;
  failMount=true; assert(!storage.begin() && !powerCount && !mountCount); failMount=false;
  assert(storage.begin() && storage.begin() && powerCount==1 && mountCount==1);
  for(auto path:{"../escape","/../escape","/folder\\escape","relative"}) assert(!storage.open(path,true));

  const std::string electrum="{\n  \"use_encryption\": false,\n  \"keystore\": {\"xprv\": \"test-only-public-fixture\"}\n}\n";
  {
    auto file=storage.open("/electrum.json",true); assert(file && allocations.size()==1);
    void *original=allocations.begin()->first;
    assert(file.write(reinterpret_cast<const uint8_t *>(electrum.data()),electrum.size())==electrum.size());
    AuroraFile moved(std::move(file)); assert(!file && moved && allocations.count(original));
    moved=std::move(moved); assert(moved && allocations.count(original));
    assert(storage.sync(moved));
    failClose=true; moved.close(); assert(!moved); // Even fclose error wipes owned buffer.
    moved.close(); noBuffers();
  }
  assert(bytes("storage-card/electrum.json")==electrum); // Newline/bytes unchanged.
  assert(!storage.open("/electrum.json",true)); // O_EXCL never truncates an export.
  assert(bytes("storage-card/electrum.json")==electrum);
  {
    auto read=storage.open("/electrum.json"); assert(read && read.size()==electrum.size());
    uint8_t data[512]{}; assert(read.read(data,sizeof(data))==electrum.size());
    assert(!memcmp(data,electrum.data(),electrum.size()));
    auto write=storage.open("/moved.json",true); assert(write);
    const auto before=freeCount; write=std::move(read);
    assert(!read && write && freeCount==before+1 && allocations.size()==1);
  }
  noBuffers();
  for(auto fault:{&failAllocation,&failSetvbuf,&failFdopen}) {
    *fault=true; assert(!storage.open("/failed.json",true));
    assert(!storage.exists("/failed.json")); noBuffers();
  }
  for(auto fault:{&failAllocation,&failSetvbuf}) {
    *fault=true; assert(!storage.open("/electrum.json")); noBuffers();
    assert(bytes("storage-card/electrum.json")==electrum);
  }
  assert(!storage.open("/absent.json")); noBuffers();
  {
    auto file=storage.open("/bulk.json",true); assert(file);
    std::string content(4097,'x'); content[510]='\n'; content[512]='\n';
    assert(file.write(reinterpret_cast<const uint8_t *>(content.data()),content.size())==content.size());
    failFlush=true; assert(!file.flush());
    failSync=true; assert(!file.flush());
    assert(file.flush()); file.close(); noBuffers();
    assert(bytes("storage-card/bulk.json")==content);
  }
  {
    std::filesystem::create_directory(card/"subdir");
    auto directory=storage.open("/"); assert(directory && directory.isDirectory());
    AuroraFile moved(std::move(directory)); assert(!directory && moved.isDirectory());
    unsigned files=0;
    while(auto entry=moved.openNextFile()) { assert(!entry.isDirectory()); ++files; }
    assert(files==3); moved.close(); assert(storageOpenDirectories==0); noBuffers();
    directory=storage.open("/"); failSetvbuf=true;
    while(auto entry=directory.openNextFile()) ++files;
    assert(files==5 && !failSetvbuf); // Failure closes that file then skips to the next.
  }
  assert(storageOpenDirectories==0); noBuffers();
  failUnmount=true; storage.end(); assert(powerCount==1 && mountCount==1);
  failUnmount=false; storage.end(); assert(!powerCount && !mountCount);
  storage.end(); noBuffers();
  puts("PASS: actual P4 storage.cpp owns/zeros/frees every 512-byte stdio buffer after fclose, including injected failures and moves");
  puts("PASS: binary payload/newline identity, exclusive export creation, reads, flush/fsync failures, directory ownership, mount/power cleanup");
  puts("LIMIT: Windows CRT and simulated SDK/allocator hooks, not ESP-IDF FATFS or physical SD/power-loss validation");
}
