#pragma once

#include <algorithm>
#include <folly/Unit.h>
#include <folly/executors/GlobalExecutor.h>
#include <folly/experimental/coro/Task.h>
#include <folly/io/async/EventBaseManager.h>
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
template <typename TFn> struct function_traits;

template <typename TRes, typename TKlass, typename... TArgs>
struct function_traits<TRes (TKlass::*)(TArgs...)> {
  template <size_t i> struct arg {
    using type = typename std::tuple_element<i, std::tuple<TArgs...>>::type;
  };
};

template <typename TWrapper> struct unwrap;
template <template <typename> class TOuter, typename TInner>
struct unwrap<TOuter<TInner>> {
  using type = TInner;
};

constexpr gpr_timespec kNoDeadline{std::numeric_limits<std::int64_t>::max(), 0,
                                   GPR_CLOCK_MONOTONIC};

} // namespace detail

template <typename TService> struct RuntimeCtxBase {
  typename TService::AsyncService *service;
  grpc::ServerCompletionQueue *cq;
  folly::Executor *executor;
};

struct CallDataBase;
using ProceedFn = std::function<void()>;
struct CallDataBase {
  ProceedFn proceedFn;
  grpc::ServerContext grpcCtx{};
};

using RegisterFn = std::function<void()>;
struct HandlerState {
  grpc::ServerCompletionQueue *cq;
  folly::Executor *executor;
  std::function<void(CallDataBase *)> proceedFn;
  RegisterFn registerFn;

  HandlerState(grpc::ServerCompletionQueue *cq, folly::Executor *executor)
      : cq{cq}, executor{executor} {}
};

template <typename TService> class IHanlder {
public:
  IHanlder() = default;
  virtual ~IHanlder() = default;
  virtual std::unique_ptr<HandlerState>
  addToRuntime(RuntimeCtxBase<TService> &runtimeCtx) = 0;

  IHanlder(const IHanlder &) = default;
  IHanlder(IHanlder &&) noexcept = default;
  IHanlder &operator=(const IHanlder &) = default;
  IHanlder &operator=(IHanlder &&) noexcept = default;
};

template <typename TServer, typename TService, typename THanlderCtx,
          typename THandler, auto TRegisterFn>
class HandlerBase : public IHanlder<TService> {
public:
  using Request = std::remove_pointer_t<typename detail::function_traits<
      decltype(TRegisterFn)>::template arg<1>::type>;
  using Reply = typename detail::unwrap<
      std::remove_pointer_t<typename detail::function_traits<
          decltype(TRegisterFn)>::template arg<2>::type>>::type;

  struct CallData : public CallDataBase {
    grpc::ServerAsyncResponseWriter<Reply> responder;
    HandlerState *state;
    Request request{};
    Reply reply{};
    bool processed{false};

    explicit CallData(HandlerState *state)
        : CallDataBase{[=]() { state->proceedFn(this); }}, responder{&grpcCtx},
          state{state} {}
  };

  explicit HandlerBase(THanlderCtx hanlderCtx) : hanlderCtx_{hanlderCtx} {}

  std::unique_ptr<HandlerState>
  addToRuntime(RuntimeCtxBase<TService> &runtimeCtx) override {
    auto state =
        std::make_unique<HandlerState>(runtimeCtx.cq, runtimeCtx.executor);
    state->proceedFn = [this, statePtr = state.get()](CallDataBase *data) {
      proceed(static_cast<CallData *>(data), statePtr);
    };
    state->registerFn = [this, statePtr = state.get(),
                         service = runtimeCtx.service]() {
      auto data = new CallData(statePtr); // NOLINT
      (service->*TRegisterFn)(&(data->grpcCtx), &(data->request),
                              &(data->responder), statePtr->cq, statePtr->cq,
                              data);
    };

    state->registerFn();
    return state;
  }

private:
  void proceed(CallData *data, HandlerState *state) {
    if (!data->processed) {
      data->processed = true;
      state->registerFn();

      THandler::process(data, hanlderCtx_)
          .scheduleOn(state->executor)
          .start()
          .defer([data = data](auto &&) {
            data->responder.Finish(data->reply, grpc::Status::OK, data);
          })
          .via(state->executor);

    } else {
      delete data; // NOLINT
    }
  }

  THanlderCtx hanlderCtx_;
};

template <typename TService> class ServiceRuntimeBase {
public:
  using RuntimeCtx = RuntimeCtxBase<TService>;

  class LoopCb : public folly::EventBase::LoopCallback {
  public:
    LoopCb(RuntimeCtx &ctx, folly::EventBase *evb)
        : folly::EventBase::LoopCallback(), cq_{ctx.cq}, evb_{evb} {
      evb_->runInLoop(this);
    }

    void runLoopCallback() noexcept override {
      void *tag{};
      bool ok{false};
      auto status = cq_->AsyncNext(&tag, &ok, detail::kNoDeadline);
      if (status == grpc::CompletionQueue::NextStatus::GOT_EVENT && ok) {
        (static_cast<CallDataBase *>(tag))->proceedFn();
        evb_->runInLoop(this);
      } else {
        evb_->terminateLoopSoon();
      }
    }

  private:
    grpc::ServerCompletionQueue *cq_;
    folly::EventBase *evb_;
  };

  void serve() {
    auto *evb = folly::EventBaseManager::get()->getEventBase();
    LoopCb cb{ctx_, evb};
    evb->loopForever();
  }

  ServiceRuntimeBase(RuntimeCtx ctx,
                     std::vector<std::unique_ptr<IHanlder<TService>>> &handlers)
      : ctx_{ctx} {
    std::for_each(handlers.begin(), handlers.end(), [this](auto &hanlder) {
      handlerStates_.emplace_back(hanlder->addToRuntime(ctx_));
    });
  }

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
  std::vector<std::unique_ptr<HandlerState>> handlerStates_{};
};

} // namespace bapid
