#pragma once

#include <grpc++/grpc++.h>
#include <grpcpp/completion_queue.h>
#include <memory>

namespace bapid {
using grpc::Server;
using grpc::ServerCompletionQueue;

class BapidServer final {
public:
  BapidServer();
  ~BapidServer();

  BapidServer(BapidServer &&other) noexcept;
  BapidServer &operator=(BapidServer &&other) noexcept = delete;
  BapidServer &operator=(const BapidServer &other) = delete;
  BapidServer(const BapidServer &other) = delete;

private:
  std::unique_ptr<ServerCompletionQueue> cq_;
  std::unique_ptr<Server> server_;
};
} // namespace bapid
