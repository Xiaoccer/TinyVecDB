#pragma once

#include <brpc/server.h>
#include <memory>
#include "database.h"
#include "vdb_service.h"

namespace vdb {

/************************************************************************/
/* VdbServer */
/************************************************************************/
class VdbServer : public brpc::Server {
 public:
  struct InitOptions {
    Database::InitOptions opts;
  };

 private:
  std::unique_ptr<VdbServiceImpl> vdb_service_;
  Database database_;

 public:
  [[nodiscard]] bool Init(const InitOptions& opts);
};

}  // namespace vdb
