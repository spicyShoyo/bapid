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

void RpcServerBase::initService(std::unique_ptr<grpc::Service> service,
                                std::unique_ptr<IRpcHanlderRegistry> registry) {
  service_ = std::move(service);
  registry_ = std::move(registry);

  grpc::ServerBuilder builder{};
  builder.AddListeningPort(addr_, grpc::InsecureServerCredentials());
  builder.RegisterService(service_.get());
  for (int i = 0; i < numThreads_; i++) {
    cqs_.emplace_back(builder.AddCompletionQueue());
  }
  server_ = builder.BuildAndStart();
}

folly::CancellationToken RpcServerBase::startRuntimes() {
  folly::CancellationSource source;
  auto token = source.getToken();
  auto guard = folly::copy_to_shared_ptr(folly::makeGuard(
      [source = std::move(source)]() { source.requestCancellation(); }));

  auto bind_registry = getBindRegistry();
  for (int i = 0; i < numThreads_; i++) {
    runtimes_.emplace_back(std::make_unique<RpcServiceRuntime>(
        RpcRuntimeCtx{service_.get(), cqs_[i].get(), executor_.get()},
        bind_registry));
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
