#pragma once

#include "if/bapid.grpc.pb.h"
#include "src/common/service_runtime.h"
#include <folly/CancellationToken.h>
#include <folly/Unit.h>
#include <folly/executors/GlobalExecutor.h>
#include <folly/experimental/coro/Task.h>
#include <folly/io/async/EventBase.h>
#include <folly/logging/xlog.h>
#include <grpc/support/log.h>

#include <functional>
#include <grpcpp/completion_queue.h>
#include <grpcpp/grpcpp.h>
#include <memory>
#include <tuple>
#include <type_traits>

namespace bapid {

class BapidServer;
struct BapidHandlerCtx {
  BapidServer *server;
};

struct BapidHandlers;
using BapidHanlderRegistry =
    RpcHanlderRegistry<BapidService, BapidHandlerCtx, BapidHandlers>;

class BapidServer final {
public:
  explicit BapidServer(std::string addr, int numThreads = 2);

  ~BapidServer();
  BapidServer(BapidServer &&other) noexcept = delete;
  BapidServer &operator=(BapidServer &&other) noexcept = delete;
  BapidServer &operator=(const BapidServer &other) = delete;
  BapidServer(const BapidServer &other) = delete;

  void serve(folly::SemiFuture<folly::Unit> &&on_serve);
  void initiateShutdown();

private:
  void registerService(grpc::ServerBuilder &builder);
  void initRegistry();
  std::unique_ptr<IRpcServiceRuntime>
  buildRuntime(grpc::ServerCompletionQueue *cq);

  folly::CancellationToken startRuntimes();

  const std::string addr_;
  const int numThreads_;
  folly::EventBase *evb_;
  folly::Executor::KeepAlive<> executor_ = folly::getGlobalCPUExecutor();

  std::vector<std::unique_ptr<grpc::ServerCompletionQueue>> cqs_{};
  std::unique_ptr<grpc::Server> server_;
  std::unique_ptr<IRpcHanlderRegistry> registry_;
  std::vector<std::unique_ptr<IRpcServiceRuntime>> runtimes_;
  std::vector<std::thread> threads_;

  BapidService::AsyncService service_{};
};
} // namespace bapid
