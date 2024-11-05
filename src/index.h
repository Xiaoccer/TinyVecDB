#pragma once

#include <roaring/roaring.h>
#include <cstdint>
#include <memory>
#include <vector>

namespace vdb {

enum class MetricType { L2, IP };

/************************************************************************/
/* Index */
/************************************************************************/
class Index {
 public:
  struct InsertOptions {
    const float* data{nullptr};
    int64_t label{-1};
  };

  struct SearchOptions {
    const float* query{nullptr};
    size_t size{0};
    int k{0};
    int ef_search{50};
    const roaring_bitmap_t* bitmap{nullptr};
  };

  struct SearchRes {
    std::vector<int64_t> indices;
    std::vector<float> distances;
  };

 public:
  Index() = default;
  virtual ~Index() = default;
  Index(const Index&) = delete;
  Index(Index&&) = delete;
  Index& operator=(const Index&) = delete;
  Index& operator=(Index&&) = delete;

 public:
  virtual void Insert(const InsertOptions& opts) = 0;
  [[nodiscard]] virtual SearchRes Search(const SearchOptions& opts) = 0;
  virtual void Remove(const std::vector<int64_t>& ids) = 0;
  [[nodiscard]] virtual bool Save(const std::string& path) = 0;
  [[nodiscard]] virtual bool Load(const std::string& path) = 0;
};

/************************************************************************/
/* Index functions */
/************************************************************************/
std::unique_ptr<Index> NewFaissIndex(int dim, MetricType metric);
std::unique_ptr<Index> NewHNSWLibIndex(int dim, int num_data, MetricType metric, int M = 16, int ef_construction = 200);

}  // namespace vdb
