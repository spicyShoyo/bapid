#pragma once

#include "src/common/rpc_runtime.h"
#include <folly/CancellationToken.h>
#include <folly/Unit.h>
#include <folly/executors/GlobalExecutor.h>
#include <folly/experimental/coro/Task.h>
#include <folly/io/async/EventBase.h>
#include <folly/logging/xlog.h>
#include <grpc/support/log.h>
#include <grpcpp/impl/codegen/service_type.h>

#include <functional>
#include <grpcpp/completion_queue.h>
#include <grpcpp/grpcpp.h>
#include <memory>
#include <tuple>
#include <type_traits>

namespace bapid {

class RpcServerBase {
public:
  RpcServerBase(std::string addr, int num_threads);
  RpcServerBase(std::string addr, int num_threads, folly::EventBase *evb_);

  virtual ~RpcServerBase();
  RpcServerBase(RpcServerBase &&other) noexcept = delete;
  RpcServerBase &operator=(RpcServerBase &&other) noexcept = delete;
  RpcServerBase &operator=(const RpcServerBase &other) = delete;
  RpcServerBase(const RpcServerBase &other) = delete;

  folly::SemiFuture<folly::Unit> start();
  void serve(folly::SemiFuture<folly::Unit> &&on_serve);
  void initiateShutdown();

protected:
  void initService(std::unique_ptr<grpc::Service> service,
                   std::unique_ptr<IRpcHanlderRegistry> registry);
  void drain();

  folly::CancellationToken startRuntimes();

  const std::string addr_;
  const int num_threads_;
  folly::EventBase *evb_;
  folly::Executor::KeepAlive<> executor_ = folly::getGlobalCPUExecutor();

  std::unique_ptr<grpc::Service> service_;
  std::vector<std::unique_ptr<grpc::ServerCompletionQueue>> cqs_{};
  std::unique_ptr<grpc::Server> server_;
  std::unique_ptr<IRpcHanlderRegistry> registry_;

  std::vector<std::unique_ptr<RpcServiceRuntime>> runtimes_;
  std::vector<std::thread> threads_;
};
} // namespace bapid
