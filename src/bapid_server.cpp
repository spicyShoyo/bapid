#include "src/bapid_server.h"
#include "if/bapid.grpc.pb.h"
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

  initHandlers();
}

void BapidServer::initHandlers() {
  BapiHanlderCtx hanlder_ctx{
      this,
  };

  hanlder_registry_ = std::make_unique<BapidHanlderRegistry>(hanlder_ctx);

  BapidHanlderRegistry::Hanlder<bapid::PingRequest, bapid::PingReply>
      pingHandler = [](bapid::PingReply &reply,
                       const bapid::PingRequest &request,
                       BapiHanlderCtx &ctx) -> folly::coro::Task<void> {
    reply.set_message("hi: " + request.name());
    co_return;
  };
  hanlder_registry_->registerHandler<&BapidService::AsyncService::RequestPing>(
      std::move(pingHandler));

  BapidHanlderRegistry::Hanlder<bapid::Empty, bapid::Empty> shutdownHanlder =
      [](bapid::Empty &reply, const bapid::Empty &reuqest,
         BapiHanlderCtx &ctx) -> folly::coro::Task<void> {
    ctx.server->initiateShutdown();
    co_return;
  };
  hanlder_registry_
      ->registerHandler<&BapidService::AsyncService::RequestShutdown>(
          std::move(shutdownHanlder));
}

folly::CancellationToken BapidServer::startRuntimes() {
  folly::CancellationSource source;
  auto token = source.getToken();
  auto guard = folly::copy_to_shared_ptr(folly::makeGuard(
      [source = std::move(source)]() { source.requestCancellation(); }));

  BapidServiceRuntime::BindRegistryFn bind_registry =
      [&](BapidRuntimeCtx &runtime_ctx) {
        return hanlder_registry_->bindRuntime(runtime_ctx);
      };

  for (int i = 0; i < numThreads_; i++) {
    runtimes_.emplace_back(std::make_unique<BapidServiceRuntime>(
        BapidRuntimeCtx{&service_, cqs_[i].get(), executor_.get()},
        bind_registry));
    threads_.emplace_back(
        [runtime = runtimes_.back().get(), guard = guard]() mutable {
          runtime->serve();
          guard.reset();
        });
  }

  return token;
}

void BapidServer::serve() {
  auto token = startRuntimes();

  folly::CancellationCallback cb(token, [=] {
    evb_->runInEventBaseThread([=] { evb_->terminateLoopSoon(); });
  });
  evb_->loopForever();

  for (auto &thread : threads_) {
    thread.join();
  }
  threads_.clear();
}

BapidServer::~BapidServer() {
  XLOG(INFO) << "draining...";
  runtimes_.clear();
  XLOG(INFO) << "shutdown complete";
}

void BapidServer::initiateShutdown() {
  evb_->runInEventBaseThread([this] {
    XLOG(INFO) << "shutdown..."; // NOLINT
    server_->Shutdown(std::chrono::system_clock::now() + kShutdownWait);
    for (auto &cq : cqs_) {
      cq->Shutdown();
    }
  });
}
} // namespace bapid
