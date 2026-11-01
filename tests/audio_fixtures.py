"""Invented audio bytes only. No commercial samples or music fixtures."""
import struct

def audio_files(path):
    sounds = []
    for i in range(87):
        sounds.append(struct.pack('<IH', 140, i + 1) + bytes([40]) * 140)
    instrument = bytes([1, 1, 20, 0, 0xF2, 0xF2, 0x77, 0x77, 0, 0, 0, 0, 0, 0, 0, 0])
    for i in range(87):
        sounds.append(struct.pack('<IH', 140, i + 1) + instrument + bytes([4]) + bytes([128]) * 140)
    sounds += [b''] * 87
    # A one-second DOS loop: 699 delayed ticks then a final key-off. Its
    # final delay is discarded by the original ISR, restarting at tick 700.
    events = [(1, 0x20, 0), (0x21, 1, 0), (0x24, 1, 0), (0x41, 20, 0),
              (0x44, 0, 0), (0x61, 0xF2, 0), (0x64, 0xF2, 0),
              (0x81, 0x77, 0), (0x84, 0x77, 0), (0xC1, 0, 0),
              (0xA1, 128, 0), (0xB1, 0x31, 699), (0xB1, 0, 99)]
    song = b''.join(struct.pack('<BBH', *e) for e in events)
    sounds += [struct.pack('<H', len(song)) + song] * 27
    shared = b''.join(struct.pack('<BBH', *e) for e in
                      [(0xB0, 0, 4), (0xA0, 37, 0), (0xB0, 0x2C, 100)])
    sounds[-1] = struct.pack('<H', len(shared)) + shared
    offsets, blob = [0], bytearray()
    for sound in sounds:
        blob += sound
        offsets.append(len(blob))
    (path / 'AUDIOHED.WL6').write_bytes(struct.pack('<289I', *offsets))
    (path / 'AUDIOT.WL6').write_bytes(blob)
    swap = path / 'VSWAP.WL6'
    if swap.exists() and len(swap.read_bytes()) >= 6 and 6 + 6 * struct.unpack_from("<H", swap.read_bytes())[0] <= len(swap.read_bytes()):
        raw = swap.read_bytes()
        count, sprites, start = struct.unpack_from('<3H', raw)
        offsets = struct.unpack_from(f'<{count}I', raw, 6)
        lengths = struct.unpack_from(f'<{count}H', raw, 6 + 4 * count)
        pages = [raw[o:o+n] if o else b'' for o, n in zip(offsets[:start], lengths[:start])]
    else:
        sprites = start = 8
        pages = [bytes(4096)] * start
    for i in range(46):
        pages.append(bytes([128 + (i % 4 + 1) * 16]) * 142)
    pages.append(b''.join(struct.pack('<HH', i, 142) for i in range(46)))
    count = len(pages)
    pos = 6 + 6 * count
    offsets, lengths = [], []
    for page in pages:
        offsets.append(pos if page else 0)
        lengths.append(len(page))
        pos += len(page)
    swap.write_bytes(struct.pack('<3H', count, sprites, start) +
                     struct.pack(f'<{count}I', *offsets) +
                     struct.pack(f'<{count}H', *lengths) + b''.join(pages))
