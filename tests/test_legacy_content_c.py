"""Python wrapper that compiles and runs legacy_content_tests.c.

Compiles the C adapter test binary and runs it, checking the process
exit code to determine pass/fail.  A nonzero exit code means any
assertion failed.
"""
import os
import subprocess
import sys
import unittest
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]


class LegacyContentCTests(unittest.TestCase):
    """Wrapper test that compiles and runs legacy_content_tests.c."""

    def test_compile_and_run(self):
        """Compile legacy_content_tests.c and run it; check exit code 0."""
        build_dir = PROJECT_ROOT / "tests" / "build"
        build_dir.mkdir(parents=True, exist_ok=True)
        binary = build_dir / "legacy-c-tests"

        sources = [
            PROJECT_ROOT / "tests" / "legacy_content_tests.c",
            PROJECT_ROOT / "src" / "engine" / "legacy_content.c",
            PROJECT_ROOT / "src" / "platform" / "data_file.c",
            PROJECT_ROOT / "src" / "platform" / "save_file.c",
        ]

        # Compile
        cc = os.environ.get("CC", "gcc")
        cmd = [cc, "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Wconversion",
               "-Wshadow", "-Werror", "-g",
               "-I", str(PROJECT_ROOT / "src"),
               "-I", str(PROJECT_ROOT / "src" / "engine")] + [str(s) for s in sources] + ["-o", str(binary)]
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            self.fail(f"Compilation failed (exit {result.returncode}): {result.stderr[:500]}")

        # Run the binary
        run_result = subprocess.run([str(binary)], capture_output=True, text=True)
        self.assertEqual(
            run_result.returncode, 0,
            f"legacy_content_tests.c exited with {run_result.returncode}. "
            f"stdout={run_result.stdout[:500]} stderr={run_result.stderr[:500]}"
        )
