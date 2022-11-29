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

struct BapidHandlers;
class BapidServer : public RpcServerBase {
public:
  BapidServer(std::string addr, int num_threads, folly::EventBase *evb);
  folly::SemiFuture<folly::Unit> getShutdownRequestedFut();

private:
  friend BapidHandlers;
  void shutdownRequested();

  folly::Promise<folly::Unit> shutdown_requested_{};
};

struct BapidHandlerCtx {
  BapidServer *server;
};

struct BapidHandlers {
  folly::coro::Task<void> ping(bapidrpc::PingReply &reply,
                               const bapidrpc::PingRequest &request,
                               BapidHandlerCtx &ctx);

  folly::coro::Task<void> shutdown(bapidrpc::Empty &reply,
                                   const bapidrpc::Empty &reuqest,
                                   BapidHandlerCtx &ctx);

  folly::coro::Task<void> arrowTest(bapidrpc::Empty &reply,
                                    const bapidrpc::Empty &reuqest,
                                    BapidHandlerCtx &ctx);
};
} // namespace bapid
