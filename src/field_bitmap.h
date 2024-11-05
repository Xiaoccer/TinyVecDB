#pragma once

#include <roaring/roaring.h>
#include <memory>
#include <unordered_map>

namespace vdb {

using roaring_bitmap_ptr = std::shared_ptr<roaring_bitmap_t>;

/************************************************************************/
/* FieldBitmap */
/************************************************************************/
class FieldBitmap {
 public:
  enum class Operation {
    EQUAL,
    NOT_EQUAL,
  };

 private:
  // TODO(cong): 支持多类型？
  std::unordered_map<std::string, std::unordered_map<int64_t, roaring_bitmap_ptr>> field_bitmap_;

 public:
  void UpdateFiledValue(int64_t id, const std::string& field_name, int64_t new_value,
                        std::optional<int64_t> old_value = {});
  [[nodiscard]] roaring_bitmap_ptr GetBitmap(const std::string& field_name, int64_t value, Operation op);

 public:
  [[nodiscard]] std::string SerializeToString();
  [[nodiscard]] bool ParseFromString(const std::string& data);

 private:
  void AddFieldValue(int64_t id, const std::string& field_name, int64_t value);
};

}  // namespace vdb
