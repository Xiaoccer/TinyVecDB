#pragma once

#include <memory>
#include <unordered_map>
#include "gen_cpp/vdb.pb.h"
#include "index/index.h"

namespace vdb {

/************************************************************************/
/* IndexFactory */
/************************************************************************/
class IndexFactory {
 private:
  std::unordered_map<service::IndexType, std::unique_ptr<Index>> type_2_index_;

 public:
  void Add(service::IndexType type, std::unique_ptr<Index>&& index);
  Index* GetIndex(service::IndexType type) const;
  [[nodiscard]] bool SaveIndex(const std::string& path);
  [[nodiscard]] bool LoadIndex(const std::string& path);
};

}  // namespace vdb
