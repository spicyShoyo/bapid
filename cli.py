from collections.abc import Callable
import sys
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

if __name__ == "__main__":
    if len(sys.argv) == 1:
        print('cmd: ', REGISTRY.keys())
        quit()
    cmd = sys.argv[1]
    if cmd not in REGISTRY:
        quit()

    REGISTRY[cmd](sys.argv[2:])
