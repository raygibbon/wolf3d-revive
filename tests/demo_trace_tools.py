"""Stable, streaming native/DOS trace readers. No executable addresses in hashes."""
# SPDX-License-Identifier: GPL-2.0-only
import csv
import hashlib
import json
from pathlib import Path
import re
import struct

DOS_RECORD = 26251

def native_frames(path, include_initial=False):
    frame = None
    with Path(path).open(newline='') as stream:
        for row in csv.reader(stream):
            kind = row[0]
            if kind == 'F':
                if frame is not None and (include_initial or frame['F'][1]):
                    yield frame
                frame = {'F': list(map(int, row[1:])), 'A': [], 'D': []}
            elif frame is None:
                raise ValueError('Trace row precedes frame header')
            elif kind == 'A':
                # Player is stepped directly in the port, rather than through
                # DOS s_player/s_attack pointers. Its state is in the F fields.
                if row[1] != '0':
                    frame['A'].append([int(row[1]), row[2], *map(int, row[3:])])
            elif kind in ('V', 'M'):
                frame[kind] = row[1]
            elif kind == 'Q':
                frame[kind] = list(map(int, row[1:]))
            elif kind == 'D':
                frame[kind].append(list(map(int, row[1:])))
            else:
                raise ValueError('Unknown trace row '+kind)
    if frame is not None and (include_initial or frame['F'][1]):
        yield frame


def dos_frames(path, symbol_map):
    text = Path(symbol_map).read_text()
    states = {int(address, 16): name for address, name in re.findall(
        r'3A39:([0-9A-F]+)\s+(?:idle\s+)?_(s_\w+)', text)}
    def short(data, offset):
        return struct.unpack_from('<h', data, offset)[0]
    def word(data, offset):
        return struct.unpack_from('<H', data, offset)[0]
    def long(data, offset):
        return struct.unpack_from('<i', data, offset)[0]
    previous = None
    demo = -1
    with Path(path).open('rb') as stream:
        while data := stream.read(DOS_RECORD):
            if len(data) != DOS_RECORD:
                raise ValueError('Truncated DOS trace record')
            time = long(data, 52)
            if previous is None or time <= previous:
                demo += 1
            previous = time
            if not 0 <= demo < 4 or time <= 0 or time % 4:
                raise ValueError('Invalid DOS demo clock')
            actors = data[92:9092]
            pointer = word(data, 66)
            seen = set()
            rows = []
            while pointer:
                if pointer in seen or pointer < 0xb51c or (pointer-0xb51c)%60 or pointer >= 0xb51c+9000:
                    raise ValueError('Invalid DOS actor list')
                seen.add(pointer)
                actor = actors[pointer-0xb51c:pointer-0xb51c+60]
                state = word(actor, 6)
                if state not in states:
                    raise ValueError('Unknown DOS actor state')
                rows.append([len(rows), states[state], long(actor,16), long(actor,20), short(actor,42),
                             word(actor,24), word(actor,26), short(actor,4), short(actor,2), short(actor,44),
                             short(actor,14), long(actor,46), long(actor,10), short(actor,50), short(actor,52),
                             actor[8], int(short(actor,0) != 0), short(actor,30), word(actor,32)])
                pointer = word(actor,56)
            if not rows:
                raise ValueError('Missing DOS player')
            command = struct.unpack_from('<Bbb', data, 9094)
            f = [demo, time//4, time, *rows[0][2:5], short(data,18), short(data,20), word(data,9092),
                 *command, short(data,26), short(data,32), short(data,34)]
            count = word(data,26249)
            if count > 64:
                raise ValueError('Invalid DOS door count')
            doors = []
            for i in range(count):
                door = data[17417+i*10:17427+i*10]
                doors.append([i, door[0], door[1], word(door,2), door[4],
                              {0:2,1:0,2:1,3:3}[short(door,6)], word(door,8), word(data,17289+i*2)])
            grid = []
            for i in range(4096):
                value = word(data,18057+i*2)
                if value < 256:
                    grid.append(0)  # Solid integer tags are represented by M/D.
                elif 0xb51c <= value < 0xb51c+9000 and (value-0xb51c)%60 == 0:
                    grid.append((value-0xb51c)//60)
                else:
                    raise ValueError('Invalid DOS reservation')
            yield {'F': f, 'A': rows[1:], 'D': doors, 'Q': grid,
                   'V': ''.join('1' if b else '0' for b in data[9097:13193]),
                   'M': data[13193:17289].hex()}


def canonical(frame):
    result = dict(frame)
    # SpawnDoor does not initialise ticcount in DOS. The value has no meaning
    # outside dr_open; DoorOpening resets it before the timer can be used.
    result['D'] = [row[:6]+[row[6] if row[5] == 2 else 0, row[7]] for row in frame['D']]
    return result


def digest(frame):
    value = json.dumps(canonical(frame), sort_keys=True, separators=(',', ':')).encode()
    return hashlib.sha256(value).hexdigest()


def compare(native, reference):
    """Return first differing frame/component, or None for complete equality."""
    from itertools import zip_longest
    for left, right in zip_longest(native, reference):
        if left is None or right is None:
            return {'frame': (left or right)['F'][:2], 'component': 'duration'}
        a, b = canonical(left), canonical(right)
        for key in ('F', 'A', 'V', 'M', 'Q', 'D'):
            if a[key] != b[key]:
                return {'frame': b['F'][:2], 'component': key, 'native': a[key], 'dos': b[key]}
    return None
