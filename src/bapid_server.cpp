#include "src/bapid_server.h"
#include "if/bapid.grpc.pb.h"
#include <atomic>
#include <folly/logging/xlog.h>

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

  SCOPE_EXIT {
    XLOG(INFO) << "shutdown..."; // NOLINT
    server_->Shutdown();
    XLOG(INFO) << "shutdown server"; // NOLINT
    cq_->Shutdown();
    XLOG(INFO) << "shutdown finish"; // NOLINT
  };

  void *tag{};
  bool ok{false};
  while (cq_->Next(&tag, &ok)) {
    if (!ok) {
      break;
    }

    (*static_cast<ProceedFn *>(tag))();

    if (shutdownRequested_.load(std::memory_order_relaxed)) {
      break;
    }
  }
}

void BapidServer::initiateShutdown() { shutdownRequested_.store(true); }

} // namespace bapid
