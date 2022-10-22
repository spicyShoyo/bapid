#include "src/common/rpc_runtime.h"

namespace bapid {

HandlerState::HandlerState(grpc::ServerCompletionQueue *cq,
                           folly::Executor *executor)
    : cq{cq}, executor{executor} {}

void HandlerState::receivingNextRequest() {
  auto data = receiving_next_request_fn();
  data.swap(next_call_data);

  if (!data) {
    return;
  }

  inflight_call_data.emplace_back(std::move(data));
  inflight_call_data.back()->it = inflight_call_data.end();
  inflight_call_data.back()->it--;
}

void HandlerState::processCallData(CallDataBase *call_data) {
  if (!call_data->processed) {
    XCHECK(call_data == next_call_data.get());
    receivingNextRequest();

    call_data->processed = true;
    process_fn(call_data).via(executor);
  } else {
    inflight_call_data.erase(call_data->it);
  }
}

void RpcServiceRuntime::serve() const {
  void *tag{};
  bool ok{false};
  while (ctx_.cq->Next(&tag, &ok)) {
    if (!ok) {
      break;
    }

    auto *call_data = static_cast<CallDataBase *>(tag);
    call_data->state->processCallData(call_data);
  }
}

RpcServiceRuntime::RpcServiceRuntime(
    RpcRuntimeCtx ctx, std::unique_ptr<IRpcHanlderRegistry> &registry)
    : ctx_{ctx}, handler_states_{registry->bindRuntime(ctx_)} {}

RpcServiceRuntime::~RpcServiceRuntime() {
  void *ignored_tag{};
  bool ignored_ok{};
  while (ctx_.cq->Next(&ignored_tag, &ignored_ok)) {
  }
}
} // namespace bapid
