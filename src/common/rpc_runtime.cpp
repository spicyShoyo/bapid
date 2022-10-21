#include "src/common/rpc_runtime.h"

namespace bapid {
void RpcServiceRuntime::serve() const {
  void *tag{};
  bool ok{false};
  while (ctx_.cq->Next(&tag, &ok)) {
    if (!ok) {
      break;
    }

    auto *call_data = static_cast<CallDataBase *>(tag);
    call_data->state->proceed_fn(call_data);
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
