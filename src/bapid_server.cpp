#include "src/bapid_server.h"
#include "if/bapid.grpc.pb.h"

namespace bapid {

BapidServer::BapidServer(std::string addr) : addr_{std::move(addr)} {
  grpc::ServerBuilder builder{};
  builder.AddListeningPort(addr_, grpc::InsecureServerCredentials());
  builder.RegisterService(&service_);
  cq_ = builder.AddCompletionQueue();
  server_ = builder.BuildAndStart();
}

BapidServer::~BapidServer() {
  server_->Shutdown();
  cq_->Shutdown();
}

void PingHandler::process() { reply_.set_message("hi: " + request_.name()); }

void BapidServer::serve() {
  new PingHandler::type(&service_, cq_.get());

  void *tag{};
  bool ok{false};
  while (true) {
    cq_->Next(&tag, &ok);
    static_cast<PingHandler::type *>(tag)->proceed();
  }
}

} // namespace bapid
