"""Classic DOS viewport, function keys, boss key and embedded demos."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest
from test_engine import ROOT
from game_fixtures import finale_files


def classic_files(directory, mode="normal"):
    import struct
    from test_engine import encode
    finale_files(directory)
    h=(directory/'VGAHEAD.WL6').read_bytes(); g=(directory/'VGAGRAPH.WL6').read_bytes()
    offsets=[int.from_bytes(h[i:i+3],'little') for i in range(0,450,3)]
    chunks={n:g[a:next(b for b in offsets[n+1:] if b!=0xffffff)]
            for n,a in enumerate(offsets[:-1]) if a!=0xffffff}
    raw={}
    for n in ([] if mode=="missing" else range(139,143)):
        packets=bytes([0,0,221, 2,35,0, 16,0,0, 0,0,0])*4
        raw[n]=bytes([0])+struct.pack('<H',len(packets)+4)+bytes([216])+packets
    if mode=="corrupt": raw={n:b"broken" for n in range(139,143)}
    for n,data in raw.items(): chunks[n]=struct.pack('<I',len(data))+encode(data)
    offsets=[0xffffff]*150; graph=bytearray()
    for n,data in sorted(chunks.items()): offsets[n]=len(graph);graph.extend(data)
    offsets[149]=len(graph)
    (directory/'VGAHEAD.WL6').write_bytes(b''.join(n.to_bytes(3,'little') for n in offsets))
    (directory/'VGAGRAPH.WL6').write_bytes(graph)

class ClassicTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        sdk = Path(os.environ.get('SDL_SDK', ROOT.parent / 'cart/vendor/SDL'))
        if not (sdk / 'build/libSDL3.a').is_file(): raise unittest.SkipTest('SDL SDK unavailable')
        cls.build = tempfile.TemporaryDirectory(prefix='w3r-classic-build-')
        cls.addClassCleanup(cls.build.cleanup)
        cls.binary = Path(cls.build.name) / 'classic-tests'
        flags = ['-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Wconversion', '-Wshadow', '-Werror', '-g', '-DWOLF3D_DISPLAY_TEST']
        if os.environ.get('SANITIZE') == '1': flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        sources = sorted(p for p in (ROOT / 'src').rglob('*.c') if p.name != 'main.c')
        subprocess.run(shlex.split(os.environ.get('CC', 'gcc')) + flags +
                       ['-I' + str(ROOT / 'src'), '-I' + str(sdk / 'include'), str(ROOT / 'tests/classic_tests.c'),
                        *map(str, sources), str(sdk / 'build/libSDL3.a'), '-pthread', '-lm', '-o', str(cls.binary)], check=True)

    def run_display(self, mode, owned=False):
        with tempfile.TemporaryDirectory(prefix='w3r-classic-data-') as directory:
            root = Path(directory); data = root / 'data'; data.mkdir(); slots = root / 'slots'; slots.mkdir()
            if owned:
                if not os.environ.get('WOLF3D_DATA'): self.skipTest('Owned WL6 data not supplied')
                data = Path(os.environ['WOLF3D_DATA'])
            else:
                classic_files(data, mode)
                if mode=='hidden':
                    from audio_fixtures import audio_files
                    audio_files(data)
            env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software', SDL_AUDIODRIVER='dummy')
            subprocess.run([str(self.binary), str(data), str(slots), mode], env=env, check=True)
            if mode == 'scores':
                subprocess.run([str(self.binary), str(data), str(slots), 'scores-read'], env=env, check=True)
            if mode == 'view': self.assertEqual((slots / 'slot1.w3s').read_bytes()[96:], (slots / 'slot2.w3s').read_bytes()[96:])

    def test_interface(self): self.run_display('interface')
    def test_owned_interface(self): self.run_display('interface', owned=True)

    def test_hidden_features(self): self.run_display('hidden')

    def test_high_scores(self): self.run_display('scores')

    def test_view_sizes(self): self.run_display('view')
    def test_function_keys(self): self.run_display('keys')
    def test_demos(self): self.run_display('demo')
    def test_owned_views(self): self.run_display('view', owned=True)
    def test_owned_shortcuts(self): self.run_display('keys', owned=True)
    def test_owned_demos(self): self.run_display('demo', owned=True)
    def test_missing_demos(self): self.run_display("missing")
    def test_corrupt_demos(self): self.run_display("corrupt")
