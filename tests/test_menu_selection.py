"""Integration tests for the real WL_MENU episode/difficulty consumer."""
import os
import shlex
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class MenuSelectionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.build = tempfile.TemporaryDirectory(prefix="w3r-menu-selection-build-")
        cls.addClassCleanup(cls.build.cleanup)
        cls.binary = Path(cls.build.name) / "menu-selection-tests"
        flags = ["-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Wconversion", "-Wshadow", "-Werror", "-g"]
        if os.environ.get("SANITIZE") == "1":
            flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
        sources = [
            str(ROOT / "src" / "engine" / "WL_MENU.c"),
            str(ROOT / "src" / "engine" / "game_definition.c"),
        ]
        subprocess.run(
            shlex.split(os.environ.get("CC", "gcc"))
            + flags
            + ["-I" + str(ROOT / "src"), "-I" + str(ROOT / "src" / "engine"),
               str(ROOT / "tests" / "menu_selection_tests.c"), *sources, "-o", str(cls.binary)],
            check=True,
        )

    def test_menu_selection(self):
        result = subprocess.run([str(self.binary)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("menu_selection_tests: PASS", result.stdout)
