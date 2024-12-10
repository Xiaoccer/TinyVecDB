#include <gflags/gflags.h>
#include <glog/logging.h>
#include <iostream>
#include <sstream>
#include "buildinfo.h"
#include "server/server.h"

DEFINE_int32(port, 7123, "TCP Port of this server");
DEFINE_string(listen_addr, "",
              "Server listen address, may be IPV4/IPV6/UDS."
              " If this is set, the flag port will be ignored");
DEFINE_int32(idle_timeout_s, -1,
             "Connection will be closed if there is no "
             "read/write operations during the last `idle_timeout_s'");
DEFINE_int32(vec_dim, 1, "Dimension of each vector");
DEFINE_string(persistence_path, "./storage/", "Path to store persistent data");
DEFINE_bool(show_info, false, "show version");

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  FLAGS_logtostderr = true;
  google::ParseCommandLineFlags(&argc, &argv, true);

  std::ostringstream os;
  os << "BuildInfo";
  os << " timestamp=" << BuildInfo::Timestamp;
  os << " commit=" << BuildInfo::CommitSHA;
  os << " version=" << BuildInfo::Version;
  if (FLAGS_show_info) {
    std::cout << os.str();
    return 0;
  }

  LOG(INFO) << os.str();

  vdb::VdbServer server;
  vdb::VdbServer::InitOptions opts;
  auto db_opts = &opts.db_opts;
  db_opts->persistence_path = FLAGS_persistence_path;
  db_opts->dim = FLAGS_vec_dim;
  if (!server.Init(opts)) {
    LOG(ERROR) << "Fail to init VdbServer.";
    return -1;
  }

  butil::EndPoint point;
  if (!FLAGS_listen_addr.empty()) {
    if (butil::str2endpoint(FLAGS_listen_addr.c_str(), &point) < 0) {
      LOG(ERROR) << "Invalid listen address:" << FLAGS_listen_addr << ".";
      return -1;
    }
  } else {
    point = butil::EndPoint(butil::IP_ANY, FLAGS_port);
  }
  brpc::ServerOptions options;
  options.idle_timeout_sec = FLAGS_idle_timeout_s;
  if (server.Start(point, &options) != 0) {
    LOG(ERROR) << "Fail to start VdbServer";
    return -1;
  }

  server.RunUntilAskedToQuit();

  google::ShutDownCommandLineFlags();
  google::ShutdownGoogleLogging();
  return 0;
}
