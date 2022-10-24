#include "src/http_server.h"

namespace bapid {
namespace {
constexpr unsigned int kHttpOk = 200;
constexpr kj::StringPtr kHttpOkStr = "OK"_kj;
} // namespace
  //
BapidHttpServer::BapidHttpServer(std::string addr)
    : addr_{std::move(addr)}, table_{kj::HttpHeaderTable::Builder{}.build()} {}

kj::Promise<void> BapidHttpServer::request(kj::HttpMethod method,
                                           kj::StringPtr url,
                                           const kj::HttpHeaders &headers,
                                           kj::AsyncInputStream &requestBody,
                                           Response &response) {
  auto out = response.send(kHttpOk, kHttpOkStr, kj::HttpHeaders(*table_));
  auto msg = "hi"_kj;
  return out->write(msg.begin(), msg.size()).attach(kj::mv(out));
}

void BapidHttpServer::start() {
  int fd[2]; // NOLINT
  pipe(fd);
  shutdown_in_ = fd[1];
  server_thread_ =
      std::thread{[shutdown_out = fd[0], this]() { serve(shutdown_out); }};
}

void BapidHttpServer::shutdown() {
  write(shutdown_in_, ".", sizeof(char));
  server_thread_.join();
}

void BapidHttpServer::serve(int shutdown_out) {
  auto io = kj::setupAsyncIo();
  auto tasks = kj::heap<kj::TaskSet>(*this);

  tasks->add(io.provider->getNetwork().parseAddress(addr_).then(
      [&](kj::Own<kj::NetworkAddress> &&addr) {
        kj::Own<kj::HttpServer> server =
            kj::heap<kj::HttpServer>(io.provider->getTimer(), *table_, *this);
        auto listener = addr->listen();

        return server->listenHttp(*listener)
            .attach(std::move(listener))
            .attach(std::move(server));
      }));

  kj::evalLater([&]() {
    static char _{};
    auto shutdown = io.lowLevelProvider->wrapInputFd(shutdown_out);
    return shutdown->read(&_, sizeof(char)).attach(std::move(shutdown));
  }).wait(io.waitScope);
}

void BapidHttpServer::taskFailed(kj::Exception &&exception) {
  XLOG(WARN) << exception.getDescription().cStr();
}

} // namespace bapid
