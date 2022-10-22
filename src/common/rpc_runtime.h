#pragma once

#include <algorithm>
#include <folly/Unit.h>
#include <folly/executors/GlobalExecutor.h>
#include <folly/experimental/coro/Task.h>
#include <folly/logging/xlog.h>
#include <functional>
#include <grpc/support/log.h>
#include <grpcpp/completion_queue.h>
#include <grpcpp/grpcpp.h>
#include <list>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace bapid {
template <typename TFn> struct extract_args;

template <typename TRes, typename TKlass, typename... TArgs>
struct extract_args<TRes (TKlass::*)(TArgs...)> {
  template <size_t i> struct arg {
    using type = typename std::tuple_element<i, std::tuple<TArgs...>>::type;
  };
};

template <typename TWrapper> struct unwrap;
template <template <typename> class TOuter, typename TInner>
struct unwrap<TOuter<TInner>> {
  using type = TInner;
};

template <auto TGrpcRegisterFn> struct unwrap_request {
  using type = std::remove_pointer_t<
      typename extract_args<decltype(TGrpcRegisterFn)>::template arg<1>::type>;
};

template <auto TGrpcRegisterFn> struct unwrap_reply {
  using type = typename unwrap<std::remove_pointer_t<typename extract_args<
      decltype(TGrpcRegisterFn)>::template arg<2>::type>>::type;
};

struct HandlerState;
struct CallDataBase {
  HandlerState *state;
  grpc::ServerContext grpc_ctx{};
  std::list<std::unique_ptr<CallDataBase>>::iterator it;
  bool processed{false};
};

struct RpcRuntimeCtx {
  grpc::Service *service;
  grpc::ServerCompletionQueue *cq;
  folly::Executor *executor;
};

struct HandlerState;
struct IHandlerRecord {
  using ProcessFn =
      std::function<folly::SemiFuture<folly::Unit>(CallDataBase *)>;
  using ReceivingNextRequest =
      std::function<std::unique_ptr<CallDataBase>(HandlerState *state)>;
  using BindRuntimeFn =
      std::function<std::unique_ptr<HandlerState>(RpcRuntimeCtx &runtime_ctx)>;

  IHandlerRecord(ProcessFn process_fn,
                 ReceivingNextRequest receiving_next_request_fn,
                 BindRuntimeFn bind_runtime_fn);

  ProcessFn process_fn;
  ReceivingNextRequest receiving_next_request_fn;
  BindRuntimeFn bind_runtime_fn;
};

struct HandlerState {
  RpcRuntimeCtx ctx;
  IHandlerRecord *record;

  std::unique_ptr<CallDataBase> next_call_data{};
  std::list<std::unique_ptr<CallDataBase>> inflight_call_data{};

  HandlerState(RpcRuntimeCtx ctx, IHandlerRecord *record);
  void receivingNextRequest();
  void processCallData(CallDataBase *call_data);
};

class IRpcHanlderRegistry {
public:
  std::vector<std::unique_ptr<HandlerState>>
  bindRuntime(RpcRuntimeCtx &runtime_ctx);

  virtual ~IRpcHanlderRegistry() = default;

  IRpcHanlderRegistry() = default;
  IRpcHanlderRegistry(const IRpcHanlderRegistry &) = default;
  IRpcHanlderRegistry(IRpcHanlderRegistry &&) = default;
  IRpcHanlderRegistry &operator=(const IRpcHanlderRegistry &) = default;
  IRpcHanlderRegistry &operator=(IRpcHanlderRegistry &&) = default;

protected:
  std::vector<std::unique_ptr<IHandlerRecord>> hanlder_records_{};
};

template <typename TService, typename THanlderCtx, typename THanlders>
class RpcHanlderRegistry : public IRpcHanlderRegistry {
public:
  template <typename Request, typename Reply>
  using Hanlder = folly::coro::Task<void> (THanlders::*)(Reply &reply,
                                                         const Request &request,
                                                         THanlderCtx &ctx);

  explicit RpcHanlderRegistry(THanlderCtx hanlder_ctx)
      : hanlder_ctx_{hanlder_ctx} {}

  template <auto TGrpcRegisterFn,
            typename Request = typename unwrap_request<TGrpcRegisterFn>::type,
            typename Reply = typename unwrap_reply<TGrpcRegisterFn>::type>
  void registerHandler(Hanlder<Request, Reply> process) {
    struct CallData : public CallDataBase {
      grpc::ServerAsyncResponseWriter<Reply> responder;
      Request request{};
      Reply reply{};

      explicit CallData(HandlerState *state)
          : CallDataBase{state}, responder{&grpc_ctx} {}
    };

    struct HanlderRecord : public IHandlerRecord {
      HanlderRecord(THanlders *hanlder, Hanlder<Request, Reply> process,
                    THanlderCtx *hanlder_ctx)
          : IHandlerRecord(
                /*process_fn=*/
                [hanlder, process, hanlder_ctx](CallDataBase *baseData) {
                  auto *data = static_cast<CallData *>(baseData);
                  return (hanlder->*process)(data->reply, data->request,
                                             *hanlder_ctx)
                      .semi()
                      .deferValue([data = data](auto &&) {
                        data->responder.Finish(data->reply, grpc::Status::OK,
                                               data);
                      });
                },
                /*receiving_next_request_fn=*/
                [](HandlerState *state) {
                  auto *service =
                      dynamic_cast<typename TService::AsyncService *>(
                          state->ctx.service);
                  auto data = std::make_unique<CallData>(state);
                  (service->*TGrpcRegisterFn)(
                      &(data->grpc_ctx), &(data->request), &(data->responder),
                      state->ctx.cq, state->ctx.cq, data.get());
                  return data;
                },
                /*bind_runtime_fn=*/
                [this](RpcRuntimeCtx &ctx) {
                  auto state = std::make_unique<HandlerState>(ctx, this);
                  state->receivingNextRequest();
                  return state;
                }) {}
    };

    hanlder_records_.emplace_back(
        std::make_unique<HanlderRecord>(&hanlders_, process, &hanlder_ctx_));
  }

private:
  THanlderCtx hanlder_ctx_;
  THanlders hanlders_{};
};

class RpcServiceRuntime {
public:
  RpcServiceRuntime(RpcRuntimeCtx ctx,
                    std::unique_ptr<IRpcHanlderRegistry> &registry);

  void serve() const;
  ~RpcServiceRuntime();

  RpcServiceRuntime(const RpcServiceRuntime &) = delete;
  RpcServiceRuntime(RpcServiceRuntime &&) noexcept = delete;
  RpcServiceRuntime &operator=(const RpcServiceRuntime &) = delete;
  RpcServiceRuntime &operator=(RpcServiceRuntime &&) noexcept = delete;

private:
  RpcRuntimeCtx ctx_;
  std::vector<std::unique_ptr<HandlerState>> handler_states_;
};

} // namespace bapid
