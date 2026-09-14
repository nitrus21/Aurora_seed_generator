#pragma once
#include <map>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>

// Test-only in-memory SD. No disk, card, wallet or device is touched.
inline std::map<std::string, std::vector<uint8_t>> testCard;
inline bool testSyncOk = true;
inline bool testCardReady = true, testRootReadable = true;
inline unsigned testMounts = 0, testUnmounts = 0;
class AuroraFile {
 public:
  std::string path;
  size_t position = 0, entry = 0;
  bool valid = false, directory = false;
  explicit operator bool() const { return valid; }
  bool isDirectory() const { return directory; }
  const char *name() const { return path.c_str(); }
  size_t size() { return valid && !directory ? testCard.at(path).size() : 0; }
  size_t read(uint8_t *out, size_t length) {
    if (!valid || directory) return 0;
    auto &bytes = testCard.at(path);
    length = std::min(length, bytes.size() - position);
    memcpy(out, bytes.data() + position, length); position += length; return length;
  }
  size_t write(const uint8_t *data, size_t length) {
    if (!valid || directory) return 0;
    auto &bytes = testCard.at(path); bytes.insert(bytes.end(), data, data + length); return length;
  }
  void close() { valid = false; }
  AuroraFile openNextFile() {
    if (!valid || !directory || entry >= testCard.size()) return {};
    auto it = testCard.begin(); std::advance(it, entry++);
    return {it->first, 0, 0, true, false};
  }
};
class AuroraStorage {
 public:
  bool begin() { ++testMounts; return testCardReady; }
  void end() { ++testUnmounts; }
  bool exists(const char *path) { return testCard.count(path) != 0; }
  bool remove(const char *path) { return testCard.erase(path) != 0; }
  bool sync(AuroraFile &) { return testSyncOk; }
  AuroraFile open(const char *path, bool write = false) {
    if (!strcmp(path, "/")) return {path, 0, 0, testRootReadable, true};
    if (write) {
      if (exists(path)) return {};
      testCard[path] = {};
    }
    return {path, 0, 0, exists(path), false};
  }
};
