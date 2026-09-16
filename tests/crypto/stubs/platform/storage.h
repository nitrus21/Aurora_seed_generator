#pragma once
#include <map>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>

// Test-only in-memory SD. No disk, card, wallet or device is touched.
inline std::map<std::string, std::vector<uint8_t>> testCard;
inline bool testSyncOk = true;
inline bool testCloseOk = true, testEndOk = true;
inline bool testCardReady = true, testRootReadable = true;
inline unsigned testMounts = 0, testUnmounts = 0;
inline bool testReadOpenOk = true;
inline size_t testReadLimit = static_cast<size_t>(-1);
inline void (*testBeforeReadOpen)(const char *) = nullptr;
inline bool testAppendAfterHeader = false;
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
    length = std::min({length, bytes.size() - position, testReadLimit});
    memcpy(out, bytes.data() + position, length); position += length;
    if (testAppendAfterHeader) { bytes.push_back(0); testAppendAfterHeader = false; }
    return length;
  }
  size_t write(const uint8_t *data, size_t length) {
    if (!valid || directory) return 0;
    auto &bytes = testCard.at(path); bytes.insert(bytes.end(), data, data + length); return length;
  }
  bool close() { valid = false; return testCloseOk; }
  AuroraFile openNextFile() {
    if (!valid || !directory || entry >= testCard.size()) return {};
    auto it = testCard.begin(); std::advance(it, entry++);
    return {it->first, 0, 0, true, false};
  }
};
class AuroraStorage {
 public:
  bool begin() { ++testMounts; return testCardReady; }
  bool end() { ++testUnmounts; return testEndOk; }
  bool exists(const char *path) { return testCard.count(path) != 0; }
  bool remove(const char *path) { return testCard.erase(path) != 0; }
  bool sync(AuroraFile &) { return testSyncOk; }
  AuroraFile open(const char *path, bool write = false) {
    if (!strcmp(path, "/")) return {path, 0, 0, testRootReadable, true};
    if (write) {
      if (exists(path)) return {};
      testCard[path] = {};
    } else {
      if (testBeforeReadOpen) testBeforeReadOpen(path);
      if (!testReadOpenOk) return {};
    }
    return {path, 0, 0, exists(path), false};
  }
};
