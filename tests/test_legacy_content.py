"""Python wrapper that compiles and runs the legacy content adapter C tests.

The C binary owns all assertions and exits nonzero on any failure. The Python
wrapper owns the one-time build (setUpClass) and converts the process exit
status into a unittest failure, including captured output in the report.
"""
import os
import shlex
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class LegacyContentTests(unittest.TestCase):
    """Compiles and runs the asset-free legacy content adapter C test binary."""

    @classmethod
    def setUpClass(cls):
        cls.build = tempfile.TemporaryDirectory(prefix="w3r-legacy-build-")
        cls.addClassCleanup(cls.build.cleanup)
        flags = ["-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Wconversion",
                 "-Wshadow", "-Werror", "-g"]
        if os.environ.get("SANITIZE") == "1":
            flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
        cc = shlex.split(os.environ.get("CC", "gcc"))
        sources = [
            str(ROOT / "tests" / "legacy_content_tests.c"),
            str(ROOT / "src" / "engine" / "legacy_content.c"),
            str(ROOT / "src" / "platform" / "data_file.c"),
        ]
        binary = Path(cls.build.name) / "legacy-content-tests"
        subprocess.run(cc + flags + [
            "-I" + str(ROOT / "src"),
            "-I" + str(ROOT / "src" / "engine"),
            *sources,
            "-o", str(binary),
        ], check=True)
        cls.binary = binary

    def test_legacy_content_adapter(self):
        """Compile and run the adapter C tests; fail on nonzero exit status."""
        result = subprocess.run([str(self.binary)], capture_output=True, text=True, timeout=300)
        self.assertEqual(
            result.returncode, 0,
            f"legacy_content_tests.c exited with {result.returncode}. "
            f"stdout={result.stdout[:2000]} stderr={result.stderr[:2000]}",
        )


if __name__ == "__main__":
    unittest.main()
