#include "db/database.h"
#include <glog/logging.h>
#include <utility>
#include "bitmap/field_bitmap.h"
#include "index/index.h"
#include "index/index_factory.h"
#include "persistence/persistence.h"
#include "util/util.h"

namespace vdb {

const uint8_t VERSION = 1;

/************************************************************************/
/* Database::Impl */
/************************************************************************/
class Database::Impl {
 private:
  IndexFactory index_factory_;
  FieldBitmap field_bitmap_;
  Persistence persistence_;

 public:
  bool Init(const InitOptions& opts) {
    if (!persistence_.Init(opts.persistence_path, VERSION)) {
      return false;
    }

    index_factory_.Add(vdb::service::IndexType::IT_FLAT, vdb::NewFaissIndex(opts.dim, vdb::MetricType::L2));
    index_factory_.Add(vdb::service::IndexType::IT_HNSW,
                       vdb::NewHNSWLibIndex(opts.dim, opts.num_data, vdb::MetricType::L2));
    return true;
  }

  bool Upsert(const UpsertOptions& opts) {
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

  bool Search(const SearchOptions& opts, SearchResult* res) {
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
    auto s_res = index->Search(search_opts);
    res->distances = std::move(s_res.distances);
    res->indices = std::move(s_res.indices);
    return true;
  }

  bool Query(int64_t id, std::string* data) {
    auto ec = persistence_.Get(std::to_string(id), data);
    return ec == KVStorage::EC_OK || ec == KVStorage::EC_NotFound;
  }

  bool Reload() {
    LOG(INFO) << "Start to reloading database.";
    if (!persistence_.LoadSnapshot(&index_factory_, &field_bitmap_)) {
      LOG(WARNING) << "Failed to load snapshot.";
      return false;
    }

    WAL_TYPE wt;
    std::string data;
    auto st = persistence_.ReadNextWALLog((char*)&wt, &data);

    while (st != Persistence::LS_END) {
      if (st == Persistence::LS_ERROR) {
        LOG(WARNING) << "Failed to read WAL log.";
        return false;
      }

      LOG(INFO) << "Operation Type:" << WT_2_STRING[wt];
      if (wt == WT_UPSERT) {
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
      st = persistence_.ReadNextWALLog((char*)&wt, &data);
    }
    LOG(INFO) << "Finish to reloading database.";
    return true;
  }

  bool WriteWALLog(WAL_TYPE wt, const std::string& data) { return persistence_.WriteWALLog((char)wt, data); }

  bool SaveSnapshot() { return persistence_.SaveSnapshot(&index_factory_, &field_bitmap_); }

  bool LoadSnapshot() { return persistence_.LoadSnapshot(&index_factory_, &field_bitmap_); }
};

/************************************************************************/
/* Database */
/************************************************************************/
Database::Database() : impl_(std::make_unique<Impl>()) {}

Database::~Database() {}

bool Database::Init(const InitOptions& opts) { return impl_->Init(opts); }

bool Database::Upsert(const UpsertOptions& opts) { return impl_->Upsert(opts); }

bool Database::Search(const SearchOptions& opts, SearchResult* res) { return impl_->Search(opts, res); }

bool Database::Query(int64_t id, std::string* data) { return impl_->Query(id, data); }

bool Database::Reload() { return impl_->Reload(); }

bool Database::WriteWALLog(WAL_TYPE wt, const std::string& data) { return impl_->WriteWALLog(wt, data); }

bool Database::SaveSnapshot() { return impl_->SaveSnapshot(); }

bool Database::LoadSnapshot() { return impl_->LoadSnapshot(); }

}  // namespace vdb
