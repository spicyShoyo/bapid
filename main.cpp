#include "src/bapid.h"
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <iostream>

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  FLAGS_timestamp_in_logfile_name = false;
  google::InitGoogleLogging(argv[0]); // NOLINT

  std::string logDir = google::GetLoggingDirectories()[0] + "/bapid.log";
  google::SetLogDestination(google::INFO, logDir.c_str());
  google::SetLogDestination(google::WARNING, logDir.c_str());
  google::SetLogDestination(google::ERROR, logDir.c_str());
  google::SetLogDestination(google::FATAL, logDir.c_str());

  LOG(INFO) << "init";
  return bapid::BapidMain::run();
}
