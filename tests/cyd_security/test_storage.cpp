// Real adapter open/read/write/close path, against a faultable host VFS.
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cerrno>
#include <map>
#include <string>
#include <vector>
#include <utility>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "secure_memory.h"
struct Handle { std::string path; size_t offset; bool writing; };
static std::map<std::string,std::vector<uint8_t>> files;
static std::map<int,Handle> handles;
static int nextFd=0, closes=0, unlinks=0, interrupts=0;
static bool failStat=false, nonRegular=false, failSync=false, failClose=false;
static bool failRead=false, failWrite=false, zeroWrite=false;
static bool failAfterRead=false, failAfterWrite=false;
static size_t chunk=3;
static int vfsOpen(const char *path,int flags,int) {
  assert(std::string(path).find("/sd/")==0);
  const bool writing=(flags&O_WRONLY)!=0;
  if(writing) {
    assert((flags&(O_CREAT|O_EXCL))==(O_CREAT|O_EXCL));
    if(files.count(path)) {errno=EEXIST;return -1;}
    files[path]={};
  } else if(!files.count(path)) {errno=ENOENT;return -1;}
  int fd=nextFd++;handles[fd]={path,0,writing};return fd;
}
static int vfsStat(int fd,struct stat *st) {
  if(failStat){errno=EIO;return -1;}
  st->st_mode=nonRegular?_S_IFDIR:_S_IFREG;
  st->st_size=static_cast<decltype(st->st_size)>(files.at(handles.at(fd).path).size());return 0;
}
static int vfsRead(int fd,void *out,size_t len) {
  if(interrupts){--interrupts;errno=EINTR;return -1;}
  if(failRead){errno=EIO;return -1;}
  auto &h=handles.at(fd);assert(!h.writing);auto &data=files.at(h.path);
  const size_t n=(std::min)((std::min)(len,chunk),data.size()-h.offset);
  if(n)memcpy(out,data.data()+h.offset,n);h.offset+=n;
  if(failAfterRead)failRead=true;return static_cast<int>(n);
}
static int vfsWrite(int fd,const void *in,size_t len) {
  if(interrupts){--interrupts;errno=EINTR;return -1;}
  if(failWrite){errno=EIO;return -1;}
  if(zeroWrite)return 0;
  auto &h=handles.at(fd);assert(h.writing);auto &data=files.at(h.path);
  const size_t n=(std::min)(len,chunk);auto p=static_cast<const uint8_t*>(in);
  data.insert(data.end(),p,p+n);h.offset+=n;
  if(failAfterWrite)failWrite=true;return static_cast<int>(n);
}
static int vfsSync(int fd) {
  assert(handles.count(fd));
  if(interrupts){--interrupts;errno=EINTR;return -1;}
  if(failSync){errno=EIO;return -1;}return 0;
}
static int vfsClose(int fd){assert(handles.erase(fd)==1);++closes;errno=EIO;return failClose?-1:0;}
static int vfsUnlink(const char *p){++unlinks;assert(files.erase(p)==1);return 0;}
// Replaces OS calls AND identically named methods; no private-field injection.
#define open vfsOpen
#define read vfsRead
#define write vfsWrite
#define close vfsClose
#define fstat vfsStat
#define fsync vfsSync
#define unlink vfsUnlink
#include "platform/cyd_file.h"
#undef open
#undef read
#undef write
#undef close
#undef fstat
#undef fsync
#undef unlink
int main(){
  const uint8_t fixture[]="PUBLIC-TEST-ONLY";
  {
    auto f=AuroraFile::vfsOpen("/roundtrip.aurora",true);assert(f);
    interrupts=2;assert(f.vfsWrite(fixture,sizeof(fixture))==sizeof(fixture));
    assert(f.size()==sizeof(fixture));interrupts=1;assert(f.flush());assert(f.vfsClose());
    assert(!f && !*f.name());
  }
  const auto original=files.at("/sd/roundtrip.aurora");
  assert(!AuroraFile::vfsOpen("/roundtrip.aurora",true));assert(files.at("/sd/roundtrip.aurora")==original);
  assert(unlinks==0);assert(!AuroraFile::vfsOpen("/missing"));
  for(const char *p:{"bad","/../bad","/bad\\bad"})assert(!AuroraFile::vfsOpen(p,true));
  assert(!AuroraFile::vfsOpen(nullptr,true));
  {
    auto f=AuroraFile::vfsOpen("/roundtrip.aurora");uint8_t out[64]{};
    interrupts=1;assert(f.vfsRead(out,sizeof(out))==sizeof(fixture));
    assert(!memcmp(out,fixture,sizeof(fixture)));assert(f.vfsRead(out,1)==0);assert(f.vfsClose());
  }
  for(int fault=0;fault<4;++fault){
    auto f=AuroraFile::vfsOpen(("/fault"+std::to_string(fault)).c_str(),true);assert(f);
    failWrite=fault==0;zeroWrite=fault==1;failSync=fault==2;failClose=fault==3;
    assert(f.vfsWrite(fixture,sizeof(fixture))==((fault<2)?0:sizeof(fixture)));
    assert(f.flush()==(fault==3));
    failWrite=zeroWrite=failSync=false;
    assert(f.flush()==(fault==3));
    assert(!f.vfsClose());failClose=false;assert(!f.vfsClose());
  }
  {
    auto f=AuroraFile::vfsOpen("/roundtrip.aurora");uint8_t out[16]{};
    failRead=true;assert(!f.vfsRead(out,16));failRead=false;
    assert(!f.vfsRead(out,16));assert(!f.vfsClose());
  }
  for(int kind=0;kind<2;++kind){
    failStat=kind==0;nonRegular=kind==1;
    assert(!AuroraFile::vfsOpen("/setup",true));assert(!files.count("/sd/setup"));
    assert(!AuroraFile::vfsOpen("/roundtrip.aurora"));assert(files.at("/sd/roundtrip.aurora")==original);
    failStat=nonRegular=false;
  }
  {
    auto f=AuroraFile::vfsOpen("/partial",true);failAfterWrite=true;
    assert(f.vfsWrite(fixture,sizeof(fixture))==chunk);
    failAfterWrite=failWrite=false;assert(f.vfsWrite(fixture,1)==0);assert(!f.vfsClose());
    auto r=AuroraFile::vfsOpen("/roundtrip.aurora");uint8_t out[32]{};failAfterRead=true;
    assert(r.vfsRead(out,sizeof(out))==chunk);assert(!memcmp(out,fixture,chunk));
    failAfterRead=failRead=false;assert(r.vfsRead(out,1)==0);assert(!r.vfsClose());
  }
  {
    auto f=AuroraFile::vfsOpen("/roundtrip.aurora");failStat=true;
    assert(f.size()==0);failStat=false;assert(!f.vfsClose());
  }
  const int beforeDestructor=closes;
  {auto f=AuroraFile::vfsOpen("/roundtrip.aurora");assert(f);}
  assert(closes==beforeDestructor+1);
  failStat=failClose=true;
  assert(!AuroraFile::vfsOpen("/ambiguous",true));assert(files.count("/sd/ambiguous"));
  failStat=failClose=false;
  {
    auto a=AuroraFile::vfsOpen("/roundtrip.aurora");int before=closes;
    AuroraFile b(std::move(a));assert(!a && !*a.name() && b);
    auto c=AuroraFile::vfsOpen("/roundtrip.aurora");c=std::move(b);
    assert(closes==before+1 && !b && !*b.name());assert(c.vfsClose());
  }
  assert(handles.empty());assert(files.at("/sd/roundtrip.aurora")==original);
  puts("PASS: actual adapter open, fd zero, exclusive no-overwrite, read/write/EOF, short I/O, EINTR, latched errors, setup cleanup, ambiguous close, move/destructor ownership");
  puts("LIMIT: simulated VFS, not physical FAT or full UI");
}
