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

template <typename TServer, typename TService> struct ServiceCtxBase {
  TServer *server;
  typename TService::AsyncService *service;
  grpc::ServerCompletionQueue *cq;
  folly::Executor *executor;
};

using ProceedFn = std::function<void()>;
template <typename TServer, typename TService, typename THandler,
          auto TRegisterFn>
class HandlerBase {
public:
  using ServiceCtx = ServiceCtxBase<TServer, TService>;
  using Request = std::remove_pointer_t<typename detail::function_traits<
      decltype(TRegisterFn)>::template arg<1>::type>;
  using Reply = typename detail::unwrap<
      std::remove_pointer_t<typename detail::function_traits<
          decltype(TRegisterFn)>::template arg<2>::type>>::type;
  using type = HandlerBase<TService, TServer, THandler, TRegisterFn>;

  struct CallData {
    HandlerBase *handler;
    Request request{};
    Reply reply{};
    bool processed{false};
  };

  explicit HandlerBase(ServiceCtx ctx)
      : ctx_{ctx}, responder_(&grpcCtx_), registerFn_{[this]() {
          auto data = new CallData(this); // NOLINT
          (ctx_.service->*TRegisterFn)(&grpcCtx_, *(data->req), &responder_,
                                       ctx_.cq, ctx_.cq, data);
        }} {
    registerFn_();
  }

  void proceed(CallData *data) {
    if (!data->processed) {
      data->processed = true;
      registerFn_();

      static_cast<THandler *>(this)
          ->process()
          .scheduleOn(ctx_.executor_)
          .start()
          .defer([&](auto &&) {
            responder_.Finish(&(data->reply), grpc::Status::OK, &data);
          })
          .via(ctx_.executor_);

    } else {
      delete data; // NOLINT
    }
  }

private:
  friend THandler;
  grpc::ServerContext grpcCtx_;
  grpc::ServerAsyncResponseWriter<Reply> responder_;
  ServiceCtx ctx_;
  std::function<void()> registerFn_;
};
} // namespace bapid
