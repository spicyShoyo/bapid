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

#include <grpcpp/completion_queue.h>
#include <grpcpp/grpcpp.h>
#include <memory>
#include <tuple>
#include <type_traits>

namespace bapid {

class BapidServer;
using BapidRuntimeCtx = RuntimeCtxBase<BapidService>;
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
  static folly::coro::Task<void> process(CallData *data, BapiHanlderCtx &ctx);
};

class ShutdownHandler
    : public BapidHanlder<ShutdownHandler,
                          &BapidService::AsyncService::RequestShutdown> {
  friend HandlerBase;
  using HandlerBase::HandlerBase;
  static folly::coro::Task<void> process(CallData *data, BapiHanlderCtx &ctx);
};

class BapidServer final {
public:
  explicit BapidServer(std::string addr, int num_threads = 2);

  ~BapidServer();
  BapidServer(BapidServer &&other) noexcept = delete;
  BapidServer &operator=(BapidServer &&other) noexcept = delete;
  BapidServer &operator=(const BapidServer &other) = delete;
  BapidServer(const BapidServer &other) = delete;

  void serve();
  void initiateShutdown();

private:
  folly::CancellationToken startRuntimes();
  void initHandlers();

  int num_threads_;
  folly::EventBase *evb_;
  folly::Executor::KeepAlive<> executor_ = folly::getGlobalCPUExecutor();
  const std::string addr_;
  BapidService::AsyncService service_{};
  std::vector<std::unique_ptr<grpc::ServerCompletionQueue>> cqs_{};
  std::unique_ptr<grpc::Server> server_;

  std::vector<std::unique_ptr<IHanlder<BapidService>>> hanlders_;
  std::vector<std::unique_ptr<BapidServiceRuntime>> runtimes_;
  std::vector<std::thread> threads_;
};
} // namespace bapid
