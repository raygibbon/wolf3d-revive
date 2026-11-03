"""DOS PreloadGraphics and Died presentation, synthetic and privately owned data."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest
from test_engine import ROOT
from game_fixtures import finale_files


class LoadingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        sdk = Path(os.environ.get('SDL_SDK', ROOT.parent / 'cart/vendor/SDL'))
        if not (sdk / 'build/libSDL3.a').is_file():
            raise unittest.SkipTest('SDL SDK unavailable')
        cls.build = tempfile.TemporaryDirectory(prefix='w3r-loading-')
        cls.addClassCleanup(cls.build.cleanup)
        cls.binary = Path(cls.build.name) / 'loading-tests'
        flags = ['-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Wconversion', '-Wshadow', '-Werror', '-g']
        if os.environ.get('SANITIZE') == '1':
            flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        sources = sorted(p for p in (ROOT / 'src').rglob('*.c') if p.name != 'main.c')
        subprocess.run(shlex.split(os.environ.get('CC', 'gcc')) + flags +
                       ['-I' + str(ROOT / 'src'), '-I' + str(sdk / 'include'), str(ROOT / 'tests/loading_tests.c'),
                        *map(str, sources), str(sdk / 'build/libSDL3.a'), '-pthread', '-lm', '-o', str(cls.binary)], check=True)

    def run_case(self, data, mode):
        with tempfile.TemporaryDirectory(prefix='w3r-loading-saves-') as saves:
            env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software', SDL_AUDIODRIVER='dummy')
            subprocess.run([str(self.binary), str(data), saves, mode], env=env, check=True, timeout=180)

    def test_synthetic_loading_and_death(self):
        with tempfile.TemporaryDirectory(prefix='w3r-loading-data-') as directory:
            data = Path(directory)
            finale_files(data)
            self.run_case(data, 'synthetic')

    def test_owned_loading_and_death(self):
        data = os.environ.get('WOLF3D_DATA')
        if not data:
            self.skipTest('Set WOLF3D_DATA to legally owned WL6 data')
        self.run_case(data, 'owned')
