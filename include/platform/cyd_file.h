#pragma once
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "secure_memory.h"

// Own the FAT VFS descriptor: Arduino File::flush() loses sync errors, while
// IDF 4.4 fdopen() requires an unsupported FAT fcntl(F_GETFL). Direct I/O also
// avoids an application-owned plaintext stdio buffer. SDK buffers are separate.
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
  explicit operator bool() const { return fd_ >= 0 || dir_; }
  bool isDirectory() const { return dir_ != nullptr; }
  const char *name() const { return name_; }
  size_t read(uint8_t *out, size_t length) {
    if (fd_ < 0 || writing_ || failed_) return 0;
    size_t done = 0;
    while (done < length) {
      const auto n = ::read(fd_, out + done, length - done);
      if (n < 0) { if (errno == EINTR) continue; failed_ = true; break; }
      if (n == 0) break; // EOF is not an error.
      done += static_cast<size_t>(n);
    }
    return done;
  }
  size_t write(const uint8_t *data, size_t length) {
    if (fd_ < 0 || !writing_ || failed_) return 0;
    size_t done = 0;
    while (done < length) {
      const auto n = ::write(fd_, data + done, length - done);
      if (n < 0 && errno == EINTR) continue;
      if (n <= 0) { failed_ = true; break; }
      done += static_cast<size_t>(n);
    }
    return done;
  }
  size_t size() {
    struct stat st{};
    if (fd_ < 0 || fstat(fd_, &st) != 0 || st.st_size < 0) { failed_ = true; return 0; }
    return static_cast<size_t>(st.st_size);
  }
  bool flush() {
    if (fd_ < 0) return false;
    int status;
    do { status = fsync(fd_); } while (status != 0 && errno == EINTR);
    failed_ |= status != 0;
    return !failed_;
  }
  bool close() {
    if (fd_ >= 0) {
      if (writing_) flush();
      // Never retry close: after an error descriptor ownership is ambiguous.
      if (::close(fd_) != 0) failed_ = true;
      fd_ = -1;
    }
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
          const bool closed = ::close(fd) == 0;
          if (write && closed) unlink(full); // Only our own empty O_EXCL artifact.
          secureZero(full, sizeof(full));
          return result;
        }
        result.fd_ = fd;
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
    fd_ = other.fd_; dir_ = other.dir_;
    failed_ = other.failed_; writing_ = other.writing_;
    memcpy(name_, other.name_, sizeof(name_));
    other.fd_ = -1; other.dir_ = nullptr;
    other.failed_ = false; other.writing_ = false;
    secureZero(other.name_, sizeof(other.name_));
  }
  int fd_ = -1;
  DIR *dir_ = nullptr;
  bool failed_ = false;
  bool writing_ = false;
  char name_[128]{};
};
