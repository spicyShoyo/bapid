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

void BapidServer::serve() {
  new CallData(&service_, cq_.get());

  void *tag{};
  bool ok{false};
  while (true) {
    cq_->Next(&tag, &ok);
    static_cast<CallData *>(tag)->proceed();
  }
}

BapidServer::CallData::CallData(BapidService::AsyncService *service,
                                grpc::ServerCompletionQueue *cq)
    : service_{service}, cq_{cq}, responder_(&ctx_) {
  service_->RequestPing(&ctx_, &request_, &responder_, cq_, cq_, this);
}

void BapidServer::CallData::proceed() {
  if (!processed_) {
    new CallData(service_, cq_);

    reply_.set_message("hi: " + request_.name());

    processed_ = true;
    responder_.Finish(reply_, grpc::Status::OK, this);
    return;
  }

  delete this;
}

} // namespace bapid
