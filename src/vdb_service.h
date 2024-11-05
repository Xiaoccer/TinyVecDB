#pragma once

#include <gen_cpp/vdb.pb.h>
#include "database.h"

namespace vdb {

/************************************************************************/
/* VdbServiceImpl */
/************************************************************************/
class VdbServiceImpl : public service::VdbService {
 private:
  Database* database_ = nullptr;

 public:
  explicit VdbServiceImpl(Database* database) : database_(database){};
  ~VdbServiceImpl() override = default;

 public:
  void http(google::protobuf::RpcController* cntl_base, const service::HttpRequest* request,
            service::HttpResponse* response, google::protobuf::Closure* done) override;
};

}  // namespace vdb
