#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "secure_memory.h"
static bool failFlush=false, failSync=false, failClose=false;
static unsigned freed=0;
static int checkedFlush(FILE *f) { return failFlush ? EOF : fflush(f); }
static int checkedClose(FILE *f) { int r=fclose(f); return failClose ? EOF : r; }
int fsync(int fd) { return failSync ? -1 : _commit(fd); }
static void checkedFree(void *p) {
  for(unsigned n=0;n<512;++n) assert(static_cast<uint8_t *>(p)[n]==0);
  ++freed; free(p);
}
#define private public
#define fflush checkedFlush
#define fclose checkedClose
#define free checkedFree
#include "platform/cyd_file.h"
#undef private
#undef fflush
#undef fclose
#undef free
int main() {
  for(unsigned fault=0;fault<4;++fault) {
    AuroraFile f;
    f.file_=tmpfile(); assert(f.file_);
    f.buffer_=static_cast<uint8_t *>(malloc(512)); assert(f.buffer_);
    memset(f.buffer_,0xA5,512);
    assert(setvbuf(f.file_,reinterpret_cast<char *>(f.buffer_),_IOFBF,512)==0);
    f.writing_=true;
    const uint8_t fixture[]="PUBLIC-TEST-SECRET";
    assert(f.write(fixture,sizeof(fixture))==sizeof(fixture));
    failFlush=fault==1; failSync=fault==2; failClose=fault==3;
    assert(f.flush()==(fault!=1 && fault!=2));
    // Errors latch: a later successful sync cannot disguise an earlier failure.
    failFlush=failSync=false;
    assert(f.flush()==(fault!=1 && fault!=2));
    assert(f.close()==(fault==0));
    assert(!f && !f.buffer_); failClose=false;
  }
  assert(freed==4);
  puts("PASS: actual CYD flush/close, fflush/fsync/fclose errors, latched failures, buffer wiped before free");
}
