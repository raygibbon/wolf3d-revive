"""Resolution-independent indexed rendering, SDL composition and isolation."""
import os
import hashlib
import json
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest
from test_engine import ROOT
from game_fixtures import finale_files

class RenderTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.build = tempfile.TemporaryDirectory(prefix='w3r-render-build-')
        cls.addClassCleanup(cls.build.cleanup)
        cls.flags = ['-std=c11','-Wall','-Wextra','-Wpedantic','-Wconversion','-Wshadow','-Werror','-g']
        if os.environ.get('SANITIZE') == '1': cls.flags += ['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        cls.cc = shlex.split(os.environ.get('CC','gcc'))
        cls.target = Path(cls.build.name)/'target'
        subprocess.run(cls.cc+cls.flags+['-I'+str(ROOT/'src'),str(ROOT/'tests/render_target_tests.c'),str(ROOT/'src/engine/WL_RENDER_TARGET.c'),'-o',str(cls.target)],check=True)
        sdk = Path(os.environ.get('SDL_SDK',ROOT.parent/'cart/vendor/SDL'))
        cls.binary = Path(cls.build.name)/'render'
        sources = sorted(p for p in (ROOT/'src').rglob('*.c') if p.name!='main.c')
        subprocess.run(cls.cc+cls.flags+['-DWOLF3D_DISPLAY_TEST','-I'+str(ROOT/'src'),'-I'+str(sdk/'include'),str(ROOT/'tests/render_tests.c'),*map(str,sources),str(sdk/'build/libSDL3.a'),'-pthread','-lm','-o',str(cls.binary)],check=True)

    def test_original_framebuffer_hashes(self):
        if not os.environ.get('WOLF3D_DATA'): self.skipTest('Owned data unavailable')
        data=Path(os.environ['WOLF3D_DATA'])
        identities=json.loads((ROOT/'tests/fixtures/dos_demo_hashes.json').read_text())['assets']
        files={p.name.upper():p for p in data.iterdir()}
        for name,identity in identities.items():
            if name not in files or hashlib.sha256(files[name].read_bytes()).hexdigest()!=identity:
                self.skipTest('Different original data revision')
        binary=Path(self.build.name)/'original'
        sources=sorted((ROOT/'src/engine').glob('*.c'))
        subprocess.run(self.cc+self.flags+['-I'+str(ROOT/'src'),str(ROOT/'tests/render_original_tests.c'),*map(str,sources),str(ROOT/'src/platform/data_file.c'),str(ROOT/'src/platform/save_file.c'),'-lm','-o',str(binary)],check=True)
        output=subprocess.check_output([str(binary),str(data)],text=True)
        self.assertEqual(output,(ROOT/'tests/fixtures/original_render_hashes.csv').read_text())

    def test_target(self): subprocess.run([str(self.target)],check=True)
    def run_render(self,owned=False):
        with tempfile.TemporaryDirectory(prefix='w3r-render-data-') as directory:
            root=Path(directory);data=root/'data';data.mkdir()
            if owned:
                if not os.environ.get('WOLF3D_DATA'): self.skipTest('Owned data unavailable')
                data=Path(os.environ['WOLF3D_DATA'])
            else: finale_files(data)
            env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_RENDER_DRIVER='software',SDL_AUDIODRIVER='dummy')
            subprocess.run([str(self.binary),str(data),str(root),"owned" if owned else "synthetic"],env=env,check=True)
            self.assertEqual((root/"slot1.w3s").read_bytes()[96:],(root/"slot2.w3s").read_bytes()[96:])
    def test_render_and_settings(self): self.run_render()
    def test_owned_render(self): self.run_render(True)
