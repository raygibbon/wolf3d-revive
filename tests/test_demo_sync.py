"""DOS-derived, per-frame demo synchronization regressions (no game assets)."""
# SPDX-License-Identifier: GPL-2.0-only
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import struct
import tempfile
import unittest
from demo_trace_tools import DOS_RECORD, canonical, compare, digest, dos_frames, native_frames
from test_engine import ROOT
from test_classic import classic_files


class TraceReaderTests(unittest.TestCase):
    def test_truncated_dos_trace(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'trace.bin'
            path.write_bytes(b'truncated')
            with self.assertRaisesRegex(ValueError, 'Truncated'):
                list(dos_frames(path, ROOT/'WOLFSRC/WOLF3D.MAP'))
            raw = bytearray(DOS_RECORD)
            path.write_bytes(raw)
            with self.assertRaisesRegex(ValueError, 'clock'):
                list(dos_frames(path, ROOT/'WOLFSRC/WOLF3D.MAP'))
            struct.pack_into('<i',raw,52,4)
            struct.pack_into('<H',raw,66,65535)
            path.write_bytes(raw)
            with self.assertRaisesRegex(ValueError, 'actor list'):
                list(dos_frames(path, ROOT/'WOLFSRC/WOLF3D.MAP'))
            struct.pack_into('<H',raw,66,0xb51c)
            struct.pack_into('<H',raw,92+6,0x190c)  # original s_player
            struct.pack_into('<H',raw,92+56,0xb51c)
            path.write_bytes(raw)
            with self.assertRaisesRegex(ValueError, 'actor list'):
                list(dos_frames(path, ROOT/'WOLFSRC/WOLF3D.MAP'))

    def test_unused_door_timer_is_not_simulation_state(self):
        frame = {'F':[0,1], 'A':[], 'V':'', 'M':'', 'Q':[], 'D':[[0,2,6,0,0,0,123,0]]}
        other = dict(frame, D=[[0,2,6,0,0,0,999,0]])
        self.assertEqual(digest(frame), digest(other))
        self.assertIsNone(compare([frame], [other]))
        other['D'][0][5] = 2  # dr_open: the clock now drives simulation.
        self.assertNotEqual(digest(frame), digest(other))
        self.assertEqual(compare([frame], [other])['component'], 'D')
        self.assertEqual(compare([], [frame])['component'], 'duration')
        self.assertEqual(canonical(frame)['D'][0][6], 0)


class DemoSyncTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        sdk = Path(os.environ.get('SDL_SDK', ROOT.parent/'cart/vendor/SDL'))
        if not (sdk/'build/libSDL3.a').is_file():
            raise unittest.SkipTest('SDL SDK unavailable')
        cls.build = tempfile.TemporaryDirectory(prefix='w3r-demo-trace-')
        cls.addClassCleanup(cls.build.cleanup)
        cls.binary = Path(cls.build.name)/'demo-trace'
        flags = ['-std=c11','-Wall','-Wextra','-Wpedantic','-Wconversion','-Wshadow','-Werror','-g']
        if os.environ.get('SANITIZE') == '1':
            flags += ['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        sources = sorted(p for p in (ROOT/'src').rglob('*.c') if p.name != 'main.c')
        subprocess.run(shlex.split(os.environ.get('CC','gcc'))+flags+
                       ['-I'+str(ROOT/'src'),'-I'+str(sdk/'include'),str(ROOT/'tests/demo_trace.c'),
                        *map(str,sources),str(sdk/'build/libSDL3.a'),'-pthread','-lm','-o',str(cls.binary)],check=True)

    def trace(self, data, destination):
        with destination.open('w') as stream:
            subprocess.run([str(self.binary),str(data)],stdout=stream,check=True)

    def test_synthetic_trace_repeatability(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory);data = root/'data';data.mkdir();classic_files(data)
            a,b = root/'a.csv',root/'b.csv'
            self.trace(data,a);self.trace(data,b)
            self.assertEqual(a.read_bytes(), b.read_bytes())
            frames = list(native_frames(a))
            self.assertEqual(len(frames),64)
            for frame in frames:
                self.assertEqual(frame['F'][2],frame['F'][1]*4)
                self.assertEqual(len(frame['V']),4096)
                self.assertEqual(len(frame['M']),8192)
                self.assertEqual(len(frame['Q']),4096)
                self.assertTrue(frame['A'])

    def test_owned_dos_frames_and_durations(self):
        if not os.environ.get('WOLF3D_DATA'):
            self.skipTest('Owned WL6 data not supplied')
        data = Path(os.environ['WOLF3D_DATA'])
        expected = json.loads((ROOT/'tests/fixtures/dos_demo_hashes.json').read_text())
        paths = {p.name.upper():p for p in data.iterdir()}
        for name, identity in expected['assets'].items():
            if name not in paths or hashlib.sha256(paths[name].read_bytes()).hexdigest() != identity:
                self.skipTest('DOS reference hashes cover a different WL6 data revision: '+name)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'trace.csv';self.trace(data,path)
            counts = [0]*4
            for frame in native_frames(path):
                number,command = frame['F'][:2]
                with self.subTest(demo=number,command=command):
                    self.assertEqual(command,counts[number]+1)
                    self.assertEqual(digest(frame),expected['frames'][number][command-1])
                counts[number] += 1
            self.assertEqual(counts,[691,1899,1140,1656])
            self.assertEqual([n*4 for n in counts],[2764,7596,4560,6624])
