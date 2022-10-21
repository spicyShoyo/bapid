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

struct HandlerState {
  grpc::ServerCompletionQueue *cq;
  folly::Executor *executor;
  std::unique_ptr<CallDataBase> next_call_data{};
  std::list<std::unique_ptr<CallDataBase>> inflight_call_data{};

  std::function<void(CallDataBase *)> proceed_fn;
  std::function<std::unique_ptr<CallDataBase>()> receiving_next_request_fn;

  HandlerState(grpc::ServerCompletionQueue *cq, folly::Executor *executor);
  void receivingNextRequest();
};

struct RpcRuntimeCtx {
  grpc::Service *service;
  grpc::ServerCompletionQueue *cq;
  folly::Executor *executor;
};

class IRpcHanlderRegistry {
public:
  virtual std::vector<std::unique_ptr<HandlerState>>
  bindRuntime(RpcRuntimeCtx &runtime_ctx) = 0;

  virtual ~IRpcHanlderRegistry() = default;

  IRpcHanlderRegistry() = default;
  IRpcHanlderRegistry(const IRpcHanlderRegistry &) = default;
  IRpcHanlderRegistry(IRpcHanlderRegistry &&) = default;
  IRpcHanlderRegistry &operator=(const IRpcHanlderRegistry &) = default;
  IRpcHanlderRegistry &operator=(IRpcHanlderRegistry &&) = default;
};

template <typename TService, typename THanlderCtx, typename THanlders>
class RpcHanlderRegistry : public IRpcHanlderRegistry {
public:
  template <typename Request, typename Reply>
  using Hanlder = folly::coro::Task<void> (THanlders::*)(Reply &reply,
                                                         const Request &request,
                                                         THanlderCtx &ctx);
  using BindHandlerFn =
      std::function<std::unique_ptr<HandlerState>(RpcRuntimeCtx &runtime_ctx)>;

  explicit RpcHanlderRegistry(THanlderCtx hanlder_ctx)
      : hanlder_ctx_{hanlder_ctx} {}

  std::vector<std::unique_ptr<HandlerState>>
  bindRuntime(RpcRuntimeCtx &runtime_ctx) override {
    std::vector<std::unique_ptr<HandlerState>> states{};
    std::for_each(hanlder_binders_.begin(), hanlder_binders_.end(),
                  [&](auto &bind_handler) {
                    states.emplace_back(bind_handler(runtime_ctx));
                  });

    return states;
  }

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

    BindHandlerFn bind_hanlder =
        [this,
         process](RpcRuntimeCtx &runtime_ctx) -> std::unique_ptr<HandlerState> {
      auto state =
          std::make_unique<HandlerState>(runtime_ctx.cq, runtime_ctx.executor);

      state->proceed_fn = [this, process,
                           state = state.get()](CallDataBase *baseData) {
        auto *data = static_cast<CallData *>(baseData);

        if (!data->processed) {
          XCHECK(data == state->next_call_data.get());
          state->receivingNextRequest();

          data->processed = true;
          (this->hanlders_.*process)(data->reply, data->request,
                                     this->hanlder_ctx_)
              .scheduleOn(state->executor)
              .start()
              .defer([data = data](auto &&) {
                data->responder.Finish(data->reply, grpc::Status::OK, data);
              })
              .via(state->executor);

        } else {
          state->inflight_call_data.erase(data->it);
        }
      };

      state->receiving_next_request_fn =
          [state = state.get(),
           service = dynamic_cast<typename TService::AsyncService *>(
               runtime_ctx.service)]() {
            auto data = std::make_unique<CallData>(state);
            (service->*TGrpcRegisterFn)(&(data->grpc_ctx), &(data->request),
                                        &(data->responder), state->cq,
                                        state->cq, data.get());
            return data;
          };

      state->receivingNextRequest();
      return state;
    };

    hanlder_binders_.emplace_back(std::move(bind_hanlder));
  }

private:
  THanlderCtx hanlder_ctx_;
  THanlders hanlders_{};
  std::vector<BindHandlerFn> hanlder_binders_{};
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
