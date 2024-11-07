#pragma once

#include <stdint.h>
#include "bitmap/field_bitmap.h"
#include "gen_cpp/vdb.pb.h"
#include "index/index.h"
#include "index/index_factory.h"
#include "persistence/persistence.h"

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

 private:
  IndexFactory index_factory_;
  FieldBitmap field_bitmap_;
  Persistence persistence_;

 public:
  [[nodiscard]] bool Init(const InitOptions& opts);

 public:
  [[nodiscard]] bool Upsert(const UpsertOptions& opts);
  [[nodiscard]] bool Search(const SearchOptions& opts, Index::SearchRes* res);
  [[nodiscard]] bool Query(int64_t id, std::string* data);

 public:
  [[nodiscard]] bool Reload();
  [[nodiscard]] bool WriteWALLog(Persistence::WAL_TYPE wt, const std::string& data);
  [[nodiscard]] bool SaveSnapshot();
  [[nodiscard]] bool LoadSnapshot();
};

}  // namespace vdb
