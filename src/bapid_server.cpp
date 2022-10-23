#include "src/bapid_server.h"
#include "if/bapid.grpc.pb.h"
#include "src/common/rpc_runtime.h"
#include "src/common/rpc_server.h"
#include <atomic>
#include <chrono>
#include <folly/executors/GlobalExecutor.h>
#include <folly/experimental/coro/Task.h>
#include <folly/io/async/EventBaseManager.h>
#include <folly/logging/xlog.h>
#include <initializer_list>
#include <memory>

namespace bapid {

folly::coro::Task<void>
BapidHandlers::ping(bapidrpc::PingReply &reply,
                    const bapidrpc::PingRequest &request,
                    BapidHandlerCtx &ctx) {
  reply.set_message("hi: " + request.name());
  co_return;
}

folly::coro::Task<void> BapidHandlers::shutdown(bapidrpc::Empty &reply,
                                                const bapidrpc::Empty &reuqest,
                                                BapidHandlerCtx &ctx) {
  ctx.server->initiateShutdown();
  co_return;
};

BapidServer::BapidServer(std::string addr, int numThreads)
    : RpcServerBase(std::move(addr), numThreads) {
  using bapidrpc::BapidService;

  auto service = std::make_unique<BapidService::AsyncService>();
  auto registry = std::make_unique<
      RpcHanlderRegistry<BapidService, BapidHandlerCtx, BapidHandlers>>(
      service.get(), BapidHandlerCtx{this});

  registry->registerHandler<&BapidService::AsyncService::RequestPing>(
      &BapidHandlers::ping);
  registry->registerHandler<&BapidService::AsyncService::RequestShutdown>(
      &BapidHandlers::shutdown);

  initService(std::move(service), std::move(registry));
}

} // namespace bapid
