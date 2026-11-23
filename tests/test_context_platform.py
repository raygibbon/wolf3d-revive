"""SDL dummy-driver platform context tests."""
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
        library = sdk / "build" / "libSDL3.a"
        header = sdk / "include" / "SDL3" / "SDL.h"
        missing = [str(path) for path in (library, header) if not path.is_file()]
        if missing:
            raise unittest.SkipTest("SDL SDK unavailable: " + ", ".join(missing))
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
        output_path = None
        with tempfile.TemporaryDirectory(prefix="w3r-ctx-platform-prefs-") as output:
            output_path = Path(output)
            env = dict(
                os.environ,
                SDL_VIDEODRIVER="dummy",
                SDL_RENDER_DRIVER="software",
                SDL_AUDIODRIVER="dummy",
                XDG_DATA_HOME=output,
            )
            result = subprocess.run(
                [str(self.binary), output], env=env, capture_output=True, text=True
            )
            if result.returncode != 0:
                self.fail(
                    f"context platform executable failed with {result.returncode}\n"
                    f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
                )
            self.assertIn("context_platform_tests: PASS", result.stdout)
            screenshots = sorted(output_path.rglob("screenshot-*.bmp"))
            by_directory = {}
            for screenshot in screenshots:
                self.assertGreater(screenshot.stat().st_size, 0)
                by_directory.setdefault(screenshot.parent.name, 0)
                by_directory[screenshot.parent.name] += 1
            # alt-game: 3 (active context screenshots)
            self.assertEqual(by_directory.get("alt-game"), 3)
            # override-game: 1 (override screenshot, must not change active context)
            self.assertEqual(by_directory.get("override-game"), 1)
            # wolf3d-revive: 2 (default context after restart)
            self.assertEqual(by_directory.get("wolf3d-revive"), 2)

        self.assertIsNotNone(output_path)
        self.assertFalse(output_path.exists(), "temporary screenshot directory was not cleaned")
