#include "src/bapid_server.h"
#include "if/bapid.grpc.pb.h"
#include <atomic>
#include <folly/executors/GlobalExecutor.h>
#include <folly/experimental/coro/Task.h>
#include <folly/logging/xlog.h>
#include <folly/tracing/AsyncStack.h>

namespace folly {
// wtf
FOLLY_NOINLINE void
resumeCoroutineWithNewAsyncStackRoot(coro::coroutine_handle<> h,
                                     folly::AsyncStackFrame &frame) noexcept {
  detail::ScopedAsyncStackRoot root;
  root.activateFrame(frame);
  h.resume();
}
} // namespace folly

namespace bapid {

BapidServer::BapidServer(std::string addr) : addr_{std::move(addr)} {
  grpc::ServerBuilder builder{};
  builder.AddListeningPort(addr_, grpc::InsecureServerCredentials());
  builder.RegisterService(&service_);
  cq_ = builder.AddCompletionQueue();
  server_ = builder.BuildAndStart();
}

void PingHandler::process() { reply_.set_message("hi: " + request_.name()); }
void Ping2Handler::process() { reply_.set_message("hi2: " + request_.name()); }
void ShutdownHandler::process() { data_.server->initiateShutdown(); }

void BapidServer::serve() {
  CallData data{this, &service_, cq_.get()};
  new PingHandler::type(data);
  new Ping2Handler::type(data);
  new ShutdownHandler::type(data);

  void *tag{};
  bool ok{false};
  while (cq_->Next(&tag, &ok)) {
    if (!ok) {
      break;
    }

    (*static_cast<ProceedFn *>(tag))();
  }
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
  doShutdown().scheduleOn(folly::getGlobalCPUExecutor().get()).start();
}

} // namespace bapid
