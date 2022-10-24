#include "src/bapid.h"
#include <fmt/core.h>
#include <folly/File.h>
#include <folly/FileUtil.h>
#include <folly/init/Init.h>
#include <folly/logging/xlog.h>
#include <tuple>

#ifdef WTF_FOLLY_HACK
#include <folly/tracing/AsyncStack.h>

namespace folly {
// wtf
FOLLY_NOINLINE void
resumeCoroutineWithNewAsyncStackRoot(coro::coroutine_handle<> h,
                                     folly::AsyncStackFrame &frame) noexcept {
  detail::ScopedAsyncStackRoot root;
  root.activateFrame(frame);
  h.resume();
}
} // namespace folly
#endif

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

std::tuple<folly::File, std::string>
initLogging(const std::string &flag_log_dir) {
  folly::File original_stderr =
      folly::File{kStderrFileno, /*ownsFd=*/false}.dupCloseOnExec();
  std::string log_filename;

  if (!flag_log_dir.empty()) {
    log_filename = flag_log_dir + "/bapid.log";
    folly::File logHandle(
        log_filename, O_APPEND | O_CREAT | O_WRONLY | O_CLOEXEC, kLogFilePerms);
    dup2(logHandle.fd(), kStdoutFileno);
    dup2(logHandle.fd(), kStderrFileno);
  }

  return {std::move(original_stderr), std::move(log_filename)};
}
} // namespace

int main(int argc, char **argv) {
  folly::init(&argc, &argv);
  auto [original_stderr, log_filename] = initLogging(FLAGS_log_dir);

  bapid::Bapid service{bapid::Bapid::Config{
      .rpc_addr = "localhost:50051",
      .http_addr = "localhost:8000",
  }};

  service.start(folly::makeSemiFutureWith(
      [&, original_stderr = std::move(original_stderr),
       log_filename = std::move(log_filename)]() mutable {
        writeMessage(
            original_stderr,
            fmt::format("http at {:s}; rpc at {:s}; log at {:s}",
                        service.getConfig().http_addr,
                        service.getConfig().rpc_addr,
                        log_filename.empty() ? "terminal" : log_filename));
        original_stderr.close();
        XLOG(INFO) << "init";
      }));
  return 0;
}
