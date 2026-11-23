"""Legacy content adapter headless tests.

Asset-free: no original Wolfenstein data required. Synthetic fixtures only.
Runs as part of the R1.4 focused test suite.
Compiles and runs the C adapter test binary.
"""
import os
import shlex
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class LegacyContentTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.build = tempfile.TemporaryDirectory(prefix="w3r-legacy-build-")
        cls.addClassCleanup(cls.build.cleanup)
        flags = ["-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Wconversion",
                 "-Wshadow", "-Werror", "-g"]
        if os.environ.get("SANITIZE") == "1":
            flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
        cc = shlex.split(os.environ.get("CC", "gcc"))
        # Only compile files needed for the legacy content adapter;
        # avoid pulling in game-specific symbols (gamestate, OPL, etc.).
        sources = [
            str(ROOT / "tests" / "legacy_content_tests.c"),
            str(ROOT / "src" / "engine" / "legacy_content.c"),
            str(ROOT / "src" / "platform" / "data_file.c"),
            str(ROOT / "src" / "platform" / "save_file.c"),
        ]
        binary = Path(cls.build.name) / "legacy-content-tests"
        subprocess.run(cc + flags +
                       ["-I" + str(ROOT / "src"), "-I" + str(ROOT / "src" / "engine"),
                        *sources, "-o", str(binary)],
                       check=True)
        cls.binary = binary

    def run_legacy_content_test(self):
        result = subprocess.run([str(self.binary)], capture_output=True, text=True)
        return result.returncode == 0, result.stdout + result.stderr

    def test_default_names(self):
        ok, output = self.run_legacy_content_test()
        self.assertEqual(ok, True, output)
        self.assertIn("PASS", output)

    def test_init_binds_directory(self):
        ok, output = self.run_legacy_content_test()
        self.assertEqual(ok, True, output)
        self.assertIn("PASS", output)

    def test_set_resource(self):
        ok, output = self.run_legacy_content_test()
        self.assertEqual(ok, True, output)
        self.assertIn("PASS", output)

    def test_validate_null(self):
        ok, output = self.run_legacy_content_test()
        self.assertEqual(ok, True, output)
        self.assertIn("PASS", output)

    def test_validate_assets_missing(self):
        ok, output = self.run_legacy_content_test()
        self.assertEqual(ok, True, output)
        self.assertIn("PASS", output)

    def test_error_strings(self):
        ok, output = self.run_legacy_content_test()
        self.assertEqual(ok, True, output)
        self.assertIn("PASS", output)