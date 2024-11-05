#include "index_factory.h"
#include <gen_cpp/vdb.pb.h>
#include <glog/logging.h>
#include <string>
#include "index.h"

namespace vdb {

namespace {

std::string BuildSavePath(const std::string& path, service::IndexType type) {
  return path + "/" + std::to_string(type) + ".index";
}

}  // namespace

/************************************************************************/
/* IndexFactory */
/************************************************************************/
void IndexFactory::Add(service::IndexType type, std::unique_ptr<Index>&& index) {
  type_2_index_[type] = std::move(index);
}

Index* IndexFactory::GetIndex(service::IndexType type) const {
  auto it = type_2_index_.find(type);
  if (it == type_2_index_.end()) {
    return nullptr;
  }
  return it->second.get();
}

bool IndexFactory::SaveIndex(const std::string& path) {
  for (const auto& [type, index] : type_2_index_) {
    std::string file_path = BuildSavePath(path, type);
    if (!index->Save(file_path)) {
      LOG(WARNING) << "Failed to save index, type=" << type << ",path=" << file_path << ".";
      return false;
    }
  }
  return true;
}

bool IndexFactory::LoadIndex(const std::string& path) {
  for (const auto& [type, index] : type_2_index_) {
    std::string file_path = BuildSavePath(path, type);
    if (!index->Load(file_path)) {
      LOG(WARNING) << "Failed to load index, type=" << type << ",path=" << file_path << ".";
      return false;
    }
  }
  return true;
}

/************************************************************************/
/* IndexFactory functions */
/************************************************************************/
IndexFactory& GlobalIndexFactory() {
  static IndexFactory f;
  return f;
}

}  // namespace vdb
