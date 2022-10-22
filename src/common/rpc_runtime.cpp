#include "src/common/rpc_runtime.h"

namespace bapid {

IHandlerRecord::IHandlerRecord(ProcessFn process_fn,
                               ReceivingNextRequest receiving_next_request_fn,
                               BindRuntimeFn bind_runtime_fn)
    : process_fn{std::move(process_fn)}, receiving_next_request_fn{std::move(
                                             receiving_next_request_fn)},
      bind_runtime_fn{std::move(bind_runtime_fn)} {}

HandlerState::HandlerState(RpcRuntimeCtx ctx, IHandlerRecord *record)
    : ctx{ctx}, record{record} {}

void HandlerState::receivingNextRequest() {
  auto data = record->receiving_next_request_fn(this);
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
    record->process_fn(call_data).via(ctx.executor);
  } else {
    inflight_call_data.erase(call_data->it);
  }
}

std::vector<std::unique_ptr<HandlerState>>
IRpcHanlderRegistry::bindRuntime(RpcRuntimeCtx &runtime_ctx) {
  std::vector<std::unique_ptr<HandlerState>> states{};
  std::for_each(hanlder_records_.begin(), hanlder_records_.end(),
                [&](auto &record) {
                  states.emplace_back(record->bind_runtime_fn(runtime_ctx));
                });

  return states;
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
