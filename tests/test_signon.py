"""Private original-art extraction and DOS sign-on presentation regressions."""
# SPDX-License-Identifier: GPL-2.0-only
import hashlib
import importlib.util
import os
from pathlib import Path
import shlex
import struct
import subprocess
import tempfile
import unittest
from test_engine import ROOT, encode, original_palette
from game_fixtures import finale_files
from audio_fixtures import audio_files

spec = importlib.util.spec_from_file_location('extract_signon', ROOT/'scripts/extract_signon.py')
extractor = importlib.util.module_from_spec(spec)
spec.loader.exec_module(extractor)


class Packed:
    """Independent invented LZEXE tokens; no executable or artwork fixture."""
    def __init__(self):
        self.data = bytearray(2)
        self.control = self.used = 0
    def bit(self, value):
        self.data[self.control + self.used//8] |= value << (self.used % 8)
        self.used += 1
        if self.used == 16:
            self.control = len(self.data)
            self.data.extend(bytes(2))
            self.used = 0
    def literal(self, value):
        self.bit(1); self.data.append(value)
    def short(self, distance, length):
        self.bit(0); self.bit(0); self.bit((length-2)//2); self.bit((length-2)%2)
        self.data.append(distance & 255)
    def long(self, distance, length):
        self.bit(0); self.bit(1)
        self.data.extend((distance & 255, ((distance & 0x1f00) >> 5) | (length-2 if 3 <= length <= 9 else 0)))
        if not 3 <= length <= 9: self.data.append(length-1)
    def end(self):
        self.long(-8192, 1)  # Extended length 0 is the end marker.
        return bytes(self.data)


class ExtractionTests(unittest.TestCase):
    def test_literals_and_control_prefetch(self):
        packed = Packed(); expected = bytes(range(256))*2
        for value in expected: packed.literal(value)
        self.assertEqual(extractor.decode_stream(packed.end(), 0, len(expected)), expected)
    def test_overlapping_short_and_long_references(self):
        packed = Packed(); packed.literal(65); packed.short(-1, 5); packed.long(-1, 100)
        packed.long(-8192, 2)  # Extended length 1 is a segment marker.
        packed.literal(66); packed.long(-2, 9)
        self.assertEqual(extractor.decode_stream(packed.end(), 0, 116), b'A'*106 + b'BA'*5)
    def test_stream_bounds_and_invalid_references(self):
        packed = Packed(); packed.literal(42); data = packed.end()
        for length in range(len(data)):
            with self.subTest(length=length), self.assertRaises(ValueError): extractor.decode_stream(data[:length], 0, 1)
        packed = Packed(); packed.short(-1, 2)
        with self.assertRaisesRegex(ValueError, 'backreference'): extractor.decode_stream(packed.end(), 0, 100)
        packed = Packed(); packed.literal(1); packed.literal(2)
        with self.assertRaisesRegex(ValueError, 'exceeds'): extractor.decode_stream(packed.end(), 0, 1)
        with self.assertRaisesRegex(ValueError, 'Truncated'): extractor.decode_stream(b'', -1, 100)
    def test_unknown_executable_is_rejected(self):
        for data in (b'', b'MZ'+bytes(100), bytes(108779)):
            with self.assertRaisesRegex(ValueError, 'Unsupported'): extractor.extract(data)
    def test_output_locations_and_input_bounds(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory); source = root/'input.exe'; source.write_bytes(bytes(1024*1024+1))
            with self.assertRaisesRegex(ValueError, 'separate'): extractor.export(source, source)
            with self.assertRaisesRegex(ValueError, 'outside'): extractor.export(source, ROOT/'SIGNON.RAW')
            with self.assertRaisesRegex(ValueError, 'exceeds'): extractor.export(source, root/'SIGNON.RAW')
            self.assertFalse((root/'SIGNON.RAW').exists())
    def test_source_object_and_directory_extraction(self):
        source = ROOT/'WOLFSRC/OBJ/SIGNON.OBJ'
        pixels = extractor.extract(source.read_bytes())
        self.assertEqual(len(pixels), 64000)
        self.assertEqual(hashlib.sha256(pixels).hexdigest(), extractor.SOURCE_SIGNON_SHA256)
        with tempfile.TemporaryDirectory() as directory:
            for number,path in enumerate((source, ROOT/'WOLFSRC', ROOT)):
                output = Path(directory)/f'{number}.raw'
                self.assertEqual(extractor.export(path, output), extractor.SOURCE_SIGNON_SHA256)
                self.assertEqual(output.read_bytes(), pixels)
    def test_object_bounds_checksums_and_coverage(self):
        def record(kind, payload):
            data = bytes([kind]) + struct.pack('<H', len(payload)+1) + payload
            return data + bytes([-sum(data) % 256])
        one = record(0xa0, b'\x01\x00\x00A')
        for data,message in ((one[:-1], 'Truncated'), (one, 'Incomplete'),
                             (one+one, 'Overlapping'), (record(0xa0, b'\x01\xff\xf9AB'), 'oversized'),
                             (record(0x9c, b'fixup'), 'Unsupported')):
            with self.subTest(message=message), self.assertRaisesRegex(ValueError, message):
                extractor.decode_object(data)
        broken = bytearray(one); broken[-1] = (broken[-1]+1) % 256 or 1
        with self.assertRaisesRegex(ValueError, 'checksum'): extractor.decode_object(broken)
    def test_directory_without_supported_art_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(ValueError, 'No supported'): extractor.select_source(Path(directory))
    def test_owned_extraction_and_overwrite_protection(self):
        data = os.environ.get('WOLF3D_DATA')
        if not data: self.skipTest('Owned DOS installation unavailable')
        source = next((p for p in Path(data).iterdir() if p.name.lower() == 'wolf3d.exe'), None)
        if source is None: self.skipTest('Owned DOS executable unavailable')
        before = source.read_bytes()
        if hashlib.sha256(before).hexdigest() != extractor.EXE_SHA256: self.skipTest('Different owned DOS executable revision')
        self.assertEqual(extractor.select_source(source.parent), source)
        self.assertEqual(extractor.select_source(source.parent.parent), source)
        self.assertIn(original_palette(), extractor.decode_stream(before, 32, extractor.LOAD_SIZE))
        with tempfile.TemporaryDirectory() as directory:
            destination = Path(directory)/'SIGNON.RAW'
            self.assertEqual(extractor.export(source, destination), extractor.SIGNON_SHA256)
            self.assertEqual(len(destination.read_bytes()), 64000)
            with self.assertRaises(FileExistsError): extractor.export(source, destination)
            self.assertEqual(source.read_bytes(), before)


def rating_art(data):
    """Add an invented optional PG13 image to the synthetic archive."""
    offsets = [int.from_bytes((data/'VGAHEAD.WL6').read_bytes()[i:i+3], 'little') for i in range(0, 450, 3)]
    graph = (data/'VGAGRAPH.WL6').read_bytes()
    chunks = {i:graph[at:next(x for x in offsets[i+1:] if x != 0xffffff)]
              for i,at in enumerate(offsets[:-1]) if at != 0xffffff}
    dims = bytearray(encode(chunks[0][4:])); struct.pack_into('<HH', dims, (88-3)*4, 88, 64)
    chunks[0] = struct.pack('<I', len(dims)) + encode(dims)
    chunks[88] = struct.pack('<I', 88*64) + encode(bytes([88])*(88*64))
    graph = bytearray(); offsets = [0xffffff]*150
    for i,chunk in sorted(chunks.items()): offsets[i] = len(graph); graph.extend(chunk)
    offsets[149] = len(graph)
    (data/'VGAHEAD.WL6').write_bytes(b''.join(x.to_bytes(3, 'little') for x in offsets))
    (data/'VGAGRAPH.WL6').write_bytes(graph)


class SignonTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix='w3r-signon-build-'); cls.addClassCleanup(cls.temp.cleanup)
        sdk = Path(os.environ.get('SDL_SDK', ROOT.parent/'cart/vendor/SDL'))
        if not (sdk/'build/libSDL3.a').is_file(): raise unittest.SkipTest('SDL SDK unavailable')
        cls.binary = Path(cls.temp.name)/'signon-tests'
        cls.app = Path(os.environ.get('WOLF3D_BINARY', ROOT/'build/cart-sdl/linux/wolf3d-revive'))
        flags = ['-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Wconversion', '-Wshadow', '-Werror', '-g']
        if os.environ.get('SANITIZE') == '1': flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        sources = sorted(p for p in (ROOT/'src').rglob('*.c') if p.name != 'main.c')
        subprocess.run(shlex.split(os.environ.get('CC', 'gcc')) + flags + ['-I'+str(ROOT/'src'), '-I'+str(sdk/'include'),
            str(ROOT/'tests/signon_tests.c'), *map(str, sources), str(sdk/'build/libSDL3.a'), '-pthread', '-lm', '-o', str(cls.binary)], check=True)
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='w3r-signon-data-'); self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name); self.data = self.root/'data'; self.data.mkdir()
        finale_files(self.data); audio_files(self.data)
        self.raw = self.data/'SIGNON.RAW'
        self.pixels = bytes((x*13+y*7) % 256 for y in range(200) for x in range(320))
        self.env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software', SDL_AUDIODRIVER='dummy', XDG_DATA_HOME=str(self.root/'xdg'))
    def run_signon(self, mode='valid', data=None):
        subprocess.run([str(self.binary), str(data or self.data), str(self.root), str(self.raw), mode], env=self.env, check=True)
    def test_indexed_pixels_palette_geometry_and_dismissal(self):
        self.raw.write_bytes(self.pixels); self.run_signon()
    def test_rating_sequence_and_held_key_isolation(self):
        rating_art(self.data); self.raw.write_bytes(self.pixels); self.run_signon()
    def test_missing_optional_artwork(self): self.run_signon('missing')
    def test_truncated_and_oversized_artwork(self):
        for size in (0, 1, 63999, 64001, 128000):
            with self.subTest(size=size):
                self.raw.write_bytes(bytes(size)); self.run_signon('reject')
    def test_owned_signon_pixels(self):
        data = os.environ.get('WOLF3D_DATA')
        if not data: self.skipTest('Owned WL6 installation unavailable')
        executable = next((p for p in Path(data).iterdir() if p.name.lower() == 'wolf3d.exe'), None)
        if not executable: self.skipTest('Owned DOS executable unavailable')
        self.raw.write_bytes(extractor.extract(executable.read_bytes())); self.run_signon(data=Path(data))
    def app_run(self, *args):
        return subprocess.run([str(self.app), '--data', str(self.data), '--no-audio', '--frames', '1', *args], env=self.env, capture_output=True, text=True)
    def test_cli_bypasses_and_diagnostics(self):
        self.raw.write_bytes(b'invalid')
        bad = self.app_run(); self.assertNotEqual(bad.returncode, 0); self.assertIn('64000 indexed pixels', bad.stderr)
        for flag in ('--menu', '--level'):
            result = self.app_run(flag); self.assertEqual(result.returncode, 0, result.stderr)
        self.raw.unlink(); result = self.app_run(); self.assertEqual(result.returncode, 0, result.stderr)
    def test_cli_capture_palette_and_pixel_order(self):
        self.raw.write_bytes(self.pixels); capture = self.root/'signon.bmp'
        result = self.app_run('--capture', str(capture)); self.assertEqual(result.returncode, 0, result.stderr)
        # Read SDL BMP without imaging-library dependencies. Validate the top
        # 189 DOS rows, before the original FinishSignon prompt overlay.
        bmp = capture.read_bytes(); offset = struct.unpack_from('<I', bmp, 10)[0]
        width,height = struct.unpack_from('<ii', bmp, 18); bits = struct.unpack_from('<H', bmp, 28)[0]
        self.assertIn(bits, (24,32)); self.assertEqual((width,abs(height)), (960,720))
        pixel_bytes = bits//8; stride = (width*pixel_bytes+3)//4*4; palette = bytes((v << 2) | (v >> 4) for v in original_palette())
        for y in range(189):
            for x in range(320):
                px,py = x*3+1, int((y+0.5)*3.6)
                row = py if height < 0 else height-1-py
                at = offset+row*stride+px*pixel_bytes; index = self.pixels[y*320+x]; r,g,b = palette[index*3:index*3+3]
                self.assertEqual(bmp[at:at+3], bytes((b,g,r)), (x,y))
