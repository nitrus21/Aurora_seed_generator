#pragma once
#include <filesystem>
#include <cstring>
#include <new>
struct dirent { char d_name[260]{}; };
struct DIR {
  std::filesystem::directory_iterator current,end;
  dirent entry;
};
inline unsigned storageOpenDirectories=0;
inline DIR *opendir(const char *path) {
  std::error_code error;
  std::filesystem::directory_iterator current(path,error);
  if(error) return nullptr;
  auto *result=new(std::nothrow) DIR;
  if(!result) return nullptr;
  result->current=current; ++storageOpenDirectories; return result;
}
inline dirent *readdir(DIR *directory) {
  if(directory->current==directory->end) return nullptr;
  const auto name=directory->current->path().filename().string();
  strncpy(directory->entry.d_name,name.c_str(),sizeof(directory->entry.d_name)-1);
  directory->entry.d_name[sizeof(directory->entry.d_name)-1]=0;
  ++directory->current; return &directory->entry;
}
inline int closedir(DIR *directory) { delete directory; --storageOpenDirectories; return 0; }
