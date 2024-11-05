#pragma once

#include <glog/logging.h>
#include <google/protobuf/stubs/status.h>
#include <google/protobuf/util/json_util.h>
#include <sys/types.h>
#include <cstddef>
#include <iomanip>

namespace vdb {

inline std::string PbToJsonStr(const google::protobuf::Message& msg) {
  std::string str;
  static google::protobuf::util::JsonPrintOptions json_print_option;
  json_print_option.preserve_proto_field_names = true;
  json_print_option.add_whitespace = false;

  auto st = google::protobuf::util::MessageToJsonString(msg, &str, json_print_option);
  if (!st.ok()) {
    LOG(WARNING) << "Failed to convert pb to string.";
    return st.ToString();
  }
  return str;
}

inline google::protobuf::util::Status JsonStrToPb(const std::string& str, google::protobuf::Message* msg) {
  google::protobuf::util::JsonParseOptions json_parse_options;
  json_parse_options.ignore_unknown_fields = false;
  auto st = google::protobuf::util::JsonStringToMessage(str, msg, json_parse_options);
  if (!st.ok()) {
    LOG(WARNING) << "Failed to parse http request"
                 << ",body=" << std::quoted(str) << ",error=" << st.ToString() << ".";
    return st;
  }
  return google::protobuf::util::Status();
}

}  // namespace vdb
