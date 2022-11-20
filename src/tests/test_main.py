import unittest
import time
import subprocess
from test_lib import *

class TestBasic(BaseTest):
    def test_basic(self):
        subprocess.Popen(["./bapid"])
        for _ in range(3):
            time.sleep(1)
            try:
                out = get_output_as_str(["curl", "-s", "localhost:8000"])
            except subprocess.CalledProcessError:
                continue
            self.assertEqual(out, "hi")
            break;
        else:
            self.assertTrue(False, "server not started")
        
        cmd = ["grpc_cli", "call", "--json_output", "localhost:50051", "Ping", "name: 'ok'"]
        for _ in range(3):
            out = get_output_as_json(cmd)
            self.assertEqual(out["message"], "hi: ok")

if __name__ == "__main__":
    unittest.main()
