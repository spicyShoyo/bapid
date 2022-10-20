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
  using ServiceRuntime = RpcServiceRuntime<BapidService>;
  using RuntimeCtx = RpcRuntimeCtx<BapidService>;

  explicit BapidServer(std::string addr, int numThreads = 2);

  ~BapidServer();
  BapidServer(BapidServer &&other) noexcept = delete;
  BapidServer &operator=(BapidServer &&other) noexcept = delete;
  BapidServer &operator=(const BapidServer &other) = delete;
  BapidServer(const BapidServer &other) = delete;

  void serve(folly::SemiFuture<folly::Unit> &&on_serve);
  void initiateShutdown();

private:
  folly::CancellationToken startRuntimes();
  void initHandlers();

  int numThreads_;
  folly::EventBase *evb_;
  folly::Executor::KeepAlive<> executor_ = folly::getGlobalCPUExecutor();
  const std::string addr_;
  BapidService::AsyncService service_{};
  std::vector<std::unique_ptr<grpc::ServerCompletionQueue>> cqs_{};
  std::unique_ptr<grpc::Server> server_;

  std::unique_ptr<BapidHanlderRegistry> hanlder_registry_;
  std::vector<std::unique_ptr<ServiceRuntime>> runtimes_;
  std::vector<std::thread> threads_;
};
} // namespace bapid
