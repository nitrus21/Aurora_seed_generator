#include "platform/storage.h"
#include "bsp/esp-bsp.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <utility>
#include "esp_vfs_fat.h"
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#include "secure_memory.h"
#include "secure_lvgl_memory.h"

#ifdef CONFIG_BSP_SD_FORMAT_ON_MOUNT_FAIL
#error "AURORA must never automatically format the user's microSD card"
#endif

namespace {
constexpr size_t IO_BUFFER_SIZE = 512;
bool cardPath(const char *path, char (&out)[160]) {
  if (!path || path[0] != '/' || strstr(path, "..") || strchr(path, '\\')) return false;
  return snprintf(out, sizeof(out), "%s%s", BSP_SD_MOUNT_POINT, path) < static_cast<int>(sizeof(out));
}
}
AuroraFile::AuroraFile(AuroraFile &&other) noexcept { *this = std::move(other); }
AuroraFile &AuroraFile::operator=(AuroraFile &&other) noexcept {
  if (this != &other) {
    close();
    file_ = other.file_; directory_ = other.directory_;
    ioBuffer_ = other.ioBuffer_;
    memcpy(name_, other.name_, sizeof(name_));
    other.file_ = nullptr; other.directory_ = nullptr;
    other.ioBuffer_ = nullptr;
  }
  return *this;
}
bool AuroraFile::prepareBuffer() {
  if (!file_) return false;
  // Keep stdio's buffer under explicit ownership, including Electrum exports.
  // A heap allocation stays at the same address when AuroraFile is moved.
  // Share the registered secure allocator so terminal cleanup also covers
  // this buffer if a fault interrupts stdio before the normal close path.
  ioBuffer_ = static_cast<uint8_t *>(auroraUiAlloc(IO_BUFFER_SIZE));
  if (ioBuffer_) secureZero(ioBuffer_, IO_BUFFER_SIZE);
  if (!ioBuffer_ || setvbuf(file_, reinterpret_cast<char *>(ioBuffer_),
                           _IOFBF, IO_BUFFER_SIZE) != 0) {
    close();
    return false;
  }
  return true;
}
size_t AuroraFile::read(uint8_t *data, size_t length) { return file_ ? fread(data, 1, length, file_) : 0; }
size_t AuroraFile::write(const uint8_t *data, size_t length) { return file_ ? fwrite(data, 1, length, file_) : 0; }
size_t AuroraFile::size() {
  struct stat info{};
  return file_ && fstat(fileno(file_), &info) == 0 ? info.st_size : 0;
}
bool AuroraFile::flush() {
  return file_ && fflush(file_) == 0 && !ferror(file_) && fsync(fileno(file_)) == 0;
}
void AuroraFile::close() {
  if (file_) fclose(file_);
  if (ioBuffer_) {
    secureZero(ioBuffer_, IO_BUFFER_SIZE);
    auroraUiFree(ioBuffer_);
    ioBuffer_ = nullptr;
  }
  if (directory_) closedir(directory_);
  file_ = nullptr; directory_ = nullptr;
}
AuroraFile AuroraFile::openNextFile() {
  AuroraFile next;
  if (!directory_) return next;
  while (auto entry = readdir(directory_)) {
    if (entry->d_name[0] == '.') continue;
    char full[256];
    const int length = snprintf(full, sizeof(full), "%s/%s", name_, entry->d_name);
    if (length <= 0 || length >= static_cast<int>(sizeof(full))) continue;
    struct stat info{};
    if (stat(full, &info) != 0 || !S_ISREG(info.st_mode)) continue;
    next.file_ = fopen(full, "rb");
    if (!next.prepareBuffer()) continue;
    strlcpy(next.name_, entry->d_name, sizeof(next.name_));
    return next;
  }
  return next;
}
bool AuroraStorage::begin() {
  if (mounted_) return true;
  // Own the LDO handle too: repeated mount/unmount must not leak BSP resources.
  sd_pwr_ctrl_ldo_config_t ldo{}; ldo.ldo_chan_id = 4;
  if (sd_pwr_ctrl_new_on_chip_ldo(&ldo, &power_) != ESP_OK) return false;
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  host.slot = SDMMC_HOST_SLOT_0; host.pwr_ctrl_handle = power_;
  sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
  slot.width = 4;
  esp_vfs_fat_sdmmc_mount_config_t config{};
  config.format_if_mount_failed = false; config.max_files = 5;
  config.allocation_unit_size = 64 * 1024;
  mounted_ = esp_vfs_fat_sdmmc_mount(BSP_SD_MOUNT_POINT, &host, &slot, &config, &card_) == ESP_OK;
  if (!mounted_) { sd_pwr_ctrl_del_on_chip_ldo(power_); power_ = nullptr; card_ = nullptr; }
  return mounted_;
}
void AuroraStorage::end() {
  if (mounted_) {
    if (esp_vfs_fat_sdcard_unmount(BSP_SD_MOUNT_POINT, card_) != ESP_OK) return;
    mounted_ = false; card_ = nullptr;
  }
  if (power_) { sd_pwr_ctrl_del_on_chip_ldo(power_); power_ = nullptr; }
}
bool AuroraStorage::exists(const char *path) {
  char full[160]; struct stat info{};
  return mounted_ && cardPath(path, full) && stat(full, &info) == 0;
}
bool AuroraStorage::remove(const char *path) {
  char full[160];
  return mounted_ && cardPath(path, full) && unlink(full) == 0;
}
AuroraFile AuroraStorage::open(const char *path, bool write) {
  AuroraFile result; char full[160];
  if (!mounted_ || !cardPath(path, full)) return result;
  strlcpy(result.name_, full, sizeof(result.name_));
  if (strcmp(path, "/") == 0 && !write) result.directory_ = opendir(full);
  else if (write) {
    const int fd = ::open(full, O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (fd >= 0) {
      result.file_ = fdopen(fd, "wb");
      if (!result.file_) { ::close(fd); unlink(full); }
      else if (!result.prepareBuffer()) unlink(full);
    }
  } else {
    result.file_ = fopen(full, "rb");
    result.prepareBuffer();
  }
  return result;
}
