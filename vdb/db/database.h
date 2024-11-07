#pragma once

#include <gen_cpp/vdb.pb.h>
#include <google/protobuf/map.h>
#include <stddef.h>
#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

namespace vdb {

/************************************************************************/
/* Database */
/************************************************************************/
class Database {
 public:
  struct InitOptions {
    std::string persistence_path;
    int dim = 1;
    int num_data = 1000;
  };

 public:
  struct UpsertOptions {
    int64_t id{-1};
    service::IndexType index_type{service::IndexType::IT_INVALID};
    const float* data{nullptr};
    std::string scalar_data;
    // TODO(cong): 不够灵活
    const ::google::protobuf::Map<std::string, ::google::protobuf::int64>* field{nullptr};
  };

  struct SearchOptions {
    service::IndexType index_type{service::IndexType::IT_INVALID};
    const float* query{nullptr};
    size_t size{0};
    int k{0};
    std::string filter_field;
    std::string filter_op;
    int64_t filter_value{0};
  };

  struct SearchResult {
    std::vector<int64_t> indices;
    std::vector<float> distances;
  };

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

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;

 public:
  Database();
  ~Database();

 public:
  Database(const Database&) = delete;
  Database(Database&&) = delete;
  Database& operator=(const Database&) = delete;
  Database& operator=(Database&&) = delete;

 public:
  [[nodiscard]] bool Init(const InitOptions& opts);

 public:
  [[nodiscard]] bool Upsert(const UpsertOptions& opts);
  [[nodiscard]] bool Search(const SearchOptions& opts, SearchResult* res);
  [[nodiscard]] bool Query(int64_t id, std::string* data);

 public:
  [[nodiscard]] bool Reload();
  [[nodiscard]] bool WriteWALLog(WAL_TYPE wt, const std::string& data);
  [[nodiscard]] bool SaveSnapshot();
  [[nodiscard]] bool LoadSnapshot();
};

}  // namespace vdb
