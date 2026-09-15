#pragma once
#include "platform/runtime.h"

#if defined(AURORA_BOARD_P4)
#include <dirent.h>
#include "sdmmc_cmd.h"
#include "sd_pwr_ctrl.h"

class AuroraFile {
 public:
  AuroraFile() = default;
  ~AuroraFile() { close(); }
  AuroraFile(const AuroraFile &) = delete;
  AuroraFile &operator=(const AuroraFile &) = delete;
  AuroraFile(AuroraFile &&other) noexcept;
  AuroraFile &operator=(AuroraFile &&other) noexcept;
  explicit operator bool() const { return file_ || directory_; }
  size_t read(uint8_t *data, size_t length);
  size_t write(const uint8_t *data, size_t length);
  size_t size();
  bool flush();
  void close();
  bool isDirectory() const { return directory_ != nullptr; }
  const char *name() const { return name_; }
  AuroraFile openNextFile();
 private:
  friend class AuroraStorage;
  bool prepareBuffer();
  FILE *file_ = nullptr;
  uint8_t *ioBuffer_ = nullptr;
  DIR *directory_ = nullptr;
  char name_[128]{};
};

class AuroraStorage {
 public:
  ~AuroraStorage() { end(); }
  bool begin();
  void end();
  bool exists(const char *path);
  bool remove(const char *path);
  bool sync(AuroraFile &file) { return file.flush(); }
  AuroraFile open(const char *path, bool write = false);
 private:
  bool mounted_ = false;
  sdmmc_card_t *card_ = nullptr;
  sd_pwr_ctrl_handle_t power_ = nullptr;
};
#else
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include "board_config.h"
using AuroraFile = File;
class AuroraStorage {
 public:
  AuroraStorage() : spi_(HSPI) {}
  bool begin() {
    pinMode(AURORA_SD_CS, OUTPUT);
    digitalWrite(AURORA_SD_CS, HIGH);
    spi_.begin(AURORA_SD_CLK, AURORA_SD_MISO, AURORA_SD_MOSI, AURORA_SD_CS);
    return SD.begin(AURORA_SD_CS, spi_, AURORA_SD_FREQUENCY) && SD.cardType() != CARD_NONE;
  }
  void end() { SD.end(); spi_.end(); }
  bool exists(const char *path) { return SD.exists(path); }
  bool remove(const char *path) { return SD.remove(path); }
  bool sync(AuroraFile &file) { file.flush(); return file.getWriteError() == 0; }
  AuroraFile open(const char *path, bool write = false) { return SD.open(path, write ? FILE_WRITE : FILE_READ); }
 private:
  SPIClass spi_;
};
#endif
