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

BapidServer::BapidServer(std::string addr, int num_threads)
    : addr_{std::move(addr)}, num_threads_{num_threads},
      evb_{folly::EventBaseManager::get()->getEventBase()} {
  XCHECK(num_threads > 0);

  grpc::ServerBuilder builder{};
  builder.AddListeningPort(addr_, grpc::InsecureServerCredentials());
  builder.RegisterService(&service_);
  for (int i = 0; i < num_threads; i++) {
    cqs_.emplace_back(builder.AddCompletionQueue());
  }
  server_ = builder.BuildAndStart();

  initHandlers();
}

void BapidServer::initHandlers() {
  BapiHanlderCtx handler_ctx{
      this,
  };
  hanlders_.emplace_back(std::make_unique<PingHandler>(handler_ctx));
  hanlders_.emplace_back(std::make_unique<ShutdownHandler>(handler_ctx));
}

/*static*/ folly::coro::Task<void> PingHandler::process(CallData *data,
                                                        BapiHanlderCtx &ctx) {
  data->reply.set_message("hi: " + data->request.name());
  co_return;
}

/*static*/ folly::coro::Task<void>
ShutdownHandler::process(CallData *data, BapiHanlderCtx &ctx) {
  ctx.server->initiateShutdown();
  co_return;
}

folly::CancellationToken BapidServer::startRuntimes() {
  folly::CancellationSource source;
  auto token = source.getToken();
  auto guard = folly::copy_to_shared_ptr(folly::makeGuard(
      [source = std::move(source)]() { source.requestCancellation(); }));

  for (int i = 0; i < num_threads_; i++) {
    BapidRuntimeCtx runtime_ctx{&service_, cqs_[i].get(), executor_.get()};
    runtimes_.emplace_back(
        std::make_unique<BapidServiceRuntime>(runtime_ctx, hanlders_));
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
