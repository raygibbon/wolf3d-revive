"""Python wrapper that compiles and runs the legacy content production-loader
integration test binary against synthetic WL6 fixtures.

The C harness exercises the real CA_Startup / CA_StartupWithContent /
CA_ResourceName / CA_Shutdown path. The Python wrapper generates the synthetic
WL6 fixture directories (original and renamed graphics names), builds the C
harness once per class and converts the harness exit status into a unittest
result, including captured output in the failure report.
"""
import os
import shlex
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TESTS_DIR = ROOT / "tests"
if str(TESTS_DIR) not in sys.path:
    sys.path.insert(0, str(TESTS_DIR))


class LegacyContentIntegrationTests(unittest.TestCase):
    """Compiles and runs the synthetic WL6 production-loader integration tests."""

    @classmethod
    def setUpClass(cls):
        cls.build = tempfile.TemporaryDirectory(prefix="w3r-legacy-integration-build-")
        cls.addClassCleanup(cls.build.cleanup)
        flags = ["-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Wconversion",
                 "-Wshadow", "-Werror", "-g"]
        if os.environ.get("SANITIZE") == "1":
            flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
        cc = shlex.split(os.environ.get("CC", "gcc"))
        sources = [
            str(ROOT / "tests" / "legacy_content_integration_tests.c"),
        ]
        sources += [str(p) for p in sorted((ROOT / "src" / "engine").glob("*.c"))]
        sources += [
            str(ROOT / "src" / "platform" / "data_file.c"),
            str(ROOT / "src" / "platform" / "save_file.c"),
        ]
        binary = Path(cls.build.name) / "legacy-content-integration"
        subprocess.run(cc + flags + [
            "-I" + str(ROOT / "src"),
            "-I" + str(ROOT / "src" / "engine"),
            *sources,
            "-lm",
            "-o", str(binary),
        ], check=True)
        cls.binary = binary

        cls.original_dir = Path(tempfile.mkdtemp(prefix="w3r-legacy-fixture-orig-"))
        cls.renamed_dir = Path(tempfile.mkdtemp(prefix="w3r-legacy-fixture-ren-"))
        cls.scratch_dir = Path(tempfile.mkdtemp(prefix="w3r-legacy-fixture-scratch-"))

        def cleanup():
            for path in (cls.original_dir, cls.renamed_dir, cls.scratch_dir):
                shutil.rmtree(path, ignore_errors=True)

        cls.addClassCleanup(cleanup)

        from game_fixtures import game_files  # noqa: E402  (tests dir added to sys.path above)

        game_files(cls.original_dir)
        game_files(cls.renamed_dir)
        for old, new in (("VGADICT.WL6", "GFXDICT.WL6"),
                        ("VGAHEAD.WL6", "GFXHEAD.WL6"),
                        ("VGAGRAPH.WL6", "GFXGRAPH.WL6")):
            os.rename(cls.renamed_dir / old, cls.renamed_dir / new)

    def test_production_loader_integration(self):
        """Run the real production-loader integration harness; fail on nonzero exit."""
        result = subprocess.run(
            [str(self.binary), str(self.original_dir), str(self.renamed_dir), str(self.scratch_dir)],
            capture_output=True, text=True, timeout=600,
        )
        self.assertEqual(
            result.returncode, 0,
            f"legacy content integration harness exited with {result.returncode}. "
            f"stdout={result.stdout[:2000]} stderr={result.stderr[:4000]}",
        )


if __name__ == "__main__":
    unittest.main()
