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
#include <memory>
#include <tuple>
#include <type_traits>
#include <vector>

namespace bapid {
namespace {
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

} // namespace

template <typename TService> struct RuntimeCtxBase {
  typename TService::AsyncService *service;
  grpc::ServerCompletionQueue *cq;
  folly::Executor *executor;
};

struct HandlerState;
struct CallDataBase {
  HandlerState *state;
  grpc::ServerContext grpc_ctx{};
  bool processed{false};
};

struct HandlerState {
  grpc::ServerCompletionQueue *cq;
  folly::Executor *executor;
  std::function<void(CallDataBase *)> proceed_fn;
  std::function<void()> register_fn;

  HandlerState(grpc::ServerCompletionQueue *cq, folly::Executor *executor)
      : cq{cq}, executor{executor} {}
};

template <typename TService, typename THanlderCtx> class RpcHanlderRegistry {
public:
  template <typename Request, typename Reply>
  using Hanlder = std::function<folly::coro::Task<void>(
      Reply &reply, const Request &request, THanlderCtx &ctx)>;
  using BindHandlerFn = std::function<std::unique_ptr<HandlerState>(
      RuntimeCtxBase<TService> &runtime_ctx)>;

  explicit RpcHanlderRegistry(THanlderCtx hanlder_ctx)
      : hanlder_ctx_{hanlder_ctx} {}

  std::vector<std::unique_ptr<HandlerState>>
  bindRuntime(RuntimeCtxBase<TService> &runtime_ctx) {
    std::vector<std::unique_ptr<HandlerState>> states{};
    std::for_each(hanlder_binders_.begin(), hanlder_binders_.end(),
                  [&](auto &add_to_runtime) {
                    states.emplace_back(add_to_runtime(runtime_ctx));
                  });

    return states;
  }

  template <auto TGrpcRegisterFn,
            typename Request = typename unwrap_request<TGrpcRegisterFn>::type,
            typename Reply = typename unwrap_reply<TGrpcRegisterFn>::type>
  void registerHandler(Hanlder<Request, Reply> &&process) {
    struct CallData : public CallDataBase {
      grpc::ServerAsyncResponseWriter<Reply> responder;
      Request request{};
      Reply reply{};

      explicit CallData(HandlerState *state)
          : CallDataBase{state}, responder{&grpc_ctx} {}
    };

    BindHandlerFn bind_hanlder =
        [&hanlder_ctx = hanlder_ctx_,
         process = std::move(process)](RuntimeCtxBase<TService> &runtime_ctx)
        -> std::unique_ptr<HandlerState> {
      auto state =
          std::make_unique<HandlerState>(runtime_ctx.cq, runtime_ctx.executor);

      state->proceed_fn = [&hanlder_ctx = hanlder_ctx, &process = process,
                           state = state.get()](CallDataBase *baseData) {
        auto data = static_cast<CallData *>(baseData);
        if (!data->processed) {
          data->processed = true;
          state->register_fn();

          process(data->reply, data->request, hanlder_ctx)
              .scheduleOn(state->executor)
              .start()
              .defer([data = data](auto &&) {
                data->responder.Finish(data->reply, grpc::Status::OK, data);
              })
              .via(state->executor);

        } else {
          delete data; // NOLINT
        }
      };
      state->register_fn = [state = state.get(),
                            service = runtime_ctx.service]() {
        auto data = new CallData(state); // NOLINT
        (service->*TGrpcRegisterFn)(&(data->grpc_ctx), &(data->request),
                                    &(data->responder), state->cq, state->cq,
                                    data);
      };

      state->register_fn();
      return state;
    };

    hanlder_binders_.emplace_back(std::move(bind_hanlder));
  }

private:
  THanlderCtx hanlder_ctx_;
  std::vector<BindHandlerFn> hanlder_binders_{};
};

template <typename TService> class ServiceRuntimeBase {
public:
  using RuntimeCtx = RuntimeCtxBase<TService>;
  using BindRegistryFn =
      std::function<std::vector<std::unique_ptr<HandlerState>>(RuntimeCtx &)>;

  void serve() {
    void *tag{};
    bool ok{false};
    while (ctx_.cq->Next(&tag, &ok)) {
      if (!ok) {
        break;
      }

      auto *call_data = static_cast<CallDataBase *>(tag);
      call_data->state->proceed_fn(call_data);
    }
  }

  ServiceRuntimeBase(RuntimeCtx ctx, BindRegistryFn &addHandlers)
      : ctx_{ctx}, handler_states_{addHandlers(ctx_)} {}

  ~ServiceRuntimeBase() {
    void *ignored_tag{};
    bool ignored_ok{};
    while (ctx_.cq->Next(&ignored_tag, &ignored_ok)) {
    }
  }

  ServiceRuntimeBase(const ServiceRuntimeBase &) = delete;
  ServiceRuntimeBase(ServiceRuntimeBase &&) noexcept = delete;
  ServiceRuntimeBase &operator=(const ServiceRuntimeBase &) = delete;
  ServiceRuntimeBase &operator=(ServiceRuntimeBase &&) noexcept = delete;

private:
  RuntimeCtx ctx_;
  std::vector<std::unique_ptr<HandlerState>> handler_states_{};
};

} // namespace bapid
