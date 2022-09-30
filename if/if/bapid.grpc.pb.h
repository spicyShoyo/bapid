// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: if/bapid.proto
#ifndef GRPC_if_2fbapid_2eproto__INCLUDED
#define GRPC_if_2fbapid_2eproto__INCLUDED

#include "if/bapid.pb.h"

#include <functional>
#include <grpcpp/impl/codegen/async_generic_service.h>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/client_callback.h>
#include <grpcpp/impl/codegen/client_context.h>
#include <grpcpp/impl/codegen/completion_queue.h>
#include <grpcpp/impl/codegen/message_allocator.h>
#include <grpcpp/impl/codegen/method_handler.h>
#include <grpcpp/impl/codegen/proto_utils.h>
#include <grpcpp/impl/codegen/rpc_method.h>
#include <grpcpp/impl/codegen/server_callback.h>
#include <grpcpp/impl/codegen/server_callback_handlers.h>
#include <grpcpp/impl/codegen/server_context.h>
#include <grpcpp/impl/codegen/service_type.h>
#include <grpcpp/impl/codegen/status.h>
#include <grpcpp/impl/codegen/stub_options.h>
#include <grpcpp/impl/codegen/sync_stream.h>

namespace bapid {

class Greeter final {
 public:
  static constexpr char const* service_full_name() {
    return "bapid.Greeter";
  }
  class StubInterface {
   public:
    virtual ~StubInterface() {}
    virtual ::grpc::Status Ping(::grpc::ClientContext* context, const ::bapid::PingRequest& request, ::bapid::PingReply* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::bapid::PingReply>> AsyncPing(::grpc::ClientContext* context, const ::bapid::PingRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::bapid::PingReply>>(AsyncPingRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::bapid::PingReply>> PrepareAsyncPing(::grpc::ClientContext* context, const ::bapid::PingRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::bapid::PingReply>>(PrepareAsyncPingRaw(context, request, cq));
    }
    class async_interface {
     public:
      virtual ~async_interface() {}
      virtual void Ping(::grpc::ClientContext* context, const ::bapid::PingRequest* request, ::bapid::PingReply* response, std::function<void(::grpc::Status)>) = 0;
      virtual void Ping(::grpc::ClientContext* context, const ::bapid::PingRequest* request, ::bapid::PingReply* response, ::grpc::ClientUnaryReactor* reactor) = 0;
    };
    typedef class async_interface experimental_async_interface;
    virtual class async_interface* async() { return nullptr; }
    class async_interface* experimental_async() { return async(); }
   private:
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::bapid::PingReply>* AsyncPingRaw(::grpc::ClientContext* context, const ::bapid::PingRequest& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::bapid::PingReply>* PrepareAsyncPingRaw(::grpc::ClientContext* context, const ::bapid::PingRequest& request, ::grpc::CompletionQueue* cq) = 0;
  };
  class Stub final : public StubInterface {
   public:
    Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options = ::grpc::StubOptions());
    ::grpc::Status Ping(::grpc::ClientContext* context, const ::bapid::PingRequest& request, ::bapid::PingReply* response) override;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::bapid::PingReply>> AsyncPing(::grpc::ClientContext* context, const ::bapid::PingRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::bapid::PingReply>>(AsyncPingRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::bapid::PingReply>> PrepareAsyncPing(::grpc::ClientContext* context, const ::bapid::PingRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::bapid::PingReply>>(PrepareAsyncPingRaw(context, request, cq));
    }
    class async final :
      public StubInterface::async_interface {
     public:
      void Ping(::grpc::ClientContext* context, const ::bapid::PingRequest* request, ::bapid::PingReply* response, std::function<void(::grpc::Status)>) override;
      void Ping(::grpc::ClientContext* context, const ::bapid::PingRequest* request, ::bapid::PingReply* response, ::grpc::ClientUnaryReactor* reactor) override;
     private:
      friend class Stub;
      explicit async(Stub* stub): stub_(stub) { }
      Stub* stub() { return stub_; }
      Stub* stub_;
    };
    class async* async() override { return &async_stub_; }

   private:
    std::shared_ptr< ::grpc::ChannelInterface> channel_;
    class async async_stub_{this};
    ::grpc::ClientAsyncResponseReader< ::bapid::PingReply>* AsyncPingRaw(::grpc::ClientContext* context, const ::bapid::PingRequest& request, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientAsyncResponseReader< ::bapid::PingReply>* PrepareAsyncPingRaw(::grpc::ClientContext* context, const ::bapid::PingRequest& request, ::grpc::CompletionQueue* cq) override;
    const ::grpc::internal::RpcMethod rpcmethod_Ping_;
  };
  static std::unique_ptr<Stub> NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options = ::grpc::StubOptions());

  class Service : public ::grpc::Service {
   public:
    Service();
    virtual ~Service();
    virtual ::grpc::Status Ping(::grpc::ServerContext* context, const ::bapid::PingRequest* request, ::bapid::PingReply* response);
  };
  template <class BaseClass>
  class WithAsyncMethod_Ping : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithAsyncMethod_Ping() {
      ::grpc::Service::MarkMethodAsync(0);
    }
    ~WithAsyncMethod_Ping() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Ping(::grpc::ServerContext* /*context*/, const ::bapid::PingRequest* /*request*/, ::bapid::PingReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestPing(::grpc::ServerContext* context, ::bapid::PingRequest* request, ::grpc::ServerAsyncResponseWriter< ::bapid::PingReply>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(0, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  typedef WithAsyncMethod_Ping<Service > AsyncService;
  template <class BaseClass>
  class WithCallbackMethod_Ping : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithCallbackMethod_Ping() {
      ::grpc::Service::MarkMethodCallback(0,
          new ::grpc::internal::CallbackUnaryHandler< ::bapid::PingRequest, ::bapid::PingReply>(
            [this](
                   ::grpc::CallbackServerContext* context, const ::bapid::PingRequest* request, ::bapid::PingReply* response) { return this->Ping(context, request, response); }));}
    void SetMessageAllocatorFor_Ping(
        ::grpc::MessageAllocator< ::bapid::PingRequest, ::bapid::PingReply>* allocator) {
      ::grpc::internal::MethodHandler* const handler = ::grpc::Service::GetHandler(0);
      static_cast<::grpc::internal::CallbackUnaryHandler< ::bapid::PingRequest, ::bapid::PingReply>*>(handler)
              ->SetMessageAllocator(allocator);
    }
    ~WithCallbackMethod_Ping() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Ping(::grpc::ServerContext* /*context*/, const ::bapid::PingRequest* /*request*/, ::bapid::PingReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual ::grpc::ServerUnaryReactor* Ping(
      ::grpc::CallbackServerContext* /*context*/, const ::bapid::PingRequest* /*request*/, ::bapid::PingReply* /*response*/)  { return nullptr; }
  };
  typedef WithCallbackMethod_Ping<Service > CallbackService;
  typedef CallbackService ExperimentalCallbackService;
  template <class BaseClass>
  class WithGenericMethod_Ping : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithGenericMethod_Ping() {
      ::grpc::Service::MarkMethodGeneric(0);
    }
    ~WithGenericMethod_Ping() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Ping(::grpc::ServerContext* /*context*/, const ::bapid::PingRequest* /*request*/, ::bapid::PingReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithRawMethod_Ping : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawMethod_Ping() {
      ::grpc::Service::MarkMethodRaw(0);
    }
    ~WithRawMethod_Ping() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Ping(::grpc::ServerContext* /*context*/, const ::bapid::PingRequest* /*request*/, ::bapid::PingReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestPing(::grpc::ServerContext* context, ::grpc::ByteBuffer* request, ::grpc::ServerAsyncResponseWriter< ::grpc::ByteBuffer>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(0, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithRawCallbackMethod_Ping : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawCallbackMethod_Ping() {
      ::grpc::Service::MarkMethodRawCallback(0,
          new ::grpc::internal::CallbackUnaryHandler< ::grpc::ByteBuffer, ::grpc::ByteBuffer>(
            [this](
                   ::grpc::CallbackServerContext* context, const ::grpc::ByteBuffer* request, ::grpc::ByteBuffer* response) { return this->Ping(context, request, response); }));
    }
    ~WithRawCallbackMethod_Ping() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Ping(::grpc::ServerContext* /*context*/, const ::bapid::PingRequest* /*request*/, ::bapid::PingReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual ::grpc::ServerUnaryReactor* Ping(
      ::grpc::CallbackServerContext* /*context*/, const ::grpc::ByteBuffer* /*request*/, ::grpc::ByteBuffer* /*response*/)  { return nullptr; }
  };
  template <class BaseClass>
  class WithStreamedUnaryMethod_Ping : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithStreamedUnaryMethod_Ping() {
      ::grpc::Service::MarkMethodStreamed(0,
        new ::grpc::internal::StreamedUnaryHandler<
          ::bapid::PingRequest, ::bapid::PingReply>(
            [this](::grpc::ServerContext* context,
                   ::grpc::ServerUnaryStreamer<
                     ::bapid::PingRequest, ::bapid::PingReply>* streamer) {
                       return this->StreamedPing(context,
                         streamer);
                  }));
    }
    ~WithStreamedUnaryMethod_Ping() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable regular version of this method
    ::grpc::Status Ping(::grpc::ServerContext* /*context*/, const ::bapid::PingRequest* /*request*/, ::bapid::PingReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    // replace default version of method with streamed unary
    virtual ::grpc::Status StreamedPing(::grpc::ServerContext* context, ::grpc::ServerUnaryStreamer< ::bapid::PingRequest,::bapid::PingReply>* server_unary_streamer) = 0;
  };
  typedef WithStreamedUnaryMethod_Ping<Service > StreamedUnaryService;
  typedef Service SplitStreamedService;
  typedef WithStreamedUnaryMethod_Ping<Service > StreamedService;
};

}  // namespace bapid


#endif  // GRPC_if_2fbapid_2eproto__INCLUDED
