#pragma once
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
inline size_t strlcpy(char *out,const char *in,size_t size) {
  const size_t length=strlen(in);
  if(size) { const size_t copied=length<size-1?length:size-1; memcpy(out,in,copied); out[copied]=0; }
  return length;
}
