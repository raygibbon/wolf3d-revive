"""Headless SDL platform context tests."""
import os
import shlex
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class ContextPlatformTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        sdk = Path(os.environ.get("SDL_SDK", ROOT.parent / "cart" / "vendor" / "SDL"))
        if not (sdk / "build" / "libSDL3.a").is_file():
            raise unittest.SkipTest("SDL SDK unavailable")
        cls.build = tempfile.TemporaryDirectory(prefix="w3r-ctx-platform-build-")
        cls.addClassCleanup(cls.build.cleanup)
        cls.binary = Path(cls.build.name) / "context-platform-tests"
        flags = ["-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Wconversion", "-Wshadow", "-Werror", "-g"]
        if os.environ.get("SANITIZE") == "1":
            flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
        sources = [str(p) for p in sorted((ROOT / "src").rglob("*.c")) if p.name != "main.c"]
        subprocess.run(
            shlex.split(os.environ.get("CC", "gcc"))
            + flags
            + ["-I" + str(ROOT / "src"), "-I" + str(sdk / "include"),
               str(ROOT / "tests" / "context_platform_tests.c"), *sources, str(sdk / "build" / "libSDL3.a"), "-pthread", "-lm", "-o", str(cls.binary)],
            check=True,
        )

    def test_context_platform(self):
        env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_RENDER_DRIVER="software", SDL_AUDIODRIVER="dummy")
        result = subprocess.run([str(self.binary)], env=env, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("PASS", result.stdout)