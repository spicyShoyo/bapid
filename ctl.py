from collections.abc import Callable
import sys
import subprocess
from subprocess import check_call

GRPC_ADDR = "localhost:50051"

REGISTRY: dict[str, Callable[[list[str]], None]] = {}
def register(cmd):
    def inner(func):
        REGISTRY[cmd] = func
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
def run(*_):
    check_call(["./dev_scripts/run"])

@register("dbg")
def dbg(*_):
    check_call(["./dev_scripts/dbg"])

@register("refresh")
def refresh(args):
    check_call(["./dev_scripts/refresh"] + args)


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

