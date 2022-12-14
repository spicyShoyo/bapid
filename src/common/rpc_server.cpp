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

RpcServerBase::RpcServerBase(std::string addr, int num_threads)
    : RpcServerBase(std::move(addr), num_threads,
                    folly::EventBaseManager::get()->getEventBase()) {}

RpcServerBase::RpcServerBase(std::string addr, int num_threads,
                             folly::EventBase *evb)
    : addr_{std::move(addr)}, num_threads_{num_threads}, evb_{evb} {
  XCHECK(num_threads_ > 0);
}

void RpcServerBase::initService(std::unique_ptr<grpc::Service> service,
                                std::unique_ptr<IRpcHanlderRegistry> registry) {
  XCHECK(service);
  XCHECK(registry);

  service_ = std::move(service);
  registry_ = std::move(registry);

  grpc::ServerBuilder builder{};
  builder.AddListeningPort(addr_, grpc::InsecureServerCredentials());
  builder.RegisterService(service_.get());
  for (int i = 0; i < num_threads_; i++) {
    cqs_.emplace_back(builder.AddCompletionQueue());
  }
  server_ = builder.BuildAndStart();
}

folly::CancellationToken RpcServerBase::startRuntimes() {
  folly::CancellationSource source;
  auto token = source.getToken();
  auto guard = folly::copy_to_shared_ptr(folly::makeGuard(
      [source = std::move(source)]() { source.requestCancellation(); }));

  for (int i = 0; i < num_threads_; i++) {
    runtimes_.emplace_back(std::make_unique<RpcServiceRuntime>(
        RpcRuntimeCtx{cqs_[i].get(), executor_.get()}, registry_));
    threads_.emplace_back(
        [runtime = runtimes_.back().get(), guard = guard]() mutable {
          runtime->serve();
          guard.reset();
        });
  }

  return token;
}

folly::SemiFuture<folly::Unit> RpcServerBase::start() {
  auto token = startRuntimes();
  return folly::coro::co_withCancellation(
             std::move(token),
             folly::coro::co_invoke([this]() -> folly::coro::Task<void> {
               drain();
               co_return;
             }))
      .semi();
}

void RpcServerBase::serve(folly::SemiFuture<folly::Unit> &&on_serve) {
  auto token = startRuntimes();
  folly::CancellationCallback cb(std::move(token), [=] {
    evb_->runInEventBaseThread([=] { evb_->terminateLoopSoon(); });
  });

  std::move(on_serve).via(evb_);
  evb_->loopForever();
  drain();
}

void RpcServerBase::drain() {
  for (auto &thread : threads_) {
    thread.join();
  }

  threads_.clear();
  XLOG(INFO) << "draining...";
  runtimes_.clear();
}

RpcServerBase::~RpcServerBase() {
  XCHECK(threads_.empty());
  XLOG(INFO) << "rpc shutdown complete";
}

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
