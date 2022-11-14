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
#include <grpcpp/impl/service_type.h>
#include <list>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace bapid {
template <typename TFn> struct extract_args;

// Extracts the i-th args of the function type
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

// Extracts the request type from the generated gRPC hanlder function
template <auto TGrpcRegisterFn> struct unwrap_request {
  using type = std::remove_pointer_t<
      typename extract_args<decltype(TGrpcRegisterFn)>::template arg<1>::type>;
};

// Extracts the reply type from the generated gRPC hanlder function
template <auto TGrpcRegisterFn> struct unwrap_reply {
  using type = typename unwrap<std::remove_pointer_t<typename extract_args<
      decltype(TGrpcRegisterFn)>::template arg<2>::type>>::type;
};

struct HandlerState;
// Holds the state of a call of a gRPC method
// Created when the runtime is ready to handle the next call
// Populated (the request of this call) when the runtime handles the gRPC call
// Deleted after the reply is sent
struct CallDataBase {
  HandlerState *state;
  grpc::ServerContext grpc_ctx{};
  // The handler state holds a std::list of CallData of inflight calls.
  // When a call is finished, its CallData should be deleted. This iterator is
  // used for deleting it from the list.
  std::list<std::unique_ptr<CallDataBase>>::iterator it;
  // true if the hanlder has handled the call and a reply is ready.
  bool processed{false};
};

// Holds the runtime's completion queue and the executor for running the
// handlers
struct RpcRuntimeCtx {
  grpc::ServerCompletionQueue *cq;
  folly::Executor *executor;
};

// Responsible for setting up a runtime to start handling a gRPC method
// For a gRPC service, there is one HanlderRecord for each method.
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

  // The busines logic for handling a call of the method
  ProcessFn process_fn;
  // Creates a new CallData to be used for the next call
  ReceivingNextRequest receiving_next_request_fn;
  // Sets up the runtime so that it can handle the gRPC methodd
  BindRuntimeFn bind_runtime_fn;
};

// Holds the state of the handler for a runtime
// For a gRPC runtime, there is one HandlerState for each Handler bound to it.
struct HandlerState {
  RpcRuntimeCtx ctx;
  IHandlerRecord *record;

  std::unique_ptr<CallDataBase> next_call_data{};
  std::list<std::unique_ptr<CallDataBase>> inflight_call_data{};

  HandlerState(RpcRuntimeCtx ctx, IHandlerRecord *record);
  void receivingNextRequest();
  void processCallData(CallDataBase *call_data);
};

// Interface of RpcHandlerRegistry. The registry holds the handler records and
// is responsible for setting up a runtime to handle gRPC methods. For a gRPC
// service, there is one RpcHanlderRegistry
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

// Template for the registry for a given gRPC service
template <typename TService, typename THanlderCtx, typename THanlders>
class RpcHanlderRegistry : public IRpcHanlderRegistry {
public:
  // The implementation of the business logic of the handler
  // `request` is the request of the call, `ctx` is the additional
  // service-specific context needed for handling the call, and `reply` is the
  // result of the call.
  template <typename Request, typename Reply>
  using Hanlder = folly::coro::Task<void> (THanlders::*)(Reply &reply,
                                                         const Request &request,
                                                         THanlderCtx &ctx);

  RpcHanlderRegistry(grpc::Service *service, THanlderCtx hanlder_ctx)
      : service_{dynamic_cast<typename TService::AsyncService *>(service)},
        hanlder_ctx_{hanlder_ctx} {}

  // Creates the handler record for a hanlder
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
      HanlderRecord(typename TService::AsyncService *service,
                    THanlders *hanlder, Hanlder<Request, Reply> process,
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
                [service](HandlerState *state) {
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

    hanlder_records_.emplace_back(std::make_unique<HanlderRecord>(
        service_, &hanlders_, process, &hanlder_ctx_));
  }

private:
  typename TService::AsyncService *service_;
  THanlderCtx hanlder_ctx_;
  THanlders hanlders_{};
};

// Responsible for listening to a gRPC completion queue and
// calling handlers for the requests
class RpcServiceRuntime {
public:
  RpcServiceRuntime(RpcRuntimeCtx ctx,
                    std::unique_ptr<IRpcHanlderRegistry> &registry);

  // Starts listening to the completion queue
  // Returns when the queue is shutdown
  void serve() const;

  // Responsible for draining the completion queue
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
