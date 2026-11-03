"""SDL display transitions, readback and gameplay isolation."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest
from test_engine import ROOT
from game_fixtures import finale_files

class DisplayTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        sdk = Path(os.environ.get('SDL_SDK', ROOT.parent / 'cart/vendor/SDL'))
        if not (sdk / 'build/libSDL3.a').is_file(): raise unittest.SkipTest('SDL SDK unavailable')
        cls.build = tempfile.TemporaryDirectory(prefix='w3r-display-build-')
        cls.addClassCleanup(cls.build.cleanup)
        cls.binary = Path(cls.build.name) / 'display-tests'
        flags = ['-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Wconversion', '-Wshadow', '-Werror', '-g', '-DWOLF3D_DISPLAY_TEST']
        if os.environ.get('SANITIZE') == '1': flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        sources = sorted(p for p in (ROOT / 'src').rglob('*.c') if p.name != 'main.c')
        subprocess.run(shlex.split(os.environ.get('CC', 'gcc')) + flags +
                       ['-I' + str(ROOT / 'src'), '-I' + str(sdk / 'include'), str(ROOT / 'tests/display_tests.c'),
                        *map(str, sources), str(sdk / 'build/libSDL3.a'), '-pthread', '-lm', '-o', str(cls.binary)], check=True)

    def run_display(self, mode, owned=False):
        with tempfile.TemporaryDirectory(prefix='w3r-display-data-') as directory:
            root = Path(directory); data = root / 'data'; data.mkdir(); slots = root / 'slots'; slots.mkdir()
            if owned:
                if not os.environ.get('WOLF3D_DATA'): self.skipTest('Owned WL6 data not supplied')
                data = Path(os.environ['WOLF3D_DATA'])
            else: finale_files(data)
            env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software', SDL_AUDIODRIVER='dummy')
            subprocess.run([str(self.binary), str(data), str(slots), mode], env=env, check=True)
            if mode == 'game': self.assertEqual((slots / 'slot1.w3s').read_bytes()[96:], (slots / 'slot2.w3s').read_bytes()[96:])

    def test_geometry_and_failures(self): self.run_display('geometry')
    def test_display_gameplay(self): self.run_display('game')
    def test_owned_display(self): self.run_display('game', owned=True)

    def test_aspect_transitions(self): self.run_display('transitions')
    def test_owned_aspect_transitions(self): self.run_display('transitions', owned=True)
