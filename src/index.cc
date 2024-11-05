#include "index.h"
#include <faiss/Index.h>
#include <faiss/IndexFlat.h>
#include <faiss/IndexIDMap.h>
#include <faiss/MetricType.h>
#include <faiss/impl/IDSelector.h>
#include <faiss/index_io.h>
#include <hnswlib/hnswlib.h>
#include <cstdint>
#include <memory>

namespace vdb {

namespace {

/************************************************************************/
/* RoaringBitmap classes */
/************************************************************************/
class FaissRoaringBitmapIDSelector : public faiss::IDSelector {
 private:
  const roaring_bitmap_t* bitmap_{nullptr};

 public:
  explicit FaissRoaringBitmapIDSelector(const roaring_bitmap_t* bitmap) : bitmap_(bitmap) {}
  ~FaissRoaringBitmapIDSelector() override = default;

 public:
  bool is_member(int64_t id) const final {
    // 只支持 32 位？
    return roaring_bitmap_contains(bitmap_, (uint32_t)id);
  }
};

class HNSWRoaringBitmapIDFilter : public hnswlib::BaseFilterFunctor {
 private:
  const roaring_bitmap_t* bitmap_{nullptr};

 public:
  explicit HNSWRoaringBitmapIDFilter(const roaring_bitmap_t* bitmap) : bitmap_(bitmap) {}
  ~HNSWRoaringBitmapIDFilter() override = default;

 public:
  bool operator()(hnswlib::labeltype label) override {
    return roaring_bitmap_contains(bitmap_, static_cast<uint32_t>(label));
  }
};

/************************************************************************/
/* FaissIndex */
/************************************************************************/
class FaissIndex : public Index {
 private:
  std::unique_ptr<faiss::Index> index_;

 public:
  FaissIndex(int dim, MetricType metric) {
    faiss::MetricType faiss_metric = (metric == MetricType::L2) ? faiss::METRIC_L2 : faiss::METRIC_INNER_PRODUCT;
    index_ = std::make_unique<faiss::IndexIDMap>(new faiss::IndexFlat(dim, faiss_metric));
  }
  ~FaissIndex() override = default;

 public:
  void Insert(const InsertOptions& opts) override {
    auto id = (faiss::idx_t)opts.label;
    index_->add_with_ids(1, opts.data, &id);
  }

  SearchRes Search(const SearchOptions& opts) override {
    int dim = index_->d;
    int num_queries = opts.size / dim;
    std::vector<faiss::idx_t> indices(num_queries * opts.k);
    std::vector<float> distances(num_queries * opts.k);

    if (opts.bitmap) {
      faiss::SearchParameters search_params;
      FaissRoaringBitmapIDSelector selector(opts.bitmap);
      search_params.sel = &selector;
      index_->search(num_queries, opts.query, opts.k, distances.data(), indices.data(), &search_params);
    } else {
      index_->search(num_queries, opts.query, opts.k, distances.data(), indices.data());
    }
    return {indices, distances};
  }

  void Remove(const std::vector<int64_t>& ids) override {
    auto* id_map = (faiss::IndexIDMap*)(index_.get());
    if (id_map) {
      faiss::IDSelectorBatch selector(ids.size(), ids.data());
      id_map->remove_ids(selector);
    }
  }

  bool Save(const std::string& path) override {
    faiss::write_index(index_.get(), path.c_str());
    return true;
  }

  bool Load(const std::string& path) override {
    std::ifstream file(path);
    if (file.good()) {
      file.close();
      index_.reset(faiss::read_index(path.c_str()));
      return true;
    }
    // 空 `database` 启动时，没有对应文件。
    return true;
  }
};

/************************************************************************/
/* HNSWLibIndex */
/************************************************************************/
class HNSWLibIndex : public Index {
 private:
  int dim_{0};
  std::unique_ptr<hnswlib::SpaceInterface<float>> space_;
  std::unique_ptr<hnswlib::HierarchicalNSW<float>> index_;

 public:
  HNSWLibIndex(int dim, int num_data, MetricType metric, int M, int ef_construction) {
    dim_ = dim;
    if (metric == MetricType::L2) {
      space_ = std::make_unique<hnswlib::L2Space>(dim);
    } else {
      throw std::runtime_error("Invalid metric type.");
    }
    index_ = std::make_unique<hnswlib::HierarchicalNSW<float>>(space_.get(), num_data, M, ef_construction);
  }
  ~HNSWLibIndex() override = default;

 public:
  void Insert(const InsertOptions& opts) override { index_->addPoint(opts.data, opts.label); }

  SearchRes Search(const SearchOptions& opts) override {
    index_->setEf(opts.ef_search);

    HNSWRoaringBitmapIDFilter selector(opts.bitmap);
    auto result = index_->searchKnn(opts.query, opts.k, opts.bitmap ? &selector : nullptr);

    std::vector<int64_t> indices;
    std::vector<float> distances;
    while (!result.empty()) {
      auto item = result.top();
      indices.push_back(item.second);
      distances.push_back(item.first);
      result.pop();
    }

    return {indices, distances};
  }

  void Remove(const std::vector<int64_t>& /*ids*/) override {
    // Todo(cong): 示例代码没实现？
    return;
  }

  bool Save(const std::string& path) override {
    index_->saveIndex(path);
    return true;
  }

  bool Load(const std::string& path) override {
    std::ifstream file(path);
    if (file.good()) {
      file.close();
      index_->loadIndex(path, space_.get());
      return true;
    }
    // 空 `database` 启动时，没有对应文件。
    return true;
  }
};

}  // namespace

/************************************************************************/
/* Index functions */
/************************************************************************/
std::unique_ptr<Index> NewFaissIndex(int dim, MetricType metric) { return std::make_unique<FaissIndex>(dim, metric); }
std::unique_ptr<Index> NewHNSWLibIndex(int dim, int num_data, MetricType metric, int M, int ef_construction) {
  return std::make_unique<HNSWLibIndex>(dim, num_data, metric, M, ef_construction);
}

}  // namespace vdb
