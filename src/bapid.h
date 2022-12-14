#pragma once

#include <folly/io/async/EventBaseManager.h>

#include "src/bapid_server.h"
#include "src/http_server.h"
#include <string>

namespace bapid {

class Bapid {
public:
  struct Config {
    std::string rpc_addr;
    std::string http_addr;
    int rpc_num_threads;
  };

  explicit Bapid(const Config &config);
  void start(folly::SemiFuture<folly::Unit> &&on_serve);
  void shutdown();

private:
  folly::EventBase *evb_;
  BapidServer rpc_;
  BapidHttpServer http_;
};
} // namespace bapid
