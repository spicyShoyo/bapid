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
namespace {
constexpr std::chrono::milliseconds kShutdownWait =
    std::chrono::milliseconds(200);
};

struct BapidHandlers {
  folly::coro::Task<void> ping(bapid::PingReply &reply,
                               const bapid::PingRequest &request,
                               BapidHandlerCtx &ctx) {
    reply.set_message("hi: " + request.name());
    co_return;
  }

  folly::coro::Task<void> shutdown(bapid::Empty &reply,
                                   const bapid::Empty &reuqest,
                                   BapidHandlerCtx &ctx) {
    ctx.server->initiateShutdown();
    co_return;
  };
};

BapidServer::BapidServer(std::string addr, int numThreads)
    : RpcServerBase(std::move(addr), numThreads) {
  grpc::ServerBuilder builder{};
  builder.AddListeningPort(addr_, grpc::InsecureServerCredentials());
  builder.RegisterService(&service_);
  for (int i = 0; i < numThreads; i++) {
    cqs_.emplace_back(builder.AddCompletionQueue());
  }
  server_ = builder.BuildAndStart();
  initRegistry();
}

void BapidServer::initRegistry() {
  auto registry = std::make_unique<BapidHanlderRegistry>(BapidHandlerCtx{this});
  registry->registerHandler<&BapidService::AsyncService::RequestPing>(
      &BapidHandlers::ping);
  registry->registerHandler<&BapidService::AsyncService::RequestShutdown>(
      &BapidHandlers::shutdown);
  registry_ = std::move(registry);
}

std::unique_ptr<IRpcServiceRuntime>
BapidServer::buildRuntime(grpc::ServerCompletionQueue *cq) {
  auto *registry = dynamic_cast<BapidHanlderRegistry *>(registry_.get());

  using ServiceRuntime = RpcServiceRuntime<BapidService>;
  using RuntimeCtx = RpcRuntimeCtx<BapidService>;

  ServiceRuntime::BindRegistryFn bind_registry = [&](RuntimeCtx &runtime_ctx) {
    return registry->bindRuntime(runtime_ctx);
  };

  return std::make_unique<ServiceRuntime>(
      RuntimeCtx{&service_, cq, executor_.get()}, bind_registry);
}

} // namespace bapid
