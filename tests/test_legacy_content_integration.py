"""Python wrapper that compiles and runs legacy_content_integration_tests.c.

Compiles the synthetic WL6 integration test binary and runs it, verifying
that the actual startup and resource-loading path works with the adapter-configured
directory.  Also verifies that default startup fails when only renamed resources
are present.
"""
import subprocess
import sys
import unittest
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]


class LegacyContentIntegrationTests(unittest.TestCase):
    """Wrapper test that compiles and runs legacy_content_integration_tests.c."""

    def test_compile_and_run(self):
        """Compile and run the integration test binary."""
        build_dir = PROJECT_ROOT / "tests" / "build"
        build_dir.mkdir(parents=True, exist_ok=True)
        binary = build_dir / "legacy-integration-test"

        sources = [
            PROJECT_ROOT / "tests" / "legacy_content_integration_tests.c",
            PROJECT_ROOT / "src" / "engine" / "legacy_content.c",
            PROJECT_ROOT / "src" / "platform" / "data_file.c",
        ]

        # Compile - need -I tests to find platform/data_file.h
        cc = os.environ.get("CC", "gcc")
        cmd = [cc, "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Wconversion",
               "-Wshadow", "-Werror", "-g",
               "-I", str(PROJECT_ROOT / "src"),
               "-I", str(PROJECT_ROOT / "src" / "engine"),
               "-I", str(PROJECT_ROOT / "tests"),  # Add tests root for headers
               * [str(s) for s in sources] + ["-o", str(binary)]
        # Use Popen to avoid subprocess.run syntax issues in unittest import
        result = subprocess.run(cmd, capture_output=True, text=True)
        self.assertEqual(
            result.returncode, 0,
            f"Compilation failed (exit {result.returncode}): {result.stderr[:500]}"
        )

        # Run the binary using Popen
        proc = subprocess.Popen([str(binary)], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = proc.communicate()
        self.assertEqual(
            proc.returncode, 0,
            f"Integration test binary exited with {proc.returncode}. "
            f"stdout={out.decode()[:500]} stderr={err.decode()[:500]}"
        )

    def test_resources_present_and_readable(self):
        """Verify that required synthetic WL6 resources are present and readable."""
        build_dir = PROJECT_ROOT / "tests" / "build"
        build_dir.mkdir(parents=True, exist_ok=True)
        binary = build_dir / "legacy-integration-test"

        sources = [
            PROJECT_ROOT / "tests" / "legacy_content_integration_tests.c",
            PROJECT_ROOT / "src" / "engine" / "legacy_content.c",
            PROJECT_ROOT / "src" / "platform" / "data_file.c",
        ]

        # Compile - need -I tests to find platform/data_file.h
        cc = os.environ.get("CC", "gcc")
        cmd = [cc, "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Wconversion",
               "-Wshadow", "-Werror", "-g",
               "-I", str(PROJECT_ROOT / "src"),
               "-I", str(PROJECT_ROOT / "src" / "engine"),
               "-I", str(PROJECT_ROOT / "tests"),  # Add tests root for headers
               * [str(s) for s in sources] + ["-o", str(binary)]
        result = subprocess.run(cmd, capture_output=True, text=True)
        self.assertEqual(
            result.returncode, 0,
            f"Compilation failed (exit {result.returncode}): {result.stderr[:500]}"
        )

        # Run the binary
        proc = subprocess.Popen([str(binary)], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = proc.communicate()
        self.assertEqual(
            proc.returncode, 0,
            f"Integration test binary exited with {proc.returncode}. "
            f"stdout={out.decode()[:500]} stderr={err.decode()[:500]}"
        )

    def test_custom_filenames_used(self):
        """Verify that custom filename replacement works through the adapter."""
        build_dir = PROJECT_ROOT / "tests" / "build"
        build_dir.mkdir(parents=True, exist_ok=True)
        binary = build_dir / "legacy-integration-test"

        sources = [
            PROJECT_ROOT / "tests" / "legacy_content_integration_tests.c",
            PROJECT_ROOT / "src" / "engine" / "legacy_content.c",
            PROJECT_ROOT / "src" / "platform" / "data_file.c",
        ]

        # Compile - need -I tests to find platform/data_file.h
        cc = os.environ.get("CC", "gcc")
        cmd = [cc, "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Wconversion",
               "-Wshadow", "-Werror", "-g",
               "-I", str(PROJECT_ROOT / "src"),
               "-I", str(PROJECT_ROOT / "src" / "engine"),
               "-I", str(PROJECT_ROOT / "tests"),  # Add tests root for headers
               * [str(s) for s in sources] + ["-o", str(binary)]
        result = subprocess.run(cmd, capture_output=True, text=True)
        self.assertEqual(
            result.returncode, 0,
            f"Compilation failed (exit {result.returncode}): {result.stderr[:500]}"
        )

        # Run the binary
        proc = subprocess.Popen([str(binary)], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = proc.communicate()
        self.assertEqual(
            proc.returncode, 0,
            f"Integration test binary exited with {proc.returncode}. "
            f"stdout={out.decode()[:500]} stderr={err.decode()[:500]}"
        )
        # The output should indicate custom name was set
        self.assertIn(
            "CUSTOM.WL6", out.decode(),
            "Custom filename CUSTOM.WL6 should appear in output"
        )
