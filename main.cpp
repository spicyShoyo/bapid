#include "src/bapid_server.h"
#include <folly/File.h>
#include <folly/init/Init.h>
#include <folly/logging/xlog.h>

namespace {
constexpr mode_t kLogFilePerms = 0644;
constexpr int kStdoutFileno = 1;
constexpr int kStderrFileno = 2;
} // namespace

int main(int argc, char **argv) {
  folly::init(&argc, &argv);

  if (!FLAGS_log_dir.empty()) {
    folly::File logHandle(FLAGS_log_dir + "/bapid.log",
                          O_APPEND | O_CREAT | O_WRONLY | O_CLOEXEC,
                          kLogFilePerms);
    dup2(logHandle.fd(), kStdoutFileno);
    dup2(logHandle.fd(), kStderrFileno);
  }

  XLOG(INFO) << "init";

  bapid::BapidServer server{"localhost:50051"};
  server.serve();
  return 0;
}
