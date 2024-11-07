#pragma once

#include <rocksdb/db.h>
#include <string>
#include <string_view>
#include <vector>

namespace vdb {

/************************************************************************/
/* KVStorage */
/************************************************************************/
class KVStorage {
 public:
  enum ErrorCode {
    EC_OK = 0,
    EC_NotFound = 1,
    EC_Undefined = 2,
  };

 private:
  rocksdb::DB* db_{nullptr};

 public:
  ~KVStorage();

 public:
  [[nodiscard]] bool Init(const std::string& path);

 public:
  [[nodiscard]] bool Put(std::string_view key, std::string_view value);
  [[nodiscard]] ErrorCode Get(std::string_view key, std::string* value) const;
};

}  // namespace vdb
