#pragma once

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
} // namespace detail

template <typename TService> struct ServiceCtxBase {
  typename TService::AsyncService *service;
  grpc::ServerCompletionQueue *cq;
  folly::Executor *executor;
};

using ProceedFn = std::function<void()>;
struct CallDataBase {
  ProceedFn proceedFn;
  grpc::ServerContext grpcCtx{};
};

template <typename TServer, typename TService, typename THanlderCtx,
          typename THandler, auto TRegisterFn>
class HandlerBase {
public:
  using ServiceCtx = ServiceCtxBase<TService>;
  using Request = std::remove_pointer_t<typename detail::function_traits<
      decltype(TRegisterFn)>::template arg<1>::type>;
  using Reply = typename detail::unwrap<
      std::remove_pointer_t<typename detail::function_traits<
          decltype(TRegisterFn)>::template arg<2>::type>>::type;

  struct CallData : public CallDataBase {
    grpc::ServerAsyncResponseWriter<Reply> responder;
    HandlerBase *handler;
    Request request{};
    Reply reply{};
    bool processed{false};

    explicit CallData(HandlerBase *handler)
        : CallDataBase{[=]() { handler->proceed(this); }}, responder{&grpcCtx},
          handler{handler} {}
  };

  HandlerBase(ServiceCtx serviceCtx, THanlderCtx hanlderCtx)
      : serviceCtx_{serviceCtx}, hanlderCtx_{hanlderCtx}, registerFn_{[this]() {
          auto data = new CallData(this); // NOLINT
          (serviceCtx_.service->*TRegisterFn)(
              &(data->grpcCtx), &(data->request), &(data->responder),
              serviceCtx_.cq, serviceCtx_.cq, data);
        }} {
    registerFn_();
  }

private:
  void proceed(CallData *data) {
    if (!data->processed) {
      data->processed = true;
      registerFn_();

      static_cast<THandler *>(this)
          ->process(data, hanlderCtx_)
          .scheduleOn(serviceCtx_.executor)
          .start()
          .defer([&](auto &&) {
            data->responder.Finish(data->reply, grpc::Status::OK, data);
          })
          .via(serviceCtx_.executor);

    } else {
      delete data; // NOLINT
    }
  }

  ServiceCtx serviceCtx_;
  THanlderCtx hanlderCtx_;
  std::function<void()> registerFn_;
};

template <typename TService, typename THandlerCtx> class ServiceRuntimeBase {
public:
  void addHanlder();
  void serve() {
    void *tag{};
    bool ok{false};
    while (ctx_->cq->Next(&tag, &ok)) {
      if (!ok) {
        break;
      }

      (static_cast<CallDataBase *>(tag))->proceedFn();
    }
  }

private:
  ServiceCtxBase<TService> ctx_;
};

} // namespace bapid
