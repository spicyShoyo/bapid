#include "src/bapid.h"

namespace bapid {

Bapid::Bapid(Config config)
    : config_{std::move(config)}, rpc_{config_.rpc_addr,
                                       config_.rpc_num_threads},
      http_{config_.http_addr} {}

void Bapid::start(folly::SemiFuture<folly::Unit> &&on_serve) {
  http_.start();
  rpc_.serve(std::move(on_serve));
  http_.shutdown();
}

const Bapid::Config &Bapid::getConfig() { return config_; }
} // namespace bapid
