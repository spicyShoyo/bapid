#pragma once

#include "if/bapid.grpc.pb.h"
#include <folly/experimental/coro/Task.h>
#include <grpc/support/log.h>
#include <grpcpp/completion_queue.h>
#include <grpcpp/grpcpp.h>
#include <memory>
#include <tuple>
#include <type_traits>

namespace bapid {
namespace {
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
} // namespace

class BapidServer;
struct CallData {
  BapidServer *server;
  BapidService::AsyncService *service;
  grpc::ServerCompletionQueue *cq;
};

using ProceedFn = std::function<void()>;
template <typename THandler, auto TRegisterFn> class HandlerBase {
public:
  using Request = std::remove_pointer_t<
      typename function_traits<decltype(TRegisterFn)>::template arg<1>::type>;
  using Reply = typename unwrap<std::remove_pointer_t<typename function_traits<
      decltype(TRegisterFn)>::template arg<2>::type>>::type;
  using type = HandlerBase<THandler, TRegisterFn>;

  explicit HandlerBase(CallData data)
      : proceedFn_{[this]() { static_cast<THandler *>(this)->proceed(); }},
        data_{data}, responder_(&ctx_) {
    (data_.service->*TRegisterFn)(&ctx_, &request_, &responder_, data_.cq,
                                  data.cq, &proceedFn_);
  }

  void proceed() {
    if (!processed_) {
      new HandlerBase<THandler, TRegisterFn>(data_);
      static_cast<THandler *>(this)->process();
      processed_ = true;
      responder_.Finish(reply_, grpc::Status::OK, &proceedFn_);
    } else {
      delete static_cast<THandler *>(this);
    }
  }

private:
  friend THandler;
  ProceedFn proceedFn_;

  CallData data_;
  grpc::ServerContext ctx_;
  grpc::ServerAsyncResponseWriter<Reply> responder_;
  bool processed_{false};

  Request request_;
  Reply reply_;
};

class PingHandler
    : public HandlerBase<PingHandler,
                         &BapidService::AsyncService::RequestPing> {
  friend HandlerBase;
  void process();
};

class Ping2Handler
    : public HandlerBase<Ping2Handler,
                         &BapidService::AsyncService::RequestPing2> {
  friend HandlerBase;
  void process();
};

class ShutdownHandler
    : public HandlerBase<ShutdownHandler,
                         &BapidService::AsyncService::RequestShutdown> {
  friend HandlerBase;
  void process();
};

class BapidServer final {
public:
  explicit BapidServer(std::string addr);

  ~BapidServer();
  BapidServer(BapidServer &&other) noexcept = delete;
  BapidServer &operator=(BapidServer &&other) noexcept = delete;
  BapidServer &operator=(const BapidServer &other) = delete;
  BapidServer(const BapidServer &other) = delete;

  void serve();
  void initiateShutdown();

private:
  folly::coro::Task<void> doShutdown();

  std::atomic<bool> shutdownRequested_{false};
  const std::string addr_;
  BapidService::AsyncService service_{};
  std::unique_ptr<grpc::ServerCompletionQueue> cq_;
  std::unique_ptr<grpc::Server> server_;
};
} // namespace bapid
