#include "src/common/rpc_server.h"
#include "src/common/rpc_runtime.h"
#include <atomic>
#include <chrono>
#include <folly/executors/GlobalExecutor.h>
#include <folly/experimental/coro/Task.h>
#include <folly/io/async/EventBaseManager.h>
#include <folly/logging/xlog.h>
#include <initializer_list>
#include <memory>

namespace bapid {
namespace {
constexpr std::chrono::milliseconds kShutdownWait =
    std::chrono::milliseconds(200);
};

RpcServerBase::RpcServerBase(std::string addr, int numThreads)
    : addr_{std::move(addr)}, numThreads_{numThreads},
      evb_{folly::EventBaseManager::get()->getEventBase()} {
  XCHECK(numThreads > 0);
}

folly::CancellationToken RpcServerBase::startRuntimes() {
  folly::CancellationSource source;
  auto token = source.getToken();
  auto guard = folly::copy_to_shared_ptr(folly::makeGuard(
      [source = std::move(source)]() { source.requestCancellation(); }));

  for (int i = 0; i < numThreads_; i++) {
    runtimes_.emplace_back(buildRuntime(cqs_[i].get()));
    threads_.emplace_back(
        [runtime = runtimes_.back().get(), guard = guard]() mutable {
          runtime->serve();
          guard.reset();
        });
  }

  return token;
}

void RpcServerBase::serve(folly::SemiFuture<folly::Unit> &&on_serve) {
  auto token = startRuntimes();

  folly::CancellationCallback cb(token, [=] {
    evb_->runInEventBaseThread([=] { evb_->terminateLoopSoon(); });
  });
  std::move(on_serve).via(evb_);
  evb_->loopForever();

  for (auto &thread : threads_) {
    thread.join();
  }

  threads_.clear();
  XLOG(INFO) << "draining...";
  runtimes_.clear();
  server_.reset(nullptr);
  cqs_.clear();
}

RpcServerBase::~RpcServerBase() { XLOG(INFO) << "shutdown complete"; }

void RpcServerBase::initiateShutdown() {
  evb_->runInEventBaseThread([this] {
    XLOG(INFO) << "shutdown...";
    server_->Shutdown(std::chrono::system_clock::now() + kShutdownWait);
    for (auto &cq : cqs_) {
      cq->Shutdown();
    }
  });
}
} // namespace bapid
