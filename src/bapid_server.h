#pragma once

#include "if/bapid.grpc.pb.h"
#include <grpc/support/log.h>
#include <grpcpp/completion_queue.h>
#include <grpcpp/grpcpp.h>
#include <memory>

namespace bapid {

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

  class CallData {
  public:
    CallData(BapidService::AsyncService *service,
             grpc::ServerCompletionQueue *cq);
    void proceed();

  private:
    BapidService::AsyncService *service_;
    grpc::ServerCompletionQueue *cq_;
    grpc::ServerContext ctx_{};
    bapid::PingRequest request_{};
    bapid::PingReply reply_{};
    grpc::ServerAsyncResponseWriter<PingReply> responder_;
    bool processed_{false};
  };
  ;
};
} // namespace bapid
