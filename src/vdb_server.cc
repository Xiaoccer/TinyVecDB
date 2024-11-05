#include "vdb_server.h"
#include <gen_cpp/vdb.pb.h>
#include <glog/logging.h>
#include <memory>
#include "vdb_service.h"

namespace vdb {

/************************************************************************/
/* VdbServer */
/************************************************************************/
bool VdbServer::Init(const InitOptions& opts) {
  if (!database_.Init(opts.opts)) {
    return false;
  }
  if (!database_.Reload()) {
    return false;
  }
  vdb_service_ = std::make_unique<VdbServiceImpl>(&database_);

  if (AddService(vdb_service_.get(), brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
    LOG(ERROR) << "Failed to add service";
    return false;
  }
  return true;
}

}  // namespace vdb
