"""Original sound manager regressions, including a dummy SDL device."""
import os
import re
from pathlib import Path
import shlex
import struct
import subprocess
import tempfile
import unittest
from test_engine import ROOT, fixtures
from audio_fixtures import audio_files

class AudioTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        sdk = Path(os.environ.get('SDL_SDK', ROOT.parent / 'cart/vendor/SDL'))
        library = sdk / 'build/libSDL3.a'
        if not library.is_file():
            raise unittest.SkipTest('SDL_SDK must point to built static SDL3')
        cls.temp = tempfile.TemporaryDirectory(prefix='w3r-audio-build-')
        cls.addClassCleanup(cls.temp.cleanup)
        cls.binary = Path(cls.temp.name) / 'audio-tests'
        flags = ['-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Wconversion', '-Wshadow', '-Werror', '-g']
        if os.environ.get('SANITIZE') == '1':
            flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        sources = sorted((ROOT / 'src/engine').glob('*.c'))
        subprocess.run(shlex.split(os.environ.get('CC', 'gcc')) + flags +
                       ['-I' + str(ROOT / 'src'), '-I' + str(sdk / 'include'), str(ROOT / 'tests/audio_tests.c'),
                        *map(str, sources), str(ROOT / 'src/platform/data_file.c'), str(ROOT / 'src/platform/save_file.c'), str(ROOT / 'src/platform/sdl_audio.c'),
                        str(library), '-pthread', '-lm', '-o', str(cls.binary)], check=True)

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='w3r-audio-data-')
        self.addCleanup(self.temp.cleanup)
        self.data = Path(self.temp.name)
        fixtures(self.data, ui=True)
        audio_files(self.data)

    def run_audio(self, path=None, *args):
        return subprocess.run([str(self.binary), str(path or self.data), *args], check=True,
                              env=dict(os.environ, SDL_AUDIODRIVER='dummy'))

    def test_original_metadata(self):
        original = (ROOT / 'WOLFSRC/WL_MAIN.C').read_text(encoding='latin1')
        table = original.split('static  int     wolfdigimap[]')[1].split('#else')[0]
        expected = {name: int(index) + 1 for name, index in
                    re.findall(r'([A-Z0-9_]+SND),\s*(\d+)', table)}
        port = (ROOT / 'src/engine/ID_SD.c').read_text()
        actual = {name: int(index) for name, index in
                  re.findall(r'\[([A-Z0-9_]+SND)\]\s*=\s*(\d+)', port)}
        self.assertEqual(actual, expected)
        songs = (ROOT / 'WOLFSRC/WL_PLAY.C').read_text(encoding='latin1')
        music_names = re.findall(r'([A-Z0-9_]+_MUS),?', songs.split('int songs[]=')[1].split('#else')[0])
        header = (ROOT / 'WOLFSRC/AUDIOWL6.H').read_text()
        ids = dict((name, int(index)) for name, index in re.findall(r'([A-Z0-9_]+_MUS),\s*//\s*(\d+)', header))
        expected = [ids[name] for name in music_names]
        values = port.split('static const int songs[60]={')[1].split('}')[0]
        self.assertEqual(list(map(int, re.findall(r'\d+', values))), expected)

    def test_synthetic_audio(self):
        self.run_audio()

    def test_gameplay_audio_independence(self):
        from game_fixtures import finale_files
        finale_files(self.data)
        audio_files(self.data)
        self.run_audio(None, 'gameplay')

    def test_readonly_audio_data(self):
        files = list(self.data.iterdir())
        try:
            for file in files:
                file.chmod(0o444)
            self.data.chmod(0o555)
            self.run_audio()
        finally:
            self.data.chmod(0o755)
            for file in files:
                file.chmod(0o644)

    def test_owned_audio(self):
        if not os.environ.get('WOLF3D_DATA'):
            self.skipTest('Set WOLF3D_DATA to owned WL6 files')
        self.run_audio(os.environ['WOLF3D_DATA'], 'owned')

    def test_corrupt_audio(self):
        for name in ['AUDIOHED.WL6', 'AUDIOT.WL6', 'VSWAP.WL6']:
            file = self.data / name
            original = file.read_bytes()
            for length in [0, 1, 5, len(original) - 1]:
                with self.subTest(name=name, length=length):
                    file.write_bytes(original[:length])
                    self.run_audio(None, 'invalid')
            file.write_bytes(original)
        head = self.data / 'AUDIOHED.WL6'
        original = head.read_bytes()
        for index in [0, 87, 174, 261, 288]:
            with self.subTest(offset=index):
                raw = bytearray(original)
                struct.pack_into('<I', raw, index * 4, 0xFFFFFFFF)
                head.write_bytes(raw)
                self.run_audio(None, 'invalid')
        head.write_bytes(original)
        archive = self.data / 'AUDIOT.WL6'
        original = archive.read_bytes()
        offsets = struct.unpack('<289I', head.read_bytes())
        for offset, value, fmt in [(offsets[87], 0xFFFFFFFF, '<I'),
                                   (offsets[261], 0, '<H'), (offsets[261], 3, '<H')]:
            raw = bytearray(original)
            struct.pack_into(fmt, raw, offset, value)
            archive.write_bytes(raw)
            self.run_audio(None, 'invalid')
        archive.write_bytes(original)
