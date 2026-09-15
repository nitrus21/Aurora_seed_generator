#pragma once
#include <io.h>
#include <sys/stat.h>
#ifndef S_ISREG
#define S_ISREG(mode) (((mode) & _S_IFMT) == _S_IFREG)
#endif
int fsync(int descriptor);
