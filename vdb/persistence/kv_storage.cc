#include "persistence/kv_storage.h"
#include <glog/logging.h>
#include <rocksdb/db.h>
#include <rocksdb/options.h>

namespace vdb {

/************************************************************************/
/* KVStorage::Impl */
/************************************************************************/
class KVStorage::Impl {
 private:
  rocksdb::DB* db_{nullptr};

 public:
  ~Impl() {
    if (db_) {
      delete db_;
    }
  }

  bool Init(const std::string& path) {
    // TODO(cong): Options 可设置？
    rocksdb::Options options;
    options.create_if_missing = true;
    auto st = rocksdb::DB::Open(options, path, &db_);
    if (!st.ok()) {
      LOG(WARNING) << "Failed to open RocksDB, status=" << st.ToString() << ".";
      return false;
    }
    return true;
  }

  bool Put(std::string_view key, std::string_view value) {
    // TODO(cong): WriteOptions 可设置？
    auto st = db_->Put(rocksdb::WriteOptions(), key, value);
    if (!st.ok()) {
      LOG(WARNING) << "Failed to insert to RocksDB, key=" << key << ",status=" << st.ToString() << ".";
      return false;
    }
    return true;
  }

  ErrorCode Get(std::string_view key, std::string* value) const {
    // TODO(cong): ReadOptions 可设置？
    auto st = db_->Get(rocksdb::ReadOptions(), key, value);
    if (!st.ok()) {
      if (st.IsNotFound()) {
        return EC_NotFound;
      }
      LOG(WARNING) << "Failed to get from RocksDB, key=" << key << ",status=" << st.ToString() << ".";
      return EC_Undefined;
    }
    return EC_OK;
  }
};

/************************************************************************/
/* KVStorage */
/************************************************************************/
KVStorage::KVStorage() : impl_(std::make_unique<Impl>()) {}

KVStorage::~KVStorage() = default;

bool KVStorage::Init(const std::string& path) { return impl_->Init(path); }

bool KVStorage::Put(std::string_view key, std::string_view value) { return impl_->Put(key, value); }

KVStorage::ErrorCode KVStorage::Get(std::string_view key, std::string* value) const { return impl_->Get(key, value); }

}  // namespace vdb
