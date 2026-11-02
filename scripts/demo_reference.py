#!/usr/bin/env python3
"""Build a private instrumented original DOS EXE; never edit WOLFSRC or assets.

GNU binutils assemble the logger. The historical Borland executable and its MAP
are the reference, not a native recompilation or the port's simulation. Only
attract-menu selection, acknowledgement and diagnostic I/O are instrumented.
"""
# SPDX-License-Identifier: GPL-2.0-only
import argparse
import hashlib
from pathlib import Path
import shutil
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[1]
EXE_SHA = '8614a3e4623b66181bf0aa88fc5a4ca36cb97fe5ef8c3b903954dfaaf683ab9b'
MAP_SHA = 'b3058257601f156fee914dcc926b5857753326ac0f1eada65bb0856a7b3f1928'
ASSETS = ('AUDIOHED', 'AUDIOT', 'GAMEMAPS', 'MAPHEAD', 'VGADICT', 'VGAGRAPH', 'VGAHEAD', 'VSWAP')

def build(directory, data, fast=False):
    directory = Path(directory).resolve()
    if directory.is_relative_to(ROOT) and not directory.is_relative_to(ROOT / 'build'):
        raise ValueError('Use an ignored build directory or an external private directory')
    if directory == Path(data).resolve():
        raise ValueError('Output must be separate from original data')
    # Diagnostics and original data must remain private, outside tracked source.
    if directory.exists() and any(directory.iterdir()):
        raise ValueError('Reference output directory must be new or empty')
    directory.mkdir(parents=True, exist_ok=True)
    binary = bytearray((ROOT / 'WOLFSRC/WOLF3D.EXE').read_bytes())
    symbols = (ROOT / 'WOLFSRC/WOLF3D.MAP').read_bytes()
    if hashlib.sha256(binary).hexdigest() != EXE_SHA or hashlib.sha256(symbols).hexdigest() != MAP_SHA:
        raise ValueError('Unrecognised original executable/map: refusing address-based instrumentation')
    subprocess.run(['as', '--32', str(ROOT / 'scripts/demo_reference.s'), '-o', str(directory / 'logger.o')], check=True)
    subprocess.run(['ld', '-m', 'elf_i386', '-Ttext=0x15aa', '--oformat', 'binary', str(directory / 'logger.o'), '-o', str(directory / 'logger.bin')], check=True)
    labels = {line.split()[2]: int(line.split()[0],16) for line in
              subprocess.check_output(['nm','-n',str(directory/'logger.o')],text=True).splitlines()
              if len(line.split()) == 3}
    header = struct.unpack_from('<H', binary, 8)[0] * 16
    count = struct.unpack_from('<H', binary, 6)[0]
    table = struct.unpack_from('<H', binary, 24)[0]
    relocations = [struct.unpack_from('<HH', binary, table+4*i) for i in range(count)]
    # Discard only the appended Borland debug information, outside the MZ image.
    last, pages = struct.unpack_from('<HH', binary, 2)
    image = bytearray(binary[header:(pages-1)*512+(last or 512)])
    stub = (directory / 'logger.bin').read_bytes()
    if len(stub) > 0x6070-0x5e9a:
        raise ValueError('Logger exceeds replaced DemoLoop code region')
    image[0x5e9a:0x5e9a+len(stub)] = stub
    relocations = [(o, s) for o, s in relocations if not 0x5e9a <= s*16+o < 0x5e9a+len(stub)]
    # IN_Ack is only used for startup/menus in this private demo runner.
    image[0x18262] = 0xcb
    if fast:
        # Skip the wall-clock wait, retaining the original TimeCount update and
        # tics=DEMOTICS. Sound is disabled. Never used in the native runtime.
        image[0xd385:0xd3a0] = bytes([0x90])*27
    for address, opcode, offset in ((0x5e8a, 0xea, 0x15aa), (0xe032, 0x9a, 0x15aa+labels['frame'])):
        image[address:address+5] = struct.pack('<BHH', opcode, offset, 0x48f)
        relocations = [(o, s) for o, s in relocations if s*16+o != address+3]
        relocations.append(((address+3)%16, (address+3)//16))
    # Logger calls the original PlayDemo and UpdateSoundLoc routines.
    relocations += [(0x15aa+o, 0x48f) for o in (labels['playseg'], labels['soundseg'])]
    if table+4*len(relocations) > header:
        raise ValueError('MZ relocation table has no room')
    for i, entry in enumerate(relocations):
        struct.pack_into('<HH', binary, table+4*i, *entry)
    struct.pack_into('<H', binary, 6, len(relocations))
    binary = binary[:header]+image
    struct.pack_into('<HH', binary, 2, len(binary)%512, (len(binary)+511)//512)
    (directory / 'TRACE.EXE').write_bytes(binary)
    for name in ASSETS:
        candidates = [p for p in Path(data).iterdir() if p.name.upper() == name+'.WL6']
        if len(candidates) != 1:
            raise ValueError('Missing/ambiguous original '+name+'.WL6')
        shutil.copyfile(candidates[0], directory / (name+'.WL6'))
    # DOS config: 7 x 66-byte high scores, then fixed original settings.
    # Zero sound/music/digi modes and input switches; the demos supply input.
    config = bytearray(522)
    struct.pack_into('<HH', config, 518, 15, 5)
    (directory / 'CONFIG.WL6').write_bytes(config)
    print('Private reference built:', directory / 'TRACE.EXE')
    print('Run: SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy dosbox')
    print('  -c "config -set cpu cycles=100000" -c "mount c '+str(directory)+'" -c c: -c "trace.exe nowait" -c exit')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--fast', action='store_true')
    args = parser.parse_args()
    build(args.output, args.data, args.fast)
