import unittest
import sys
import json
import subprocess
from typing import List, Any

def get_output_as_str(cmd: List[str]) -> str:
    return subprocess.check_output(cmd).decode(sys.stdout.encoding).strip()

def get_output_as_json(cmd: List[str]) -> Any:
    res = subprocess.check_output(cmd).decode(sys.stdout.encoding).strip()
    return json.loads(res)

class BaseTest(unittest.TestCase):
    def setUp(self) -> None:
        return super().setUp()

    def tearDown(self) -> None:
        return super().setUp()
