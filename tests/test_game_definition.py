"""Headless game definition interface tests."""
import os
import shlex
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class GameDefinitionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.build = tempfile.TemporaryDirectory(prefix="w3r-gamedef-build-")
        cls.addClassCleanup(cls.build.cleanup)
        cls.binary = Path(cls.build.name) / "game-definition-tests"
        flags = ["-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Wconversion", "-Wshadow", "-Werror", "-g"]
        if os.environ.get("SANITIZE") == "1":
            flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
        sources = [str(ROOT / "src" / "engine" / "game_definition.c")]
        subprocess.run(
            shlex.split(os.environ.get("CC", "gcc"))
            + flags
            + ["-I" + str(ROOT / "src"), str(ROOT / "tests" / "game_definition_tests.c"), *sources, "-o", str(cls.binary)],
            check=True,
        )

    def test_game_definition(self):
        result = subprocess.run([str(self.binary)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("PASS", result.stdout)