#!/usr/bin/env python3
"""Compare private traces or derive asset-free per-frame DOS regression hashes."""
# SPDX-License-Identifier: GPL-2.0-only
import argparse
import hashlib
import json
from pathlib import Path
import sys
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests'))
from demo_trace_tools import compare, digest, dos_frames, native_frames
from demo_reference import ASSETS, EXE_SHA, MAP_SHA


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dos', required=True, type=Path)
    parser.add_argument('--native', type=Path)
    parser.add_argument('--write-golden', type=Path)
    parser.add_argument('--data', type=Path)
    args = parser.parse_args()
    reference = lambda: dos_frames(args.dos, ROOT / 'WOLFSRC/WOLF3D.MAP')
    if args.native:
        difference = compare(native_frames(args.native), reference())
        if difference:
            print(json.dumps(difference))
            return 1
        print('All native/DOS frame fields match.')
    if args.write_golden:
        if args.data is None:
            parser.error('--write-golden requires --data for source identities')
        hashes = [[] for _ in range(4)]
        for frame in reference():
            number, command = frame['F'][:2]
            if command != len(hashes[number])+1:
                raise ValueError('Reference command gap')
            hashes[number].append(digest(frame))
        if list(map(len, hashes)) != [691,1899,1140,1656]:
            raise ValueError('Reference did not complete all four recordings')
        identities = {}
        for name in ASSETS:
            source = next(p for p in args.data.iterdir() if p.name.upper() == name+'.WL6')
            identities[name+'.WL6'] = hashlib.sha256(source.read_bytes()).hexdigest()
        result = {'schema': 1, 'reference_exe': EXE_SHA, 'reference_map': MAP_SHA,
                  'view_size': 15, 'sound': 'off', 'assets': identities, 'frames': hashes}
        args.write_golden.write_text(json.dumps(result, indent=2)+'\n')
        print('Wrote DOS frame hashes; no commands, maps or PCM assets included.')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
