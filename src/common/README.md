# Rpc
This is a toy wrapper for building simple gRPC services. With it, an user only need minimum code to
bootstrap a gRPC service and focus on implementing their business logic.

## Glossary
### gRPC service
A gRPC service is a collection of public facing APIs (methods), whose interface is defined in 
protobuf. A service is usually responsible for some logically self-contained business logic. For 
example, the "Greeter" service has a "hi" method that takes a `name: string` and returns 
`message: string`.

### gRPC server
A gRPC server is a deployment (i.e. a process) of a concrete implementation of the gRpc service. It
handles the calls from the clients of this gRPC service. For example, we implement the "Greeter"
service in C++ and deploy it at `localhost:50051`.

### Handler
A handler is the implementation of a gRPC method. For example, in our C++ implementation of "Greeter",
the handler for the "hi" method returns `{message: "hi {request.name}"}`.
```
std::string GreeterHandlers::hi(const std::string& name) {
  return "hi " + name;
}
```

### `RpcServerBase`
`RpcServerBase` is the base class of a gRPC server.

### `RpcHanlderRegistry`
`RpcHanlderRegistry` is a container of all handlers of a server. A server has exactly one registry.
The registry is used to set up ("bind") runtimes for handling gRPC calls.
```
class GreeterHandlerRegistry registry; // templated `RpcHanlderRegistry`
registry.registerHandler<&GreeterService::AsyncService::RequestHi>(&GreeterHandlers::hi);
// now can use registry to set up runtimes
```

### `RpcServiceRuntime`
A `RpcServerBase` has one or more threads listening for requests. Each thread is driven by an instance
of `RpcServiceRuntime`. The runtime is responsible for dequeuing incoming calls, call their handlers, 
and returns the responses.

### "Bind"
"Bind a handler to a runtime" means that we set up the runtime so that it will invoke this handler
to handle calls of its method. "Bind a registry to a runtime" means that we set up the runtime so
that it will invoke handlers in this registry when handling corresponding calls of the method. 
For example, when creating a runtime for our Greeter service, we use `registry.bindRuntime`.

