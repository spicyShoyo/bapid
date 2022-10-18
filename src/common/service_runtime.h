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
namespace detail {
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
  using type = std::remove_pointer_t<typename detail::extract_args<
      decltype(TGrpcRegisterFn)>::template arg<1>::type>;
};

template <auto TGrpcRegisterFn> struct unwrap_reply {
  using type = typename detail::unwrap<
      std::remove_pointer_t<typename detail::extract_args<
          decltype(TGrpcRegisterFn)>::template arg<2>::type>>::type;
};

} // namespace detail

template <typename TService> struct RuntimeCtxBase {
  typename TService::AsyncService *service;
  grpc::ServerCompletionQueue *cq;
  folly::Executor *executor;
};

struct CallDataBase {
  std::function<void()> proceedFn;
  grpc::ServerContext grpcCtx{};
  bool processed{false};
};

struct HandlerState {
  grpc::ServerCompletionQueue *cq;
  folly::Executor *executor;
  std::function<void(CallDataBase *)> proceedFn;
  std::function<void()> registerFn;

  HandlerState(grpc::ServerCompletionQueue *cq, folly::Executor *executor)
      : cq{cq}, executor{executor} {}
};

template <typename TService, typename THanlderCtx> class RpcHanlderRegistry {
public:
  template <typename Request, typename Reply>
  using Hanlder = std::function<folly::coro::Task<void>(
      Reply &reply, const Request &request, THanlderCtx &ctx)>;
  using AddToRuntimeFn = std::function<std::unique_ptr<HandlerState>(
      RuntimeCtxBase<TService> &runtimeCtx)>;

  explicit RpcHanlderRegistry(THanlderCtx hanlder_ctx)
      : hanlder_ctx_{hanlder_ctx} {}

  std::vector<std::unique_ptr<HandlerState>>
  addHanldersToRuntime(RuntimeCtxBase<TService> &runtimeCtx) {
    std::vector<std::unique_ptr<HandlerState>> states{};
    std::for_each(handlers_.begin(), handlers_.end(), [&](auto &addToRuntime) {
      states.emplace_back(addToRuntime(runtimeCtx));
    });

    return states;
  }

  template <
      auto TGrpcRegisterFn,
      typename Request = typename detail::unwrap_request<TGrpcRegisterFn>::type,
      typename Reply = typename detail::unwrap_reply<TGrpcRegisterFn>::type>
  void registerHandler(Hanlder<Request, Reply> &&process) {
    struct CallData : public CallDataBase {
      grpc::ServerAsyncResponseWriter<Reply> responder;
      HandlerState *state;
      Request request{};
      Reply reply{};

      explicit CallData(HandlerState *state)
          : CallDataBase{[=]() { state->proceedFn(this); }},
            responder{&grpcCtx}, state{state} {}
    };

    AddToRuntimeFn addToRuntime =
        [&hanlder_ctx = hanlder_ctx_,
         process = std::move(process)](RuntimeCtxBase<TService> &runtime_ctx)
        -> std::unique_ptr<HandlerState> {
      auto state =
          std::make_unique<HandlerState>(runtime_ctx.cq, runtime_ctx.executor);

      state->proceedFn = [&hanlder_ctx = hanlder_ctx, &process = process,
                          state = state.get()](CallDataBase *baseData) {
        auto data = static_cast<CallData *>(baseData);
        if (!data->processed) {
          data->processed = true;
          state->registerFn();

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
      state->registerFn = [statePtr = state.get(),
                           service = runtime_ctx.service]() {
        auto data = new CallData(statePtr); // NOLINT
        (service->*TGrpcRegisterFn)(&(data->grpcCtx), &(data->request),
                                    &(data->responder), statePtr->cq,
                                    statePtr->cq, data);
      };

      state->registerFn();
      return state;
    };

    handlers_.emplace_back(std::move(addToRuntime));
  }

private:
  THanlderCtx hanlder_ctx_;
  std::vector<AddToRuntimeFn> handlers_{};
};

template <typename TService> class ServiceRuntimeBase {
public:
  using RuntimeCtx = RuntimeCtxBase<TService>;
  using AddHanldersFn =
      std::function<std::vector<std::unique_ptr<HandlerState>>(RuntimeCtx &)>;

  void serve() {
    void *tag{};
    bool ok{false};
    while (ctx_.cq->Next(&tag, &ok)) {
      if (!ok) {
        break;
      }

      (static_cast<CallDataBase *>(tag))->proceedFn();
    }
  }

  ServiceRuntimeBase(RuntimeCtx ctx, AddHanldersFn &addHandlers)
      : ctx_{ctx}, handler_states_{addHandlers(ctx_)} {}

  ~ServiceRuntimeBase() {
    void *ignored_tag{};
    bool ignored_ok{};
    while (ctx_.cq->Next(&ignored_tag, &ignored_ok)) {
    }
  }

  ServiceRuntimeBase(const ServiceRuntimeBase &) = default;
  ServiceRuntimeBase(ServiceRuntimeBase &&) noexcept = default;
  ServiceRuntimeBase &operator=(const ServiceRuntimeBase &) = default;
  ServiceRuntimeBase &operator=(ServiceRuntimeBase &&) noexcept = default;

private:
  RuntimeCtx ctx_;
  std::vector<std::unique_ptr<HandlerState>> handler_states_{};
};

} // namespace bapid
