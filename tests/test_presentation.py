"""Immutable interpolation, independent render rates and exact simulation state."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest
from test_engine import ROOT
from game_fixtures import finale_files

class PresentationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.build=tempfile.TemporaryDirectory(prefix='w3r-present-');cls.addClassCleanup(cls.build.cleanup)
        sdk=Path(os.environ.get('SDL_SDK',ROOT.parent/'cart/vendor/SDL'))
        cls.cc=shlex.split(os.environ.get('CC','gcc'))
        cls.flags=['-std=c11','-Wall','-Wextra','-Wpedantic','-Wconversion','-Wshadow','-Werror','-g']
        if os.environ.get('SANITIZE')=='1': cls.flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        cls.clock=Path(cls.build.name)/'clock';cls.binary=Path(cls.build.name)/'presentation'
        subprocess.run(cls.cc+cls.flags+['-I'+str(ROOT/'src'),str(ROOT/'tests/frame_clock_tests.c'),str(ROOT/'src/engine/WL_FRAME_CLOCK.c'),'-o',str(cls.clock)],check=True)
        sources=sorted(p for p in (ROOT/'src').rglob('*.c') if p.name!='main.c')
        subprocess.run(cls.cc+cls.flags+['-I'+str(ROOT/'src'),'-I'+str(sdk/'include'),str(ROOT/'tests/presentation_tests.c'),*map(str,sources),str(sdk/'build/libSDL3.a'),'-pthread','-lm','-o',str(cls.binary)],check=True)
    def test_clock(self): subprocess.run([str(self.clock)],check=True)
    def run_world(self,owned):
        with tempfile.TemporaryDirectory(prefix='w3r-present-data-') as directory:
            root=Path(directory);data=root/'data';data.mkdir()
            if owned:
                if not os.environ.get('WOLF3D_DATA'): self.skipTest('Owned WL6 data unavailable')
                data=Path(os.environ['WOLF3D_DATA'])
            else: finale_files(data)
            subprocess.run([str(self.binary),str(data),str(root)],check=True)
            payloads=[(root/f'slot{i+1}.w3s').read_bytes()[96:] for i in range(5)]
            for payload in payloads[1:]: self.assertEqual(payload,payloads[0])
    def test_synthetic(self): self.run_world(False)
    def test_owned(self): self.run_world(True)
