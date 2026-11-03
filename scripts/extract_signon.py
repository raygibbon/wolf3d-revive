#!/usr/bin/env python3
"""Extract authentic sign-on pixels from a verified DOS executable or historical source object.

No art is embedded here. The bounded LZEXE load-image decoder follows the
original LZEXE format (https://bellard.org/lzexe/) and was checked independently
against UNLZEXE (https://github.com/mywave82/unlzexe). This tool does not execute
DOS code, reconstruct an EXE or modify the source installation.
"""
# SPDX-License-Identifier: GPL-2.0-only
import argparse
import hashlib
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]
# Steam's Activision WL6 DOS executable. Addresses refer to the decompressed
# MZ load image, excluding the EXE header. Refuse unknown revisions, not guesses.
EXE_SHA256 = '48d9594649f330956e86ea1892bab76b2d83cfd4038e262ff8f3383ba945ec09'
LOAD_SHA256 = 'd397db84048d18bbb90df0ba0db88bdf5982ce3cac868bec13f117e61fb74ff0'
SIGNON_SHA256 = 'd3c2f25b82b24f3513be29ee12bd5dc71ab1b0ac737d5111cbd36150927fa6f7'
LOAD_SIZE = 254014
SIGNON_OFFSET = 0x23760
PIXELS = 320 * 200
OBJECT_SHA256 = 'dd66c2a2c7744988ad7700c826674d6747859bde9baf817cba7399f99937b376'
SOURCE_SIGNON_SHA256 = '4fd711cdde7929645a46dd572ab61feacc8d3b79844250835b0ce87312a2c999'


def decode_object(data):
    """Reassemble the source release's OMF LEDATA records, not object headers."""
    pixels, covered = bytearray(PIXELS), bytearray(PIXELS)
    position = 0
    while position < len(data):
        if position + 3 > len(data):
            raise ValueError('Truncated OMF record header')
        kind, length = data[position], int.from_bytes(data[position+1:position+3], 'little')
        end = position + 3 + length
        if length < 1 or end > len(data):
            raise ValueError('Truncated OMF record')
        record = data[position:end]
        # Zero denotes an omitted OMF checksum (used by this file's headers).
        if record[-1] and sum(record) % 256:
            raise ValueError('Invalid OMF checksum')
        payload = record[3:-1]
        if kind == 0xa0:
            if len(payload) < 3 or payload[0] != 1:
                raise ValueError('Unsupported sign-on OMF segment')
            offset = int.from_bytes(payload[1:3], 'little')
            content = payload[3:]
            if offset + len(content) > PIXELS or any(covered[offset:offset+len(content)]):
                raise ValueError('Overlapping or oversized OMF pixels')
            pixels[offset:offset+len(content)] = content
            covered[offset:offset+len(content)] = bytes([1])*len(content)
        elif kind not in (0x80, 0x88, 0x96, 0x98, 0x90, 0x8a):
            raise ValueError('Unsupported OMF record in sign-on object')
        position = end
    if not all(covered):
        raise ValueError('Incomplete OMF sign-on pixels')
    return bytes(pixels)


def read_source(source):
    with source.open('rb') as stream:
        data = stream.read(1024 * 1024 + 1)
    if len(data) > 1024 * 1024:
        raise ValueError('DOS executable/object exceeds supported size')
    return data


def select_source(source):
    """Accept an asset file, Steam install/base, repository or WOLFSRC directory."""
    if not source.is_dir():
        return source
    for relative in ('wolf3d.exe', 'base/wolf3d.exe', 'OBJ/SIGNON.OBJ',
                     'WOLFSRC/OBJ/SIGNON.OBJ', 'SIGNON.OBJ'):
        candidate = source
        for name in relative.split('/'):
            matches = [p for p in candidate.iterdir() if p.name.lower() == name.lower()] if candidate.is_dir() else []
            if len(matches) != 1:
                candidate = None
                break
            candidate = matches[0]
        if candidate and candidate.is_file():
            digest = hashlib.sha256(read_source(candidate)).hexdigest()
            if digest in (EXE_SHA256, OBJECT_SHA256):
                return candidate
    raise ValueError('No supported wolf3d.exe or source SIGNON.OBJ found in directory')


def decode_stream(data, offset, limit):
    """Decode the LZEXE literal/backreference stream with strict input bounds."""
    position = offset

    def byte():
        nonlocal position
        if not 0 <= position < len(data):
            raise ValueError('Truncated LZEXE stream')
        value = data[position]
        position += 1
        return value

    def word():
        return byte() | (byte() << 8)

    control, remaining = word(), 16

    def bit():
        nonlocal control, remaining
        result = control & 1
        remaining -= 1
        # LZEXE prefetches the next control word before consuming token bytes.
        if remaining == 0:
            control, remaining = word(), 16
        else:
            control >>= 1
        return result

    output = bytearray()
    while True:
        if bit():
            if len(output) >= limit:
                raise ValueError('LZEXE output exceeds expected size')
            output.append(byte())
            continue
        if bit():
            low, high = byte(), byte()
            distance = (low | ((high & 0xf8) << 5) | 0xe000) - 0x10000
            length = (high & 7) + 2
            if length == 2:
                length = byte()
                if length == 0:
                    return bytes(output)
                if length == 1:  # Segment adjustment; no output bytes.
                    continue
                length += 1
        else:
            length = ((bit() << 1) | bit()) + 2
            distance = byte() - 256
        if len(output) + length > limit:
            raise ValueError('LZEXE output exceeds expected size')
        if len(output) + distance < 0:
            raise ValueError('Invalid LZEXE backreference')
        for _ in range(length):  # Backreferences can overlap their destination.
            output.append(output[len(output) + distance])


def extract(data):
    digest = hashlib.sha256(data).hexdigest()
    if digest == OBJECT_SHA256:
        pixels = decode_object(data)
        if hashlib.sha256(pixels).hexdigest() != SOURCE_SIGNON_SHA256:
            raise ValueError('Source sign-on pixel checksum mismatch')
        return pixels
    if digest != EXE_SHA256:
        raise ValueError('Unsupported DOS EXE/object revision (SHA-256 mismatch); see docs/MILESTONE11_SIGNON.md')
    if len(data) < 32 or data[:2] != b'MZ' or data[28:32] != b'LZ91':
        raise ValueError('Expected verified LZEXE 0.91 executable')
    header = struct.unpack_from('<16H', data)
    stub = (header[4] + header[11]) * 16
    if stub + 16 > len(data):
        raise ValueError('Invalid LZEXE information offset')
    info = struct.unpack_from('<8H', data, stub)
    start = (header[11] - info[4] + header[4]) * 16
    image = decode_stream(data, start, LOAD_SIZE)
    if len(image) != LOAD_SIZE or hashlib.sha256(image).hexdigest() != LOAD_SHA256:
        raise ValueError('Decompressed load-image checksum mismatch')
    pixels = image[SIGNON_OFFSET:SIGNON_OFFSET + PIXELS]
    if len(pixels) != PIXELS or hashlib.sha256(pixels).hexdigest() != SIGNON_SHA256:
        raise ValueError('Sign-on pixel checksum mismatch')
    return pixels


def export(source, destination):
    source, destination = select_source(Path(source).resolve()), Path(destination).resolve()
    if destination == source:
        raise ValueError('Output must be separate from the original executable')
    if destination.is_relative_to(ROOT) and not any(destination.is_relative_to(ROOT / name)
                                                   for name in ('build', 'data', 'game-data')):
        raise ValueError('Write commercial artwork outside the repository or under ignored build/data/game-data')
    pixels = extract(read_source(source))
    with destination.open('xb') as stream:  # Never replace an existing file.
        stream.write(pixels)
    return hashlib.sha256(pixels).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path, help='Verified wolf3d.exe, SIGNON.OBJ, Steam directory or Wolf source directory')
    parser.add_argument('output', type=Path, help='New private SIGNON.RAW, exactly 64000 row-major bytes')
    args = parser.parse_args()
    try:
        checksum = export(args.source, args.output)
    except (OSError, ValueError) as error:
        parser.exit(1, f'Sign-on extraction failed: {error}\n')
    print(f'Extracted 320x200 indexed DOS sign-on: {args.output}\nSHA-256: {checksum}')


if __name__ == '__main__':
    main()
