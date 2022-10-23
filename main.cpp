#include "src/bapid_server.h"
#include <fmt/core.h>
#include <folly/File.h>
#include <folly/FileUtil.h>
#include <folly/init/Init.h>
#include <folly/logging/xlog.h>
#include <kj/compat/http.h>
#include <string_view>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

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

constexpr unsigned int kHttpOk = 200;
constexpr kj::StringPtr kHttpOkStr = "OK"_kj;
} // namespace

void test() {
  class Service final : public kj::HttpService,
                        public kj::TaskSet::ErrorHandler {

  public:
    explicit Service(kj::HttpHeaderTable &table) : table_{table} {}
    kj::Promise<void> request(kj::HttpMethod method, kj::StringPtr url,
                              const kj::HttpHeaders &headers,
                              kj::AsyncInputStream &requestBody,
                              Response &response) override {
      auto out = response.send(kHttpOk, kHttpOkStr, kj::HttpHeaders(table_));
      auto msg = "hi"_kj;
      return out->write(msg.begin(), msg.size()).attach(kj::mv(out));
    }

    kj::Promise<void> listenHttp(kj::Timer &timer,
                                 kj::Own<kj::ConnectionReceiver> &&listener) {
      kj::Own<kj::HttpServer> server =
          kj::heap<kj::HttpServer>(timer, table_, *this);
      return server->listenHttp(*listener)
          .attach(std::move(listener))
          .attach(std::move(server));
    }

  private:
    void taskFailed(kj::Exception &&exception) override {
      XLOG(INFO) << exception.getDescription().cStr();
    }

    kj::HttpHeaderTable &table_;
  };
  kj::HttpHeaderTable::Builder builder{};
  auto table = builder.build();
  Service service{*table};

  auto io = kj::setupAsyncIo();
  auto tasks = kj::heap<kj::TaskSet>(service);

  tasks->add(io.provider->getNetwork()
                 .parseAddress("localhost:8000")
                 .then([&](kj::Own<kj::NetworkAddress> &&addr) {
                   return service.listenHttp(io.provider->getTimer(),
                                             addr->listen());
                 }));
  int fd[2]; // NOLINT
  pipe(fd);
  std::thread s([&]() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    write(fd[1], ".", sizeof(char)); // NOLINT
  });
  kj::evalLater([&]() {
    static char _{};
    auto shutdown = io.lowLevelProvider->wrapInputFd(fd[0]); // NOLINT
    return shutdown->read(&_, sizeof(char)).attach(std::move(shutdown));
  }).wait(io.waitScope);
  s.join();
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
