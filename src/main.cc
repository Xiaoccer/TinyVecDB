#include <gflags/gflags.h>
#include <glog/logging.h>
#include <vdb_server.h>

DEFINE_int32(port, 7123, "TCP Port of this server");
DEFINE_string(listen_addr, "",
              "Server listen address, may be IPV4/IPV6/UDS."
              " If this is set, the flag port will be ignored");
DEFINE_int32(idle_timeout_s, -1,
             "Connection will be closed if there is no "
             "read/write operations during the last `idle_timeout_s'");
DEFINE_int32(dim, 1, "");

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  FLAGS_logtostderr = true;
  google::ParseCommandLineFlags(&argc, &argv, true);

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

  vdb::VdbServer server;
  vdb::VdbServer::InitOptions opts;
  opts.opts.persistence_path = "./Storage/";
  if (!server.Init(opts)) {
    LOG(ERROR) << "Fail to init VdbServer.";
    return -1;
  }
  if (server.Start(point, &options) != 0) {
    LOG(ERROR) << "Fail to start VdbServer";
    return -1;
  }

  server.RunUntilAskedToQuit();

  google::ShutDownCommandLineFlags();
  google::ShutdownGoogleLogging();
  return 0;
}
