#include "persistence/persistence.h"
#include <errno.h>
#include <glog/logging.h>
#include <stddef.h>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <ios>
#include "util/util.h"

namespace vdb {

namespace {

/************************************************************************/
/* Folder */
/************************************************************************/
const std::string WAL_LOG_FOLDER = "/wal/";
const std::string KV_STORAGE_FOLDER = "/kv_storage/";
const std::string SNAPSHOT_FOLDER = "/snapshot/";

/************************************************************************/
/* Inner key prefix of KV storage*/
/************************************************************************/
const std::string SNAPSHOT_PREFIX = "meta/snapshot/";
const std::string EXTERNAL_PREFIX = "external/data/";

/************************************************************************/
/* Meta key of KV storage*/
/************************************************************************/
const std::string BITMAP_KEY = SNAPSHOT_PREFIX + "bitmap";
const std::string LAST_SNAPSHOT_ID = SNAPSHOT_PREFIX + "last_snapshot_id";

}  // namespace

/************************************************************************/
/* Persistence::Impl */
/************************************************************************/
class Persistence::Impl {
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
  ~Impl() {
    if (wal_log_file_.is_open()) {
      wal_log_file_.close();
    }
  }

 public:
  bool Init(const std::string& path, uint8_t version) {
    version_ = version;
    wal_path_ = path + WAL_LOG_FOLDER;
    kv_storage_path_ = path + KV_STORAGE_FOLDER;
    snapshot_path_ = path + SNAPSHOT_FOLDER;
    if (!fs::is_directory(wal_path_) && !fs::create_directories(wal_path_)) {
      LOG(WARNING) << fs::current_path();
      LOG(WARNING) << "Failed to create wal_path=" << std::quoted(wal_path_.native()) << ".";
      return false;
    }
    if (!fs::is_directory(kv_storage_path_) && !fs::create_directories(kv_storage_path_)) {
      LOG(WARNING) << "Failed to create kv_storage_path=" << std::quoted(kv_storage_path_.native()) << ".";
      return false;
    }
    if (!fs::is_directory(snapshot_path_) && !fs::create_directories(snapshot_path_)) {
      LOG(WARNING) << "Failed to create snapshot_path=" << std::quoted(snapshot_path_.native()) << ".";
      return false;
    }

    wal_log_file_.open(wal_path_.native() + "log.log", std::ios::in | std::ios::out | std::ios::app);
    if (!wal_log_file_.is_open()) {
      LOG(WARNING) << "Failed to open WAL log file, error=" << std::strerror(errno)
                   << ",path=" << std::quoted(wal_path_.native()) << ".";
      return false;
    }

    if (!kv_storage_.Init(kv_storage_path_)) {
      LOG(WARNING) << "Failed to init kv storage ,path=" << std::quoted(kv_storage_path_.native()) << ".";
      return false;
    }
    return true;
  }

  /**
   *
   * Format of each log:
   * ----------------------------------------------------------------------------
   * | TotalSize (8) | LogID (8) | Version (1) | OP (1) |
   * ----------------------------------------------------------------------------
   * | DataSize (8) | Data |
   * ----------------------------------------------------------------------------
   *
   */
  bool WriteWALLog(char op, const std::string& data) {
    ++log_id_;

    uint64_t total_size = 8 + 1 + 1 + 8 + data.size();
    uint64_t data_size = data.size();
    wal_log_file_.write((char*)&total_size, 8);
    wal_log_file_.write((char*)&log_id_, 8);
    wal_log_file_.write((char*)&version_, 1);
    wal_log_file_.write((char*)&op, 1);
    wal_log_file_.write((char*)&data_size, 8);
    wal_log_file_.write(data.data(), data.size());

    if (wal_log_file_.fail()) {
      LOG(WARNING) << "An error occurred while writing the WAL log entry, error=" << std::strerror(errno) << ".";
      return false;
    } else {
      LOG(INFO) << "Wrote WAL log entry: log_id=" << log_id_ << ",version=" << (int32_t)version_
                << ",op=" << (int32_t)op << ",data=" << data << ".";
      wal_log_file_.flush();
    }
    return true;
  }

  LOG_STATUS ReadNextWALLog(char* op, std::string* data) {
    LOG(INFO) << "Reading next WAL log entry";

    uint64_t total_size = 0;
    while (wal_log_file_.read((char*)&total_size, 8)) {
      std::string buf;
      buf.resize(total_size);
      wal_log_file_.read(buf.data(), total_size);
      size_t offset = 0;

      uint64_t log_id;
      std::memcpy(&log_id, buf.data() + offset, 8);
      offset += 8;

      if (log_id > log_id_) {
        log_id_ = log_id;
      }

      if (log_id_ <= last_snapshot_id_) {
        LOG(INFO) << "Skip WAL log entry: log_id=" << log_id << ",last_snapshot_id=" << last_snapshot_id_ << ".";
        continue;
      }

      std::memcpy(&version_, buf.data() + offset, 1);
      offset += 1;

      std::memcpy(op, buf.data() + offset, 1);
      offset += 1;
      uint64_t data_size = 0;
      std::memcpy(&data_size, buf.data() + offset, 8);
      offset += 8;

      data->assign(buf.data() + offset, data_size);
      LOG(INFO) << "Read WAL log entry: log_id=" << log_id_ << ",version=" << (int32_t)version_
                << ",op=" << (int32_t)(*op) << ",data=" << *data << ".";

      return LOG_STATUS::LS_OK;
    }

    wal_log_file_.clear();
    LOG(INFO) << "No more WAL log entries to read";
    return LOG_STATUS::LS_END;
  }

  bool Put(std::string_view key, std::string_view value) {
    std::string encode_key = EXTERNAL_PREFIX + std::string(key.data(), key.size());
    return kv_storage_.Put(encode_key, value);
  }

  KVStorage::ErrorCode Get(std::string_view key, std::string* value) const {
    std::string encode_key = EXTERNAL_PREFIX + std::string(key.data(), key.size());
    return kv_storage_.Get(encode_key, value);
  }

  // TODO(cong): 原子性？
  bool SaveSnapshot(IndexFactory* index_factory, FieldBitmap* bitmap) {
    LOG(INFO) << "Start to saving snapshot.";
    last_snapshot_id_ = log_id_;

    if (!index_factory->SaveIndex(snapshot_path_)) {
      LOG(WARNING) << "Failed to save index.";
      return false;
    }

    if (!kv_storage_.Put(BITMAP_KEY, bitmap->SerializeToString())) {
      LOG(WARNING) << "Failed to save bitmap.";
      return false;
    }

    std::string id_str = std::to_string(last_snapshot_id_);
    if (!kv_storage_.Put(LAST_SNAPSHOT_ID, id_str)) {
      LOG(WARNING) << "Failed to save last_snapshot_id.";
      return false;
    }

    LOG(INFO) << "Finish to saving snapshot, last_snapshot_id=" << last_snapshot_id_;
    return true;
  }

  bool LoadSnapshot(IndexFactory* index_factory, FieldBitmap* bitmap) {
    LOG(INFO) << "Start to loading snapshot.";

    if (!index_factory->LoadIndex(snapshot_path_)) {
      LOG(WARNING) << "Failed to load index.";
      return false;
    }

    std::string bitmap_value;
    auto ec = kv_storage_.Get(BITMAP_KEY, &bitmap_value);
    if (ec == KVStorage::EC_Undefined) {
      LOG(WARNING) << "Failed to get bitmap.";
      return false;
    }
    if (ec == KVStorage::EC_OK && !bitmap->ParseFromString(bitmap_value)) {
      LOG(WARNING) << "Failed to parse bitmap.";
      return false;
    }

    std::string last_snapshot_id_value;
    ec = kv_storage_.Get(LAST_SNAPSHOT_ID, &last_snapshot_id_value);
    if (ec == KVStorage::EC_Undefined) {
      LOG(WARNING) << "Failed to get last_snapshot_id.";
      return false;
    }
    if (ec == KVStorage::EC_OK) {
      LOG(INFO) << "dd:" << last_snapshot_id_value;
      last_snapshot_id_ = std::stol(last_snapshot_id_value);
    }

    LOG(INFO) << "Finish to loading snapshot, last_snapshot_id=" << last_snapshot_id_;
    return true;
  }
};

/************************************************************************/
/* Persistence */
/************************************************************************/
Persistence::Persistence() : impl_(std::make_unique<Impl>()) {}

Persistence::~Persistence() = default;

bool Persistence::Init(const std::string& path, uint8_t version) { return impl_->Init(path, version); }

bool Persistence::WriteWALLog(char op, const std::string& data) { return impl_->WriteWALLog(op, data); }

Persistence::LOG_STATUS Persistence::ReadNextWALLog(char* op, std::string* data) {
  return impl_->ReadNextWALLog(op, data);
}

bool Persistence::Put(std::string_view key, std::string_view value) { return impl_->Put(key, value); }

KVStorage::ErrorCode Persistence::Get(std::string_view key, std::string* value) const { return impl_->Get(key, value); }

bool Persistence::SaveSnapshot(IndexFactory* index_factory, FieldBitmap* bitmap) {
  return impl_->SaveSnapshot(index_factory, bitmap);
}

bool Persistence::LoadSnapshot(IndexFactory* index_factory, FieldBitmap* bitmap) {
  return impl_->LoadSnapshot(index_factory, bitmap);
}

}  // namespace vdb
