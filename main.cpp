#include "src/bapid_server.h"
#include <fmt/core.h>
#include <folly/File.h>
#include <folly/FileUtil.h>
#include <folly/init/Init.h>
#include <folly/logging/xlog.h>
#include <kj/compat/http.h>
#include <string_view>

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
} // namespace

void test() {
  class Service final : public kj::HttpService {
  public:
    explicit Service(kj::HttpHeaderTable &table) : table{table} {}
    kj::Promise<void> request(kj::HttpMethod method, kj::StringPtr url,
                              const kj::HttpHeaders &headers,
                              kj::AsyncInputStream &requestBody,
                              Response &response) override {
      auto out = response.send(200, "OK"_kj, kj::HttpHeaders(table));
      auto msg = "hello there"_kj;
      return out->write(msg.begin(), 11).attach(kj::mv(out));
    }

    kj::HttpHeaderTable &table;
  };
  kj::HttpHeaderTable::Builder builder{};
  kj::TimerImpl timer{kj::origin<kj::TimePoint>()};
  auto table = builder.build();
  Service service{*table};
  kj::HttpServer server{timer, *table, service};
  auto io = kj::setupAsyncIo();
  auto listener = io.provider->getNetwork()
                      .parseAddress("localhost", 8000)
                      .wait(io.waitScope)
                      ->listen();
  // server.listenHttp(*listener);
}

int main(int argc, char **argv) {
  folly::init(&argc, &argv);

  std::string rpc_addr = "localhost:50051";
  std::string log_filename;

  folly::File original_stderr =
      folly::File{kStderrFileno, /*ownsFd=*/false}.dupCloseOnExec();
  if (!FLAGS_log_dir.empty()) {
    log_filename = FLAGS_log_dir + "/bapid.log";
    folly::File logHandle(
        log_filename, O_APPEND | O_CREAT | O_WRONLY | O_CLOEXEC, kLogFilePerms);
    dup2(logHandle.fd(), kStdoutFileno);
    dup2(logHandle.fd(), kStderrFileno);
  }

  bapid::BapidServer server{rpc_addr};
  server.serve(folly::makeSemiFutureWith([&, original_stderr = std::move(
                                                 original_stderr)]() mutable {
    writeMessage(original_stderr,
                 fmt::format("serving at {:s}; log at {:s}", rpc_addr,
                             log_filename.empty() ? "terminal" : log_filename));
    original_stderr.close();
    XLOG(INFO) << "init";
  }));
  return 0;
}
