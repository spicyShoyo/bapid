#include "src/bapid_server.h"
#include "if/bapid.grpc.pb.h"
#include "src/common/service_runtime.h"
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

struct BapidHandlers {
  folly::coro::Task<void> ping(bapid::PingReply &reply,
                               const bapid::PingRequest &request,
                               BapidHandlerCtx &ctx) {
    reply.set_message("hi: " + request.name());
    co_return;
  }

  folly::coro::Task<void> shutdown(bapid::Empty &reply,
                                   const bapid::Empty &reuqest,
                                   BapidHandlerCtx &ctx) {
    ctx.server->initiateShutdown();
    co_return;
  };
};

BapidServer::BapidServer(std::string addr, int numThreads)
    : addr_{std::move(addr)}, numThreads_{numThreads},
      evb_{folly::EventBaseManager::get()->getEventBase()} {
  XCHECK(numThreads > 0);

  grpc::ServerBuilder builder{};
  builder.AddListeningPort(addr_, grpc::InsecureServerCredentials());
  builder.RegisterService(&service_);
  for (int i = 0; i < numThreads; i++) {
    cqs_.emplace_back(builder.AddCompletionQueue());
  }
  server_ = builder.BuildAndStart();
}

folly::CancellationToken BapidServer::startRuntimes() {
  auto registry = std::make_unique<BapidHanlderRegistry>(BapidHandlerCtx{this});
  registry->registerHandler<&BapidService::AsyncService::RequestPing>(
      &BapidHandlers::ping);
  registry->registerHandler<&BapidService::AsyncService::RequestShutdown>(
      &BapidHandlers::shutdown);
  auto *registry_ptr = registry.get();

  folly::CancellationSource source;
  auto token = source.getToken();
  auto guard = folly::copy_to_shared_ptr(folly::makeGuard(
      [source = std::move(source), registry = std::move(registry)]() {
        source.requestCancellation();
      }));

  using ServiceRuntime = RpcServiceRuntime<BapidService>;
  using RuntimeCtx = RpcRuntimeCtx<BapidService>;

  ServiceRuntime::BindRegistryFn bind_registry = [&](RuntimeCtx &runtime_ctx) {
    return registry_ptr->bindRuntime(runtime_ctx);
  };

  for (int i = 0; i < numThreads_; i++) {
    auto rt = std::make_unique<ServiceRuntime>(
        RuntimeCtx{&service_, cqs_[i].get(), executor_.get()}, bind_registry);
    threads_.emplace_back([runtime = std::move(rt), guard = guard]() mutable {
      runtime->serve();
      runtime.reset(nullptr);
      guard.reset();
    });
  }

  return token;
}

void BapidServer::serve(folly::SemiFuture<folly::Unit> &&on_serve) {
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
}

BapidServer::~BapidServer() { XLOG(INFO) << "shutdown complete"; }

void BapidServer::initiateShutdown() {
  evb_->runInEventBaseThread([this] {
    XLOG(INFO) << "shutdown...";
    server_->Shutdown(std::chrono::system_clock::now() + kShutdownWait);
    for (auto &cq : cqs_) {
      cq->Shutdown();
    }
  });
}
} // namespace bapid
