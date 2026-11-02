"""Portable saves: integrity, transactional loads, deterministic continuation."""
import os
from pathlib import Path
import shlex
import struct
import subprocess
import tempfile
import unittest
import zlib
from test_engine import ROOT, fixtures
from game_fixtures import finale_files

class SaveTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix='w3r-save-build-')
        cls.addClassCleanup(cls.temp.cleanup)
        cls.binary = Path(cls.temp.name) / 'save-tests'
        flags = ['-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Wconversion', '-Wshadow', '-Werror', '-g']
        if os.environ.get('SANITIZE') == '1':
            flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        subprocess.run(shlex.split(os.environ.get('CC', 'gcc')) + flags + ['-I' + str(ROOT / 'src'),
                       str(ROOT / 'tests/save_tests.c'), *map(str, sorted((ROOT / 'src/engine').glob('*.c'))),
                       str(ROOT / 'src/platform/data_file.c'), str(ROOT / 'src/platform/save_file.c'),
                       '-lm', '-o', str(cls.binary)], check=True)

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='w3r-save-data-')
        self.addCleanup(self.temp.cleanup)
        self.data = Path(self.temp.name) / 'data'
        self.slots = Path(self.temp.name) / 'saves'
        self.data.mkdir(); self.slots.mkdir()
        fixtures(self.data, ui=True); finale_files(self.data)

    def run_save(self, *args, **kwargs):
        return subprocess.run([str(self.binary), str(self.data), str(self.slots), *args], check=True, **kwargs)

    def payload(self, slot):
        return (self.slots / f'slot{slot}.w3s').read_bytes()[96:]

    def seed(self):
        self.run_save()

    def reject(self, raw, checksum=True):
        raw = bytearray(raw)
        if checksum:
            struct.pack_into('<I', raw, 24, 0)
            struct.pack_into('<I', raw, 24, zlib.crc32(raw))
        (self.slots / 'slot1.w3s').write_bytes(raw)
        self.run_save('reject')
        self.assertEqual(self.payload(5), self.payload(6), 'failed load changed the live game')
        self.assertEqual((self.slots / 'slot1.w3s').read_bytes(), raw)

    def test_intermission_counts_and_sound_order(self):
        from audio_fixtures import audio_files
        audio_files(self.data)
        self.compile_extra('intermission_tests.c', 'intermission-tests')
        subprocess.run([str(Path(self.__class__.temp.name) / 'intermission-tests'), str(self.data)], check=True)

    @classmethod
    def compile_extra(cls, source, output, sdl=False):
        flags = ['-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Wconversion', '-Wshadow', '-Werror', '-g']
        if os.environ.get('SANITIZE') == '1':
            flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        sources = sorted((ROOT / 'src/engine').glob('*.c'))
        extra = [str(ROOT / 'src/platform/data_file.c'), str(ROOT / 'src/platform/save_file.c'), '-lm']
        if sdl:
            sdk = Path(os.environ.get('SDL_SDK', ROOT.parent / 'cart/vendor/SDL'))
            if not (sdk / 'build/libSDL3.a').is_file():
                raise unittest.SkipTest('SDL SDK unavailable')
            sources = [p for p in (ROOT / 'src').rglob('*.c') if p.name != 'main.c']
            flags += ['-I' + str(sdk / 'include')]
            extra = [str(sdk / 'build/libSDL3.a'), '-pthread', '-lm']
        subprocess.run(shlex.split(os.environ.get('CC', 'gcc')) + flags + ['-I' + str(ROOT / 'src'),
                       str(ROOT / 'tests' / source), *map(str, sources), *extra, '-o', str(Path(cls.temp.name) / output)], check=True)

    def test_sdl_save_load_and_restart(self):
        from audio_fixtures import audio_files
        audio_files(self.data)
        self.compile_extra('save_sdl_tests.c', 'save-sdl-tests', sdl=True)
        env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software', SDL_AUDIODRIVER='dummy', WOLF3D_TEST_UNICODE='1')
        program = str(Path(self.__class__.temp.name) / 'save-sdl-tests')
        for mode in ['write', 'read']:
            subprocess.run([program, str(self.data), str(self.slots), mode], env=env, check=True)
        if os.environ.get('WOLF3D_DATA'):
            owned = self.slots / 'owned'; owned.mkdir()
            for mode in ['write', 'read']:
                subprocess.run([program, os.environ['WOLF3D_DATA'], str(owned), mode], env=env, check=True)

    def test_all_slots_and_deterministic_continuation(self):
        self.seed()
        self.assertEqual(self.payload(2), self.payload(3))
        for slot in range(1, 7):
            raw = (self.slots / f'slot{slot}.w3s').read_bytes()
            self.assertEqual(raw[:8], b'W3RSAVE\0')
            self.assertEqual(struct.unpack_from('<I', raw, 20)[0], slot - 1)
            checksum = struct.unpack_from('<I', raw, 24)[0]
            clean = bytearray(raw); struct.pack_into('<I', clean, 24, 0)
            self.assertEqual(checksum, zlib.crc32(clean))

    def test_doors_pushwalls_projectiles_bosses_and_inventory(self):
        self.run_save('scenario')
        self.assertEqual(self.payload(2), self.payload(3))

    def test_save_after_death_and_respawn(self):
        self.run_save('respawn')

    def test_six_episode_continuation(self):
        self.run_save('episodes')
        if os.environ.get('WOLF3D_DATA'):
            subprocess.run([str(self.binary), os.environ['WOLF3D_DATA'], str(self.slots), 'episodes'], check=True)

    def test_audio_restoration(self):
        from audio_fixtures import audio_files
        audio_files(self.data)
        self.run_save('audio')

    def test_process_restart(self):
        self.seed(); expected = self.payload(1)
        self.run_save('read')
        self.assertEqual(self.payload(6), expected)

    def test_corrupt_headers_and_truncation(self):
        self.seed(); raw = (self.slots / 'slot1.w3s').read_bytes()
        for length in [0, 7, 95, 96, len(raw) - 1]:
            with self.subTest(truncated=length): self.reject(raw[:length], checksum=False)
        for offset, value in [(0, 0), (8, 99), (12, 4), (16, 0xFFFFFFFF), (20, 5), (28, 6), (32, 10), (36, 4)]:
            with self.subTest(header_offset=offset):
                bad = bytearray(raw); struct.pack_into('<I', bad, offset, value); self.reject(bad)
        bad = bytearray(raw); bad[40] ^= 1; self.reject(bad, checksum=False)
        bad = bytearray(raw); bad[-1] ^= 1; self.reject(bad, checksum=False)

    def test_invalid_state_and_object_references(self):
        self.seed(); raw = (self.slots / 'slot1.w3s').read_bytes()
        # Version 1: game=28 words, ratios=48 words, then five control counts.
        payload = raw[96:]
        count, stat_count, doors = struct.unpack_from('<III', payload, 312)
        cursor = 412; records = []
        for _ in range(count):
            used = struct.unpack_from('<I', payload, cursor)[0]; cursor += 4
            if used:
                records.append(cursor); cursor += 88
        cursor += stat_count * 20 + doors * 28
        wall = cursor; grid = wall + 20
        changes = [(312, 151), (316, 401), (320, 65), (324, 64 * 65536), (336, 64),
                   (404, 99999), (408, 150), (304, 256), (308, 2),
                   (grid + 8, 151), (grid + 4, 37), (grid + 12, 2),
                   (wall, 256), (grid + 3 * 20, 0)]
        if records:
            changes += [(records[0] + 80, 99999), (records[0] + 12, 64), (records[0] + 72, 2),
                        (records[0] + 60, (-65) & 0xFFFFFFFF)]
        for offset, value in changes:
            with self.subTest(payload_offset=offset):
                bad = bytearray(raw); struct.pack_into('<I', bad, 96 + offset, value); self.reject(bad)
        bad = bytearray(raw)
        struct.pack_into('<III', bad, 96 + wall, 1, 0, 64)  # active wall, invalid x
        self.reject(bad)

    def test_resource_identity_mismatch(self):
        self.seed(); raw = (self.slots / 'slot1.w3s').read_bytes()
        file = self.data / 'GAMEMAPS.WL6'; file.write_bytes(file.read_bytes() + b'new identity')
        self.reject(raw)

    def test_failed_write_preserves_slot(self):
        self.seed(); before = (self.slots / 'slot1.w3s').read_bytes()
        self.slots.chmod(0o555)
        try: self.run_save('writefail')
        finally: self.slots.chmod(0o755)
        self.assertEqual((self.slots / 'slot1.w3s').read_bytes(), before)
        self.assertFalse(list(self.slots.glob('*.tmp-*')))
        if os.name == 'posix':
            import resource
            import signal
            def limit_write():
                signal.signal(signal.SIGXFSZ, signal.SIG_IGN)
                resource.setrlimit(resource.RLIMIT_FSIZE, (128, 128))
            self.run_save('writefail', preexec_fn=limit_write)
            self.assertEqual((self.slots / 'slot1.w3s').read_bytes(), before)
            self.assertFalse(list(self.slots.glob('*.tmp-*')))

    def test_competing_create_does_not_overwrite(self):
        commands = [[str(self.binary), str(self.data), str(self.slots), name] for name in ['create one', 'create two']]
        processes = [subprocess.Popen(command) for command in commands]
        self.assertEqual(sorted(p.wait() for p in processes), [0, 2])
        raw = (self.slots / 'slot1.w3s').read_bytes()
        self.assertIn(raw[40:72].split(b'\0')[0], [b'create one', b'create two'])
        self.assertFalse(list(self.slots.glob('*.tmp-*')))
