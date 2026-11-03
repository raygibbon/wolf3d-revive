"""DOS source composition and fixed 4:3 presentation; owned art stays external."""
import os
from pathlib import Path
import shlex
import struct
import subprocess
import tempfile
import unittest
from test_engine import ROOT, encode
from game_fixtures import finale_files

class DOSMenuTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.build=tempfile.TemporaryDirectory(prefix='w3r-dos-menus-')
        cls.addClassCleanup(cls.build.cleanup)
        sdk=Path(os.environ.get('SDL_SDK', ROOT.parent/'cart/vendor/SDL'))
        cls.binary=Path(cls.build.name)/'menus'
        flags=['-std=c11','-Wall','-Wextra','-Wpedantic','-Wconversion','-Wshadow','-Werror','-g']
        if os.environ.get('SANITIZE')=='1': flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        sources=sorted(p for p in (ROOT/'src').rglob('*.c') if p.name!='main.c')
        subprocess.run(shlex.split(os.environ.get('CC','gcc'))+flags+['-I'+str(ROOT/'src'),'-I'+str(sdk/'include'),
            str(ROOT/'tests/dos_menu_tests.c'),*map(str,sources),str(sdk/'build/libSDL3.a'),'-pthread','-lm','-o',str(cls.binary)],check=True)

    def run_menus(self,owned=False,rating=False):
        with tempfile.TemporaryDirectory(prefix='w3r-menu-data-') as directory:
            root=Path(directory);data=root/'data';data.mkdir()
            if owned:
                if not os.environ.get('WOLF3D_DATA'): self.skipTest('Owned WL6 unavailable')
                data=Path(os.environ['WOLF3D_DATA'])
            else:
                finale_files(data)
                if rating:
                    head=(data/'VGAHEAD.WL6').read_bytes();graph=(data/'VGAGRAPH.WL6').read_bytes()
                    offsets=[int.from_bytes(head[i:i+3],'little') for i in range(0,450,3)]
                    chunks={}
                    for i,at in enumerate(offsets[:-1]):
                        if at!=0xffffff: chunks[i]=graph[at:next(x for x in offsets[i+1:] if x!=0xffffff)]
                    dims=bytearray(encode(chunks[0][4:]))  # Bit reversal is its own inverse.
                    struct.pack_into('<HH',dims,(88-3)*4,88,64)
                    chunks[0]=struct.pack('<I',len(dims))+encode(dims)
                    chunks[88]=struct.pack('<I',88*64)+encode(bytes([88])*(88*64))
                    out=bytearray();offsets=[0xffffff]*150
                    for i,chunk in sorted(chunks.items()): offsets[i]=len(out);out.extend(chunk)
                    offsets[149]=len(out)
                    (data/'VGAHEAD.WL6').write_bytes(b''.join(x.to_bytes(3,'little') for x in offsets))
                    (data/'VGAGRAPH.WL6').write_bytes(out)
            env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_RENDER_DRIVER='software',SDL_AUDIODRIVER='dummy')
            subprocess.run([str(self.binary),str(data),str(root)],env=env,check=True)
            self.assertEqual((root/'slot1.w3s').read_bytes()[96:],(root/'slot2.w3s').read_bytes()[96:])
            self.assertEqual((root/'control-on.idx').stat().st_size,320*200)

    def test_missing_optional_intro(self): self.run_menus()
    def test_synthetic_rating(self): self.run_menus(rating=True)
    def test_owned_menus(self): self.run_menus(owned=True)
