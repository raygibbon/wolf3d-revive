#!/usr/bin/env python3
"""Private original-DOS UI/progress pixel oracle; no commercial assets emitted to Git."""
# SPDX-License-Identifier: GPL-2.0-only
import hashlib
from pathlib import Path
import shutil
import struct
import subprocess
from demo_reference import ROOT, EXE_SHA, MAP_SHA, ASSETS


def build(directory, data):
    directory, data = Path(directory).resolve(), Path(data).resolve()
    if directory == data or (directory.is_relative_to(ROOT) and not directory.is_relative_to(ROOT/'build')):
        raise ValueError('Use a separate private output directory')
    if directory.exists() and any(directory.iterdir()):
        raise ValueError('Output directory must be new or empty')
    original = (ROOT/'WOLFSRC/WOLF3D.EXE').read_bytes()
    symbols = (ROOT/'WOLFSRC/WOLF3D.MAP').read_bytes()
    if hashlib.sha256(original).hexdigest()!=EXE_SHA or hashlib.sha256(symbols).hexdigest()!=MAP_SHA:
        raise ValueError('Unknown historical EXE/MAP; refusing instrumentation')
    directory.mkdir(parents=True,exist_ok=True)
    subprocess.run(['as','--32',str(ROOT/'scripts/ui_reference.s'),'-o',str(directory/'ui.o')],check=True)
    subprocess.run(['ld','-m','elf_i386','-Ttext=0x15aa','--oformat','binary',str(directory/'ui.o'),'-o',str(directory/'ui.bin')],check=True)
    labels={line.split()[2]:int(line.split()[0],16) for line in subprocess.check_output(['nm','-n',str(directory/'ui.o')],text=True).splitlines() if len(line.split())==3}
    stub=(directory/'ui.bin').read_bytes()
    if len(stub)>0x6070-0x5e9a:
        raise ValueError('Oracle exceeds unused DemoLoop region')
    binary=bytearray(original)
    header=struct.unpack_from('<H',binary,8)[0]*16
    count=struct.unpack_from('<H',binary,6)[0];table=struct.unpack_from('<H',binary,24)[0]
    relocations=[struct.unpack_from('<HH',binary,table+4*i) for i in range(count)]
    last,pages=struct.unpack_from('<HH',binary,2)
    image=bytearray(binary[header:(pages-1)*512+(last or 512)])
    image[0x5e9a:0x5e9a+len(stub)]=stub
    relocations=[(o,s) for o,s in relocations if not 0x5e9a<=s*16+o<0x5e9a+len(stub)]
    image[0x18262]=0xcb  # Skip only startup acknowledgement, as demo_reference.
    image[0x5e8a:0x5e8f]=struct.pack('<BHH',0xea,0x15aa,0x48f)
    relocations=[(o,s) for o,s in relocations if s*16+o!=0x5e8d]
    relocations.append((0x5e8d%16,0x5e8d//16))
    relocations += [(0x15aa+labels[name],0x48f) for name in ('setupseg','clearseg','messageseg','clear2seg','diskseg','clear3seg','latchseg','updateseg','progressseg','clear4seg','pausepicseg','update2seg','newseg','scoresseg')]
    if table+4*len(relocations)>header:
        raise ValueError('Relocation table capacity exceeded')
    for i,entry in enumerate(relocations):struct.pack_into('<HH',binary,table+4*i,*entry)
    struct.pack_into('<H',binary,6,len(relocations))
    binary=binary[:header]+image
    struct.pack_into('<HH',binary,2,len(binary)%512,(len(binary)+511)//512)
    (directory/'UI.EXE').write_bytes(binary)
    for name in ASSETS:
        candidates=[p for p in data.iterdir() if p.name.upper()==name+'.WL6']
        if len(candidates)!=1:raise ValueError('Missing/ambiguous '+name)
        shutil.copyfile(candidates[0],directory/(name+'.WL6'))
    # Private DOS settings: silent output/input, original view size. Never copy
    # or update the user's CONFIG/saves; scores use the historical default table.
    config=bytearray(522);struct.pack_into('<HH',config,518,15,5)
    # Original ID_US_1.C defaults, encoded into the DOS 66-byte records.
    names=("id software-'92", "Adrian Carmack", "John Carmack", "Kevin Cloud",
           "Tom Hall", "John Romero", "Jay Wilbur")
    for i,name in enumerate(names):
        encoded=name.encode('ascii')
        config[i*66:i*66+len(encoded)]=encoded
        struct.pack_into('<IHH',config,i*66+58,10000,1,0)
    (directory/'CONFIG.WL6').write_bytes(config)
    return directory/'UI.EXE'


if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    print(build(args.output,args.data))
