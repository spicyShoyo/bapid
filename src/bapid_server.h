#pragma once

#include "if/bapid.grpc.pb.h"
#include "src/common/service_runtime.hpp"
#include <folly/Unit.h>
#include <folly/executors/GlobalExecutor.h>
#include <folly/experimental/coro/Task.h>
#include <folly/logging/xlog.h>
#include <grpc/support/log.h>

#include <grpcpp/completion_queue.h>
#include <grpcpp/grpcpp.h>
#include <memory>
#include <tuple>
#include <type_traits>

namespace bapid {

class BapidServer;
using BapidServiceCtx = ServiceCtxBase<BapidService>;
struct BapiHanlderCtx {
  BapidServer *server;
};
using BapidServiceRuntime = ServiceRuntimeBase<BapidService>;
template <typename THandler, auto TRegisterFn>
using BapidHanlder = HandlerBase<BapidServer, BapidService, BapiHanlderCtx,
                                 THandler, TRegisterFn>;

class PingHandler
    : public BapidHanlder<PingHandler,
                          &BapidService::AsyncService::RequestPing> {
  friend HandlerBase;
  using HandlerBase::HandlerBase;
  folly::coro::Task<void> process(CallData *data, BapiHanlderCtx &ctx);
};

class ShutdownHandler
    : public BapidHanlder<ShutdownHandler,
                          &BapidService::AsyncService::RequestShutdown> {
  friend HandlerBase;
  using HandlerBase::HandlerBase;
  folly::coro::Task<void> process(CallData *data, BapiHanlderCtx &ctx);
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
  void initiateShutdown();
  folly::Executor *getEexecutor() { return executor_.get(); }

private:
  folly::Executor::KeepAlive<> executor_ = folly::getGlobalCPUExecutor();
  folly::coro::Task<void> doShutdown();

  std::atomic<bool> shutdownRequested_{false};
  const std::string addr_;
  BapidService::AsyncService service_{};
  std::unique_ptr<grpc::ServerCompletionQueue> cq_;
  std::unique_ptr<grpc::Server> server_;
};
} // namespace bapid
