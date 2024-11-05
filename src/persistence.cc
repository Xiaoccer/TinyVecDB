#include "persistence.h"
#include <glog/logging.h>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include "util.h"

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
/* Persistence */
/************************************************************************/
Persistence::~Persistence() {
  if (wal_log_file_.is_open()) {
    wal_log_file_.close();
  }
}

bool Persistence::Init(const std::string& path, uint8_t version) {
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

bool Persistence::WriteWALLog(WAL_TYPE wt, const std::string& data) {
  ++log_id_;

  // std::string write_buf;

  // write_buf.resize(total_size);
  // char* cur = write_buf.data();
  // EncodeFixed64(cur, total_size);
  // cur += 8;
  // EncodeFixed64(cur, log_id);
  // cur += 8;
  // EncodeFixed64(cur, version_);
  // cur += 8;
  // EncodeFixed64(cur, uint64_t(wt));
  // cur += 8;
  // EncodeFixed64(cur, (uint64_t)data.size());
  // cur += 8;
  // std::memcpy(cur, data.data(), data.size());

  // total_size(uint64_t) + log_id (uint64_t) + version (uint64_t) + wal_type (uint64_t) + data.size() + data
  uint64_t total_size = 8 + 1 + 1 + 8 + data.size();
  uint64_t data_size = data.size();
  wal_log_file_.write((char*)&total_size, 8);
  wal_log_file_.write((char*)&log_id_, 8);
  wal_log_file_.write((char*)&version_, 1);
  wal_log_file_.write((char*)&wt, 1);
  wal_log_file_.write((char*)&data_size, 8);
  wal_log_file_.write(data.data(), data.size());

  if (wal_log_file_.fail()) {  // 检查是否发生错误
    LOG(WARNING) << "An error occurred while writing the WAL log entry, error=" << std::strerror(errno) << ".";
    return false;
  } else {
    LOG(INFO) << "Wrote WAL log entry: log_id=" << log_id_ << ",version=" << (int32_t)version_
              << ",wal_type=" << WT_2_STRING[wt] << ",data=" << data << ".";
    wal_log_file_.flush();
  }
  return true;
}

Persistence::WAL_STATUS Persistence::ReadNextWALLog(WAL_TYPE* wt, std::string* data) {
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

    std::memcpy(wt, buf.data() + offset, 1);
    offset += 1;
    uint64_t data_size = 0;
    std::memcpy(&data_size, buf.data() + offset, 8);
    offset += 8;

    // Read data
    data->assign(buf.data() + offset, data_size);
    LOG(INFO) << "Read WAL log entry: log_id=" << log_id_ << ",version=" << (int32_t)version_
              << ",wal_type=" << WT_2_STRING[*wt] << ",data=" << *data << ".";

    return WAL_STATUS::WS_OK;
  }

  wal_log_file_.clear();
  LOG(INFO) << "No more WAL log entries to read";
  return WAL_STATUS::WS_END;

  // std::string line;
  // if (std::getline(wal_log_file_, line)) {
  //   std::istringstream iss(line);
  //   std::string log_id_str, version, wt_str, data_str;

  //   std::getline(iss, log_id_str, '|');
  //   uint64_t log_id = std::stoull(log_id_str);
  //   if (log_id > unique_id_) {
  //     unique_id_ = log_id;
  //   }

  //   if (log_id <= last_snapshot_id_) {
  //     LOG(INFO) << "Skip WAL log entry: log_id=" << log_id << ",last_snapshot_id=" << last_snapshot_id_ << ".";
  //     return;
  //   }

  //   std::getline(iss, version, '|');
  //   std::getline(iss, wt_str, '|');
  //   std::getline(iss, data_str, '|');

  //   if (wt_str == "UPSERT") {
  //     *wt = WT_UPSERT;
  //   } else {
  //     *wt = WT_NONE;
  //   }
  //   *data = std::move(data_str);

  //   LOG(INFO) << "Read WAL log entry: log_id=" << log_id << ",version=" << version << ",wal_type=" << wt_str
  //             << ",data=" << *data << ".";

  // } else {
  //   wal_log_file_.clear();
  //   LOG(INFO) << "No more WAL log entries to read";
  // }
}

bool Persistence::Put(std::string_view key, std::string_view value) {
  std::string encode_key = EXTERNAL_PREFIX + std::string(key.data(), key.size());
  return kv_storage_.Put(encode_key, value);
}

KVStorage::ErrorCode Persistence::Get(std::string_view key, std::string* value) const {
  std::string encode_key = EXTERNAL_PREFIX + std::string(key.data(), key.size());
  return kv_storage_.Get(encode_key, value);
}

// TODO(cong): 原子性？
bool Persistence::SaveSnapshot(IndexFactory* index_factory, FieldBitmap* bitmap) {
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

bool Persistence::LoadSnapshot(IndexFactory* index_factory, FieldBitmap* bitmap) {
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
  LOG(INFO) << "fuck: " << bitmap_value;
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

}  // namespace vdb
