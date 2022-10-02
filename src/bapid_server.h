#pragma once

#include "if/bapid.grpc.pb.h"
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

template <typename THandler, auto TRegisterFn> class CallDataBase {
public:
  using Request = std::remove_pointer_t<
      typename function_traits<decltype(TRegisterFn)>::template arg<1>::type>;
  using Reply = typename unwrap<std::remove_pointer_t<typename function_traits<
      decltype(TRegisterFn)>::template arg<2>::type>>::type;
  using type = CallDataBase<THandler, TRegisterFn>;

  CallDataBase(BapidService::AsyncService *service,
               grpc::ServerCompletionQueue *cq)
      : service_{service}, cq_{cq}, responder_(&ctx_) {
    (service_->*TRegisterFn)(&ctx_, &request_, &responder_, cq_, cq_,
                             static_cast<THandler *>(this));
  }

  void proceed() {
    if (!processed_) {
      new CallDataBase<THandler, TRegisterFn>(service_, cq_);
      static_cast<THandler *>(this)->process();
      processed_ = true;
      responder_.Finish(reply_, grpc::Status::OK,
                        static_cast<THandler *>(this));
    } else {
      delete static_cast<THandler *>(this);
    }
  }

private:
  friend THandler;
  BapidService::AsyncService *service_;
  grpc::ServerCompletionQueue *cq_;
  grpc::ServerContext ctx_;

  Request request_;
  Reply reply_;

  grpc::ServerAsyncResponseWriter<PingReply> responder_;
  bool processed_{false};
};

class PingHandler
    : public CallDataBase<PingHandler,
                          &BapidService::AsyncService::RequestPing> {
public:
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

private:
  std::atomic<bool> shutdownRequested_{false};
  const std::string addr_;
  BapidService::AsyncService service_{};
  std::unique_ptr<grpc::ServerCompletionQueue> cq_;
  std::unique_ptr<grpc::Server> server_;
  ;
};
} // namespace bapid
