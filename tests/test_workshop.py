"""Headless live-Workshop edit, selection, and production-collision tests."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class WorkshopTests(unittest.TestCase):
    def test_workshop_core(self):
        with tempfile.TemporaryDirectory(prefix="w3r-workshop-") as directory:
            output = Path(directory) / "workshop-tests"
            flags = ["-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Wconversion",
                     "-Wshadow", "-Werror", "-g"]
            if os.environ.get("SANITIZE") == "1":
                flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
            sources = [str(path) for path in sorted((ROOT / "src/engine").glob("*.c"))]
            command = shlex.split(os.environ.get("CC", "gcc")) + flags + [
                "-I" + str(ROOT / "src"), str(ROOT / "tests/workshop_tests.c"), *sources,
                str(ROOT / "src/platform/data_file.c"), str(ROOT / "src/platform/save_file.c"),
                "-lm", "-o", str(output)]
            subprocess.run(command, check=True)
            result = subprocess.run([str(output)], text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("workshop edit/selection/collision tests passed", result.stdout)


if __name__ == "__main__":
    unittest.main()
