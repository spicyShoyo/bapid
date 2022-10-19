#include "src/bapid_server.h"
#include <folly/File.h>
#include <folly/FileUtil.h>
#include <folly/init/Init.h>
#include <folly/logging/xlog.h>
#include <string_view>

namespace {
constexpr mode_t kLogFilePerms = 0644;
constexpr int kStdoutFileno = 1;
constexpr int kStderrFileno = 2;
constexpr std::string_view kNewline("\n");

void writeMessage(folly::File &file, std::string_view message) {
  std::array<iovec, 2> iov{};
  iov[0].iov_base = const_cast<char *>(message.data()); // NOLINT
  iov[0].iov_len = message.size();
  iov[1].iov_base = const_cast<char *>(kNewline.data()); // NOLINT
  iov[1].iov_len = kNewline.size();

  (void)folly::writevFull(file.fd(), iov.data(), iov.size());
}
} // namespace

int main(int argc, char **argv) {
  folly::init(&argc, &argv);
  XCHECK(!FLAGS_log_dir.empty());

  folly::File original_stderr =
      folly::File{kStderrFileno, /*ownsFd=*/false}.dupCloseOnExec();
  folly::File logHandle(FLAGS_log_dir + "/bapid.log",
                        O_APPEND | O_CREAT | O_WRONLY | O_CLOEXEC,
                        kLogFilePerms);
  dup2(logHandle.fd(), kStdoutFileno);
  dup2(logHandle.fd(), kStderrFileno);

  bapid::BapidServer server{"localhost:50051"};
  server.serve(folly::makeSemiFutureWith(
      [original_stderr = std::move(original_stderr)]() mutable {
        XLOG(INFO) << "init";
        writeMessage(original_stderr, "init");
        original_stderr.close();
      }));
  return 0;
}
