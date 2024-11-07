#include "bitmap/field_bitmap.h"
#include <glog/logging.h>
#include <stddef.h>
#include <stdint.h>
#include <memory>
#include <optional>
#include <sstream>
#include <vector>
#include "util/util.h"

namespace vdb {

/************************************************************************/
/* FieldBitmap */
/************************************************************************/
void FieldBitmap::UpdateFiledValue(int64_t id, const std::string& field_name, int64_t new_value,
                                   std::optional<int64_t> old_value) {
  auto it = field_bitmap_.find(field_name);
  if (it == field_bitmap_.end()) {
    AddFieldValue(id, field_name, new_value);
    return;
  }

  // 先清理旧 `value` 中的位图
  // TODO(cong): 删除空的 key?
  auto& value_map = it->second;
  auto old_bitmap_it = (old_value.has_value()) ? value_map.find(old_value.value()) : value_map.end();
  if (old_bitmap_it != value_map.end()) {
    roaring_bitmap_t* old_bitmap = old_bitmap_it->second.get();
    roaring_bitmap_remove(old_bitmap, id);
  }

  // 修改新 `value` 中的位图
  auto new_bitmap_it = value_map.find(new_value);
  if (new_bitmap_it == value_map.end()) {
    AddFieldValue(id, field_name, new_value);
  } else {
    roaring_bitmap_t* new_bitmap = new_bitmap_it->second.get();
    roaring_bitmap_add(new_bitmap, id);
  }
}

roaring_bitmap_ptr FieldBitmap::GetBitmap(const std::string& field_name, int64_t value, Operation op) {
  roaring_bitmap_ptr bitmap(roaring_bitmap_create(), roaring_bitmap_free);
  if (auto it = field_bitmap_.find(field_name); it != field_bitmap_.end()) {
    const auto& value_map = it->second;
    if (op == Operation::EQUAL) {
      auto bitmap_it = value_map.find(value);
      if (bitmap_it != value_map.end()) {
        roaring_bitmap_or_inplace(bitmap.get(), bitmap_it->second.get());
      }
    } else if (op == Operation::NOT_EQUAL) {
      for (const auto& entry : value_map) {
        if (entry.first != value) {
          roaring_bitmap_or_inplace(bitmap.get(), entry.second.get());
        }
      }
    }
  }
  return bitmap;
}

std::string FieldBitmap::SerializeToString() {
  std::ostringstream oss;
  for (const auto& field_entry : field_bitmap_) {
    const std::string& field_name = field_entry.first;
    const auto& value_map = field_entry.second;

    for (const auto& value_entry : value_map) {
      int64_t value = value_entry.first;
      const roaring_bitmap_t* bitmap = value_entry.second.get();
      if (roaring_bitmap_is_empty(bitmap)) {
        continue;
      }
      size_t size = roaring_bitmap_portable_size_in_bytes(bitmap);
      std::string data;
      data.resize(size);
      roaring_bitmap_portable_serialize(bitmap, data.data());
      uint64_t total_size = 8 + field_name.size() + 8 + 8 + data.size();

      std::string buf;
      buf.resize(total_size + 8 - data.size());
      size_t offset = 0;
      std::memcpy(buf.data() + offset, &total_size, 8);
      offset += 8;

      uint64_t field_name_size = field_name.size();
      std::memcpy(buf.data() + offset, &field_name_size, 8);
      offset += 8;

      std::memcpy(buf.data() + offset, field_name.data(), field_name_size);
      offset += field_name_size;

      std::memcpy(buf.data() + offset, &value, 8);
      offset += 8;

      uint64_t data_size = data.size();
      std::memcpy(buf.data() + offset, &data_size, 8);
      offset += 8;

      oss << buf << data;
    }
  }
  return oss.str();
}

bool FieldBitmap::ParseFromString(const std::string& data) {
  uint64_t offset = 0;
  while (offset < data.size()) {
    uint64_t total_size;
    std::memcpy(&total_size, data.data() + offset, 8);
    offset += 8;

    uint64_t field_name_size;
    std::memcpy(&field_name_size, data.data() + offset, 8);
    offset += 8;
    std::string field_name;
    field_name.assign(data.data() + offset, field_name_size);
    offset += field_name_size;

    uint64_t value;
    std::memcpy(&value, data.data() + offset, 8);
    offset += 8;

    uint64_t data_size = 0;
    std::memcpy(&data_size, data.data() + offset, 8);
    offset += 8;
    std::string bitmap_str;
    bitmap_str.assign(data.data() + offset, data_size);
    offset += data_size;

    roaring_bitmap_ptr p(roaring_bitmap_portable_deserialize(bitmap_str.data()), roaring_bitmap_free);
    LOG(INFO) << field_name << " " << value;
    field_bitmap_[field_name][value] = std::move(p);
  }

  return true;
}

void FieldBitmap::AddFieldValue(int64_t id, const std::string& field_name, int64_t value) {
  roaring_bitmap_ptr bitmap(roaring_bitmap_create(), roaring_bitmap_free);
  roaring_bitmap_add(bitmap.get(), id);
  field_bitmap_[field_name][value] = std::move(bitmap);
  LOG(INFO) << "Added int field filter: id=" << id << ",field=" << field_name << ",value=" << value << ".";
}

}  // namespace vdb
