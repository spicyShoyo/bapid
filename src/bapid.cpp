#include "src/bapid.h"

namespace bapid {

Bapid::Bapid(const Config &config)
    : evb_{folly::EventBaseManager::get()->getEventBase()},
      rpc_{config.rpc_addr, config.rpc_num_threads, evb_},
      http_{config.http_addr} {}

void Bapid::start(folly::SemiFuture<folly::Unit> &&on_serve) {
  auto rpc_fut = rpc_.start();
  auto http_fut = http_.start();

  auto do_shutdown =
      rpc_.getShutdownRequestedFut()
          .deferValue([this](auto &&) {
            http_.shutdown();
            rpc_.initiateShutdown();
          })
          .deferValue([this, rpc_fut = std::move(rpc_fut),
                       http_fut = std::move(http_fut)](auto &&) mutable {
            return folly::collectAll(rpc_fut, http_fut)
                .deferValue([this](auto &&) {
                  evb_->runInEventBaseThread(
                      [=] { evb_->terminateLoopSoon(); });
                });
          })
          .via(evb_);

  std::move(on_serve).via(evb_);
  evb_->loopForever();
}
} // namespace bapid
