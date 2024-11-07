#include "server/service.h"
#include <brpc/closure_guard.h>
#include <brpc/controller.h>
#include <gen_cpp/vdb.pb.h>
#include <glog/logging.h>
#include <google/protobuf/stubs/status.h>
#include <stddef.h>
#include <sstream>
#include <string>
#include "db/database.h"
#include "util/util.h"

namespace vdb {

namespace {

std::string ToString(const brpc::HttpHeader& request, const std::string& body) {
  std::ostringstream buf;
  auto& uri = request.uri();
  auto& unresolved_path = request.unresolved_path();
  buf << "\nuri_path=" << uri.path();
  buf << "\nunresolved_path=" << unresolved_path;
  buf << "\nquery strings:";
  for (auto it = uri.QueryBegin(); it != uri.QueryEnd(); ++it) {
    buf << "\n" << it->first << "=" << it->second;
  }
  buf << "\nheaders:";
  for (auto it = request.HeaderBegin(); it != request.HeaderEnd(); ++it) {
    buf << "\n" << it->first << ":" << it->second;
  }
  buf << "\nbody:" << body;
  return buf.str();
}

/************************************************************************/
/* Handlers */
/************************************************************************/
struct ResponseMsg {
  int ret_code = 0;
  std::string msg;

  ResponseMsg() = default;
  template <typename Message>
  // NOLINTNEXTLINE
  ResponseMsg(const Message& m) : ret_code(m.ret_code()), msg(PbToJsonStr(m)) {}
};

ResponseMsg UpsertHandler(brpc::Controller* cntl, Database* database) {
  service::UpsertRequest req;
  service::EmptyResponse resp;
  auto st = JsonStrToPb(cntl->request_attachment().to_string(), &req);
  if (!st.ok()) {
    resp.set_ret_code(400);
    resp.set_msg("Failed to parse http request");
    return resp;
  }

  if (req.vector().empty() || !req.index_type() || !req.id()) {
    resp.set_ret_code(400);
    resp.set_msg("Failed to upsert, invalid params");
    return resp;
  }

  if (!database->WriteWALLog(Database::WT_UPSERT, PbToJsonStr(req))) {
    LOG(WARNING) << "Failed to write wal log.";
    resp.set_ret_code(400);
    resp.set_msg("Failed to write wal log");
    return resp;
  }

  Database::UpsertOptions opts;
  opts.id = req.id();
  opts.index_type = (service::IndexType)req.index_type();
  opts.data = req.vector().data();
  opts.scalar_data = req.SerializeAsString();
  opts.field = req.mutable_fields();
  if (!database->Upsert(opts)) {
    LOG(WARNING) << "Failed to upsert.";
    resp.set_ret_code(400);
    resp.set_msg("Failed to upsert");
    return resp;
  }

  resp.set_ret_code(200);
  resp.set_msg("ok");
  return resp;
}

ResponseMsg SearchHandler(brpc::Controller* cntl, Database* database) {
  service::SearchRequest req;
  service::SearchResponse resp;
  auto st = JsonStrToPb(cntl->request_attachment().to_string(), &req);
  if (!st.ok()) {
    resp.set_ret_code(400);
    resp.set_msg("Failed to parse http request");
    return resp;
  }

  if (req.vector().empty() || !req.index_type() || !req.k()) {
    resp.set_ret_code(400);
    resp.set_msg("Failed to search, invalid params");
    return resp;
  }

  if (req.has_condition() && req.condition().op() != "=" && req.condition().op() != "!=") {
    resp.set_ret_code(400);
    resp.set_msg("Failed to search, invalid filter op");
    return resp;
  }

  Database::SearchOptions opts;
  opts.index_type = (service::IndexType)req.index_type();
  opts.query = req.vector().data();
  opts.size = req.vector_size();
  opts.k = req.k();
  opts.filter_field = req.condition().field();
  opts.filter_op = req.condition().op();
  opts.filter_value = req.condition().value();
  Database::SearchResult res;
  if (!database->Search(opts, &res)) {
    LOG(WARNING) << "Failed to search.";
    resp.set_ret_code(400);
    resp.set_msg("Failed to search");
  }

  resp.set_ret_code(200);
  resp.set_msg("ok");
  for (size_t i = 0; i < res.indices.size(); ++i) {
    if (res.indices[i] != -1) {
      resp.mutable_indices()->Add(res.indices[i]);
      resp.mutable_distances()->Add(res.distances[i]);
    }
  }
  return resp;
}

ResponseMsg QueryHandler(brpc::Controller* cntl, Database* database) {
  service::QueryRequest req;
  service::QueryResponse resp;
  auto st = JsonStrToPb(cntl->request_attachment().to_string(), &req);
  if (!st.ok()) {
    resp.set_ret_code(400);
    resp.set_msg("Failed to parse http request");
    return resp;
  }

  if (!req.id()) {
    resp.set_ret_code(400);
    resp.set_msg("Failed to query, invalid params");
    return resp;
  }

  std::string value;
  if (!database->Query(req.id(), &value)) {
    LOG(WARNING) << "Failed to query.";
    resp.set_ret_code(400);
    resp.set_msg("Failed to query");
  }

  resp.set_ret_code(200);
  resp.set_msg("ok");
  resp.mutable_upsert_data()->ParseFromString(value);
  return resp;
}

// TODO(cong): 自动 snapshot
ResponseMsg Snapshot(Database* database) {
  service::EmptyResponse resp;
  if (!database->SaveSnapshot()) {
    LOG(WARNING) << "Failed to search.";
    resp.set_ret_code(400);
    resp.set_msg("Failed to search");
  } else {
    resp.set_ret_code(200);
    resp.set_msg("ok");
  }
  return resp;
}

ResponseMsg UnknownHandler() {
  service::EmptyResponse resp;
  resp.set_ret_code(400);
  resp.set_msg("Failed to find unresolved_path");
  return resp;
}

}  // namespace

/************************************************************************/
/* VdbServiceImpl */
/************************************************************************/
void VdbServiceImpl::http(google::protobuf::RpcController* cntl_base, const service::HttpRequest* /* request */,
                          service::HttpResponse* /* response */, google::protobuf::Closure* done) {
  brpc::ClosureGuard done_guard(done);

  auto* cntl = (brpc::Controller*)(cntl_base);

  LOG(INFO) << "Received request[log_id=" << cntl->log_id() << "] from " << cntl->remote_side() << " to "
            << cntl->local_side() << ": " << cntl->http_request().uri().path();

  const std::string& unresolved_path = cntl->http_request().unresolved_path();
  ResponseMsg rm;
  if (unresolved_path == "upsert") {
    rm = UpsertHandler(cntl, database_);
  } else if (unresolved_path == "search") {
    rm = SearchHandler(cntl, database_);
  } else if (unresolved_path == "query") {
    rm = QueryHandler(cntl, database_);
  } else if (unresolved_path == "snapshot") {
    rm = Snapshot(database_);
  } else {
    LOG(WARNING) << "Failed to find unresolved_path";
    rm = UnknownHandler();
  }
  cntl->http_response().set_content_type("text/plain");
  cntl->http_response().set_status_code(rm.ret_code);
  cntl->response_attachment().append(rm.msg);
  cntl->response_attachment().append("\n");

  int ret = cntl->http_response().status_code();
  LOG(INFO) << "Send response[log_id=" << cntl->log_id() << "]," << (ret == 200 ? "succ to " : "failed to ")
            << "handle: " << cntl->remote_side()
            << ",request=" << ToString(cntl->http_request(), cntl->request_attachment().to_string())
            << ",response=" << rm.msg;
}

}  // namespace vdb
