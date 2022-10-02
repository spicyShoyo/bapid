#pragma once

#include "if/bapid.grpc.pb.h"
#include <grpc/support/log.h>
#include <grpcpp/completion_queue.h>
#include <grpcpp/grpcpp.h>
#include <memory>

namespace bapid {
template <typename THandler, typename TRequest, typename TReply,
          auto TRegisterFn>
class CallDataBase {
public:
  using type = CallDataBase<THandler, TRequest, TReply, TRegisterFn>;

  CallDataBase(BapidService::AsyncService *service,
               grpc::ServerCompletionQueue *cq)
      : service_{service}, cq_{cq}, responder_(&ctx_) {
    (service_->*TRegisterFn)(&ctx_, &request_, &responder_, cq_, cq_,
                             static_cast<THandler *>(this));
  }

  void proceed() {
    if (!processed_) {
      new CallDataBase<THandler, TRequest, TReply, TRegisterFn>(service_, cq_);
      static_cast<THandler *>(this)->process();
      processed_ = true;
      responder_.Finish(reply_, grpc::Status::OK,
                        static_cast<THandler *>(this));
    } else {
      delete static_cast<THandler *>(this);
    }
  }

private:
  friend THandler;
  BapidService::AsyncService *service_;
  grpc::ServerCompletionQueue *cq_;
  grpc::ServerContext ctx_;

  TRequest request_;
  TReply reply_;

  grpc::ServerAsyncResponseWriter<PingReply> responder_;
  bool processed_{false};
};

class PingHandler
    : public CallDataBase<PingHandler, PingRequest, PingReply,
                          &BapidService::AsyncService::RequestPing> {
public:
  void process();
};

class BapidServer final {
public:
  explicit BapidServer(std::string addr);
  ~BapidServer();

  BapidServer(BapidServer &&other) noexcept = delete;
  BapidServer &operator=(BapidServer &&other) noexcept = delete;
  BapidServer &operator=(const BapidServer &other) = delete;
  BapidServer(const BapidServer &other) = delete;

  void serve();

private:
  std::atomic<bool> shutdownRequested_{false};
  const std::string addr_;
  BapidService::AsyncService service_{};
  std::unique_ptr<grpc::ServerCompletionQueue> cq_;
  std::unique_ptr<grpc::Server> server_;
  ;
};
} // namespace bapid
