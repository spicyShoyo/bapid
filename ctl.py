from collections.abc import Callable
import sys
import subprocess
from subprocess import check_call

GRPC_ADDR = "localhost:50051"

REGISTRY: dict[str, Callable[[list[str]], None]] = {}
def register(cmd):
    def inner(func):
        REGISTRY[cmd] = func
        return func
    return inner

@register("test")
def test(*_):
    check_call(["echo", "test"])

@register("list")
def list(*_):
    check_call(["grpc_cli", "list", GRPC_ADDR])

@register("ping")
def ping(*_):
    check_call(["grpc_cli", "call", GRPC_ADDR, "Ping", "name: 'ok'"])

@register("shutdown")
def shutdown(*_):
    check_call(["grpc_cli", "call", GRPC_ADDR, "Shutdown", ""])

# Local dev commands
@register("run")
@register("r")
def run(args):
    check_call(["./dev_scripts/run"] + args)

@register("t")
def run_test(args):
    check_call(["./dev_scripts/run_tests"] + args)

@register("dbg")
def dbg(*_):
    check_call(["./dev_scripts/dbg"])

@register("refresh")
def refresh(args):
    check_call(["./dev_scripts/refresh"] + args)

@register("c")
def test_rpc(*_):
    print(check_call(["curl", "localhost:8000"]))
    check_call(["grpc_cli", "call", GRPC_ADDR, "Ping", "name: 'ok'"])
    check_call(["grpc_cli", "call", GRPC_ADDR, "Ping", "name: 'ok'"])
    check_call(["grpc_cli", "call", GRPC_ADDR, "Ping", "name: 'ok'"])
    check_call(["grpc_cli", "call", GRPC_ADDR, "Shutdown", ""])


@register("a")
def test_arrow(*_):
    check_call(["grpc_cli", "call", GRPC_ADDR, "ArrowTest", ""])
    check_call(["grpc_cli", "call", GRPC_ADDR, "Shutdown", ""])


if __name__ == "__main__":
    if len(sys.argv) == 1:
        print('cmd: ', REGISTRY.keys())
        quit()
    cmd = sys.argv[1]
    if cmd not in REGISTRY:
        quit()
    
    try:
        REGISTRY[cmd](sys.argv[2:])
    except KeyboardInterrupt:
        pass
    except subprocess.CalledProcessError:
        pass

