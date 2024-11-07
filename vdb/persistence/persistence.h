#pragma once

#include <cstdint>
#include <fstream>
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
  enum WAL_TYPE : char {
    WT_NONE = 0,
    WT_UPSERT = 1,
    WT_MAX = 2,
  };

  inline static const std::string WT_2_STRING[WT_MAX] = {
      "NONE",    //
      "UPSERT",  // WT_UPSERT
  };

  enum WAL_STATUS {
    WS_OK = 0,
    WS_END = 1,
    WS_ERROR = 2,
  };

 private:
  uint64_t log_id_{1};
  uint64_t last_snapshot_id_{0};
  uint8_t version_;

  fs::path wal_path_;
  fs::path kv_storage_path_;
  fs::path snapshot_path_;
  std::fstream wal_log_file_;

  KVStorage kv_storage_;

 public:
  Persistence() = default;
  ~Persistence();

 public:
  [[nodiscard]] bool Init(const std::string& path, uint8_t version);

 public:
  [[nodiscard]] bool WriteWALLog(WAL_TYPE wt, const std::string& data);
  [[nodiscard]] WAL_STATUS ReadNextWALLog(WAL_TYPE* wt, std::string* data);

 public:
  [[nodiscard]] bool Put(std::string_view key, std::string_view value);
  [[nodiscard]] KVStorage::ErrorCode Get(std::string_view key, std::string* value) const;

 public:
  [[nodiscard]] bool SaveSnapshot(IndexFactory* index_factory, FieldBitmap* bitmap);
  [[nodiscard]] bool LoadSnapshot(IndexFactory* index_factory, FieldBitmap* bitmap);
};

}  // namespace vdb
