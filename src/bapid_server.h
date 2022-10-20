#pragma once

#include "if/bapid.grpc.pb.h"
#include "src/common/rpc_runtime.h"
#include "src/common/rpc_server.h"
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

class BapidServer : public RpcServerBase {
public:
  explicit BapidServer(std::string addr, int numThreads = 2);

private:
  std::unique_ptr<IRpcServiceRuntime>
  buildRuntime(grpc::ServerCompletionQueue *cq) override;
  void initRegistry();

  BapidService::AsyncService service_{};
};
} // namespace bapid
