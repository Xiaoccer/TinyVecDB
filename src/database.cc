#include "database.h"
#include <gen_cpp/vdb.pb.h>
#include <glog/logging.h>
#include <string>
#include "field_bitmap.h"
#include "kv_storage.h"
#include "persistence.h"
#include "util.h"

namespace vdb {

const uint8_t VERSION = 1;

/************************************************************************/
/* Database */
/************************************************************************/
bool Database::Init(const InitOptions& opts) {
  if (!persistence_.Init(opts.persistence_path, VERSION)) {
    return false;
  }

  index_factory_.Add(vdb::service::IndexType::IT_FLAT, vdb::NewFaissIndex(opts.dim, vdb::MetricType::L2));
  index_factory_.Add(vdb::service::IndexType::IT_HNSW,
                     vdb::NewHNSWLibIndex(opts.dim, opts.num_data, vdb::MetricType::L2));
  return true;
}

bool Database::Upsert(const UpsertOptions& opts) {
  auto index = index_factory_.GetIndex(opts.index_type);
  if (!index) {
    LOG(WARNING) << "Failed to get index type=" << opts.index_type << ".";
    return false;
  }

  std::string scalar_value;
  auto ec = persistence_.Get(std::to_string(opts.id), &scalar_value);
  if (ec == KVStorage::EC_Undefined) {
    LOG(WARNING) << "Failed to get scalar value from storage, id=" << opts.id << ".";
    return false;
  }
  if (ec == KVStorage::EC_OK) {
    // 先删除
    index->Remove({opts.id});
  }

  if (opts.field && !opts.field->empty()) {
    if (ec == KVStorage::EC_NotFound) {
      for (const auto& [field_name, value] : *opts.field) {
        field_bitmap_.UpdateFiledValue(opts.id, field_name, value);
      }
    } else {
      // TODO(cong): 需要反序列化，不是很优雅
      service::UpsertRequest request;
      if (!request.ParseFromString(scalar_value)) {
        LOG(WARNING) << "Failed to parse scalar data.";
        return false;
      }
      for (const auto& [field_name, value] : *opts.field) {
        auto it = request.fields().find(field_name);
        if (it == request.fields().end()) {
          field_bitmap_.UpdateFiledValue(opts.id, field_name, value);
        } else {
          field_bitmap_.UpdateFiledValue(opts.id, field_name, value, it->second);
        }
      }
    }
  }

  if (!persistence_.Put(std::to_string(opts.id), opts.scalar_data)) {
    return false;
  }

  Index::InsertOptions insert_opts;
  insert_opts.label = opts.id;
  insert_opts.data = opts.data;
  index->Insert(insert_opts);
  return true;
}

bool Database::Search(const SearchOptions& opts, Index::SearchRes* res) {
  auto index = index_factory_.GetIndex(opts.index_type);
  if (!index) {
    LOG(WARNING) << "Failed to get index type=" << opts.index_type << ".";
    return false;
  }

  Index::SearchOptions search_opts;
  search_opts.query = opts.query;
  search_opts.size = opts.size;
  search_opts.k = opts.k;
  roaring_bitmap_ptr ptr;
  if (!opts.filter_op.empty()) {
    FieldBitmap::Operation op =
        (opts.filter_op == "=") ? FieldBitmap::Operation::EQUAL : FieldBitmap::Operation::NOT_EQUAL;
    ptr = field_bitmap_.GetBitmap(opts.filter_field, opts.filter_value, op);
    search_opts.bitmap = ptr.get();
  }
  *res = index->Search(search_opts);
  return true;
}

bool Database::Query(int64_t id, std::string* data) {
  auto ec = persistence_.Get(std::to_string(id), data);
  return ec == KVStorage::EC_OK || ec == KVStorage::EC_NotFound;
}

bool Database::Reload() {
  LOG(INFO) << "Start to reloading database.";
  if (!persistence_.LoadSnapshot(&index_factory_, &field_bitmap_)) {
    LOG(WARNING) << "Failed to load snapshot.";
    return false;
  }

  Persistence::WAL_TYPE wt;
  std::string data;
  auto st = persistence_.ReadNextWALLog(&wt, &data);

  while (st != Persistence::WS_END) {
    if (st == Persistence::WS_ERROR) {
      LOG(WARNING) << "Failed to read WAL log.";
      return false;
    }

    LOG(INFO) << "Operation Type:" << Persistence::WT_2_STRING[wt];
    if (wt == Persistence::WT_UPSERT) {
      service::UpsertRequest req;
      if (auto st = JsonStrToPb(data, &req); !st.ok()) {
        LOG(WARNING) << "Failed to parse data.";
        return false;
      }
      Database::UpsertOptions opts;
      opts.id = req.id();
      opts.index_type = (service::IndexType)req.index_type();
      opts.data = req.vector().data();
      opts.scalar_data = req.SerializeAsString();
      opts.field = req.mutable_fields();
      if (!Upsert(opts)) {
        LOG(WARNING) << "Failed to upsert.";
        return false;
      }
    }
    st = persistence_.ReadNextWALLog(&wt, &data);
  }
  LOG(INFO) << "Finish to reloading database.";
  return true;
}

bool Database::WriteWALLog(Persistence::WAL_TYPE wt, const std::string& data) {
  return persistence_.WriteWALLog(wt, data);
}

bool Database::SaveSnapshot() { return persistence_.SaveSnapshot(&index_factory_, &field_bitmap_); }

bool Database::LoadSnapshot() { return persistence_.LoadSnapshot(&index_factory_, &field_bitmap_); }

}  // namespace vdb
