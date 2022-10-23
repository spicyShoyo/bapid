#pragma once

#include <folly/logging/xlog.h>
#include <kj/compat/http.h>
#include <unistd.h>

namespace bapid {
constexpr unsigned int kHttpOk = 200;
constexpr kj::StringPtr kHttpOkStr = "OK"_kj;

class BapidHttpServer final : public kj::HttpService,
                              public kj::TaskSet::ErrorHandler {

public:
  explicit BapidHttpServer(kj::StringPtr addr)
      : addr_{addr}, table_{kj::HttpHeaderTable::Builder{}.build()} {}

  kj::Promise<void> request(kj::HttpMethod method, kj::StringPtr url,
                            const kj::HttpHeaders &headers,
                            kj::AsyncInputStream &requestBody,
                            Response &response) override {
    auto out = response.send(kHttpOk, kHttpOkStr, kj::HttpHeaders(*table_));
    auto msg = "hi"_kj;
    return out->write(msg.begin(), msg.size()).attach(kj::mv(out));
  }

  void start() {
    int fd[2]; // NOLINT
    pipe(fd);
    shutdown_in_ = fd[1];
    server_thread_ =
        std::thread{[shutdown_out = fd[0], this]() { serve(shutdown_out); }};
  }

  void shutdown() {
    write(shutdown_in_, ".", sizeof(char));
    server_thread_.join();
  }

  kj::Promise<void> listenHttp(kj::Timer &timer,
                               kj::Own<kj::ConnectionReceiver> &&listener) {
    kj::Own<kj::HttpServer> server =
        kj::heap<kj::HttpServer>(timer, *table_, *this);
    return server->listenHttp(*listener)
        .attach(std::move(listener))
        .attach(std::move(server));
  }

private:
  void serve(int shutdown_out) {
    auto io = kj::setupAsyncIo();
    auto tasks = kj::heap<kj::TaskSet>(*this);

    tasks->add(io.provider->getNetwork().parseAddress(addr_).then(
        [&](kj::Own<kj::NetworkAddress> &&addr) {
          return listenHttp(io.provider->getTimer(), addr->listen());
        }));

    kj::evalLater([&]() {
      static char _{};
      auto shutdown = io.lowLevelProvider->wrapInputFd(shutdown_out);
      return shutdown->read(&_, sizeof(char)).attach(std::move(shutdown));
    }).wait(io.waitScope);
  }

  void taskFailed(kj::Exception &&exception) override {
    XLOG(WARN) << exception.getDescription().cStr();
  }

  kj::StringPtr addr_;
  kj::Own<kj::HttpHeaderTable> table_;
  int shutdown_in_{};
  std::thread server_thread_{};
};
} // namespace bapid
