#include "src/bapid_server.h"
#include "if/bapid.grpc.pb.h"
#include <atomic>
#include <folly/executors/GlobalExecutor.h>
#include <folly/experimental/coro/Task.h>
#include <folly/io/async/EventBaseManager.h>
#include <folly/logging/xlog.h>
#include <initializer_list>
#include <memory>

namespace bapid {

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
  BapiHanlderCtx hanlderCtx{
      this,
  };
  hanlders_.emplace_back(std::make_unique<PingHandler>(hanlderCtx));
  hanlders_.emplace_back(std::make_unique<ShutdownHandler>(hanlderCtx));

  folly::CancellationSource source;
  auto token = source.getToken();
  auto guard = folly::copy_to_shared_ptr(folly::makeGuard(
      [source = std::move(source)]() { source.requestCancellation(); }));

  for (int i = 0; i < numThreads_; i++) {
    BapidRuntimeCtx runtimeCtx{&service_, cqs_[i].get(), executor_.get()};
    runtimes_.emplace_back(
        std::make_unique<BapidServiceRuntime>(runtimeCtx, hanlders_));
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
}

BapidServer::~BapidServer() {
  XLOG(INFO) << "draining...";
  threads_.clear();
  runtimes_.clear();
  XLOG(INFO) << "shutdown complete";
}

folly::coro::Task<void> BapidServer::doShutdown() {
  XLOG(INFO) << "shutdown...";
  server_->Shutdown();
  for (auto &cq : cqs_) {
    cq->Shutdown();
  }

  co_return;
}
void BapidServer::initiateShutdown() {
  doShutdown().scheduleOn(executor_).start();
}

} // namespace bapid
