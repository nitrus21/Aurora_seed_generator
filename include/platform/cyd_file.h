#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "secure_memory.h"

// Arduino File::flush() drops fflush/fsync errors. Own the VFS handle and its
// buffer instead. No overwrite and no hidden plaintext stdio buffer on close.
class AuroraFile {
 public:
  AuroraFile() = default;
  ~AuroraFile() { close(); }
  AuroraFile(const AuroraFile &) = delete;
  AuroraFile &operator=(const AuroraFile &) = delete;
  AuroraFile(AuroraFile &&other) noexcept { take(other); }
  AuroraFile &operator=(AuroraFile &&other) noexcept {
    if (this != &other) { close(); take(other); } return *this;
  }
  explicit operator bool() const { return file_ || dir_; }
  bool isDirectory() const { return dir_ != nullptr; }
  const char *name() const { return name_; }
  size_t read(uint8_t *out, size_t length) {
    if (!file_) return 0;
    const size_t n = fread(out, 1, length, file_);
    failed_ |= ferror(file_) != 0;
    return n;
  }
  size_t write(const uint8_t *data, size_t length) {
    if (!file_ || !writing_ || failed_) return 0;
    const size_t n = fwrite(data, 1, length, file_);
    failed_ |= n != length || ferror(file_) != 0;
    return n;
  }
  size_t size() {
    struct stat st{};
    if (!file_ || fstat(fileno(file_), &st) != 0 || st.st_size < 0) { failed_ = true; return 0; }
    return static_cast<size_t>(st.st_size);
  }
  bool flush() {
    if (!file_) return false;
    const bool flushed = fflush(file_) == 0;
    const bool synced = fsync(fileno(file_)) == 0;
    failed_ |= !flushed || !synced || ferror(file_) != 0;
    return !failed_;
  }
  bool close() {
    if (file_) {
      // Flush before zeroing: wiping pending bytes would corrupt the backup.
      if (writing_) flush();
      if (fclose(file_) != 0) failed_ = true;
      file_ = nullptr;
    }
    if (buffer_) { secureZero(buffer_, BUFFER_SIZE); free(buffer_); buffer_ = nullptr; }
    if (dir_) { if (closedir(dir_) != 0) failed_ = true; dir_ = nullptr; }
    secureZero(name_, sizeof(name_));
    return !failed_;
  }
  static AuroraFile open(const char *path, bool write = false) {
    AuroraFile result;
    char full[160]{};
    if (!path || path[0] != '/' || strstr(path, "..") || strchr(path, '\\') ||
        snprintf(full, sizeof(full), "/sd%s", path) >= static_cast<int>(sizeof(full))) return result;
    if (strcmp(path, "/") == 0 && !write) result.dir_ = opendir(full);
    else {
      const int fd = ::open(full, write ? (O_WRONLY | O_CREAT | O_EXCL) : O_RDONLY, 0600);
      if (fd >= 0) {
        struct stat st{};
        if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) {
          ::close(fd);
          if (write) unlink(full); // Only the file just created with O_EXCL.
          secureZero(full, sizeof(full));
          return result;
        }
        result.file_ = fdopen(fd, write ? "wb" : "rb");
        if (!result.file_) ::close(fd);
        else {
          result.buffer_ = static_cast<uint8_t *>(malloc(BUFFER_SIZE));
          if (result.buffer_) secureZero(result.buffer_, BUFFER_SIZE);
          if (!result.buffer_ || setvbuf(result.file_, reinterpret_cast<char *>(result.buffer_), _IOFBF, BUFFER_SIZE) != 0) {
            result.failed_ = true; result.close();
          }
        }
        // Only this call created the file with O_EXCL. A failed setup may
        // remove its empty artifact, never an existing or read-only backup.
        if (!result.file_ && write) unlink(full);
      }
    }
    result.writing_ = write;
    if (result) snprintf(result.name_, sizeof(result.name_), "%s", path);
    secureZero(full, sizeof(full));
    return result;
  }
  AuroraFile openNextFile() {
    if (!dir_) return AuroraFile();
    while (dirent *entry = readdir(dir_)) {
      if (entry->d_name[0] == '.') continue;
      char path[128]{};
      if (snprintf(path, sizeof(path), "/%s", entry->d_name) >= static_cast<int>(sizeof(path))) continue;
      AuroraFile next = open(path);
      secureZero(path, sizeof(path));
      if (next) return next;
    }
    return AuroraFile();
  }
 private:
  void take(AuroraFile &other) {
    file_ = other.file_; dir_ = other.dir_; buffer_ = other.buffer_;
    failed_ = other.failed_; writing_ = other.writing_;
    memcpy(name_, other.name_, sizeof(name_));
    other.file_ = nullptr; other.dir_ = nullptr; other.buffer_ = nullptr;
    secureZero(other.name_, sizeof(other.name_));
  }
  static constexpr size_t BUFFER_SIZE = 512;
  FILE *file_ = nullptr;
  DIR *dir_ = nullptr;
  uint8_t *buffer_ = nullptr;
  bool failed_ = false;
  bool writing_ = false;
  char name_[128]{};
};
