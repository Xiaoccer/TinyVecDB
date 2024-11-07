#pragma once

#include <memory>
#include <string>
#include <string_view>

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
  class Impl;
  std::unique_ptr<Impl> impl_;

 public:
  KVStorage();
  ~KVStorage();

 public:
  KVStorage(const KVStorage&) = delete;
  KVStorage(KVStorage&&) = delete;
  KVStorage& operator=(const KVStorage&) = delete;
  KVStorage& operator=(KVStorage&&) = delete;

 public:
  [[nodiscard]] bool Init(const std::string& path);

 public:
  [[nodiscard]] bool Put(std::string_view key, std::string_view value);
  [[nodiscard]] ErrorCode Get(std::string_view key, std::string* value) const;
};

}  // namespace vdb
