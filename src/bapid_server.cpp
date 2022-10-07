#include "src/bapid_server.h"
#include "if/bapid.grpc.pb.h"
#include <atomic>
#include <folly/executors/GlobalExecutor.h>
#include <folly/experimental/coro/Task.h>
#include <folly/logging/xlog.h>

namespace bapid {

BapidServer::BapidServer(std::string addr) : addr_{std::move(addr)} {
  grpc::ServerBuilder builder{};
  builder.AddListeningPort(addr_, grpc::InsecureServerCredentials());
  builder.RegisterService(&service_);
  cq_ = builder.AddCompletionQueue();
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

void BapidServer::serve() {
  BapidRuntimeCtx runtimeCtx{&service_, cq_.get(), executor_.get()};
  BapiHanlderCtx hanlderCtx{
      this,
  };
  PingHandler pingHandler{hanlderCtx};
  ShutdownHandler shutdownHandler{hanlderCtx};

  BapidServiceRuntime runtime{runtimeCtx};

  auto pingHandlerState = pingHandler.addToRuntime(runtimeCtx);
  auto ShutdownHandlerState = shutdownHandler.addToRuntime(runtimeCtx);
  runtime.serve();
}

BapidServer::~BapidServer() {
  XLOG(INFO) << "drain queue...";
  void *ignored_tag{};
  bool ignored_ok{};
  while (cq_->Next(&ignored_tag, &ignored_ok)) {
  }
  XLOG(INFO) << "shutdown complete";
}

folly::coro::Task<void> BapidServer::doShutdown() {
  XLOG(INFO) << "shutdown...";
  server_->Shutdown();
  cq_->Shutdown();
  co_return;
}

void BapidServer::initiateShutdown() {
  shutdownRequested_.store(true);
  doShutdown().scheduleOn(getEexecutor()).start();
}

} // namespace bapid
