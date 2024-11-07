#pragma once

#include <stdint.h>
#include <memory>
#include <string>
#include <string_view>
#include "bitmap/field_bitmap.h"
#include "index/index_factory.h"
#include "persistence/kv_storage.h"
#if defined __GLIBCXX__ && __GNUC__ <= 7
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

namespace vdb {

/************************************************************************/
/* Persistence */
/************************************************************************/
class Persistence {
 public:
  enum LOG_STATUS {
    LS_OK = 0,
    LS_END = 1,
    LS_ERROR = 2,
  };

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;

 public:
  Persistence();
  ~Persistence();

 public:
  Persistence(const Persistence&) = delete;
  Persistence(Persistence&&) = delete;
  Persistence& operator=(const Persistence&) = delete;
  Persistence& operator=(Persistence&&) = delete;

 public:
  [[nodiscard]] bool Init(const std::string& path, uint8_t version);

 public:
  [[nodiscard]] bool WriteWALLog(char op, const std::string& data);
  [[nodiscard]] LOG_STATUS ReadNextWALLog(char* op, std::string* data);

 public:
  [[nodiscard]] bool Put(std::string_view key, std::string_view value);
  [[nodiscard]] KVStorage::ErrorCode Get(std::string_view key, std::string* value) const;

 public:
  [[nodiscard]] bool SaveSnapshot(IndexFactory* index_factory, FieldBitmap* bitmap);
  [[nodiscard]] bool LoadSnapshot(IndexFactory* index_factory, FieldBitmap* bitmap);
};

}  // namespace vdb
