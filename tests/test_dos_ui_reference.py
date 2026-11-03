"""Actual historical executable UI pixels; owned archives stay private."""
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile
import unittest
from test_engine import ROOT

class DOSUIReferenceTests(unittest.TestCase):
    def test_original_executable_dialog_pixels(self):
        if not os.environ.get('WOLF3D_DATA'):self.skipTest('Owned WL6 unavailable')
        if not shutil.which('dosbox'):self.skipTest('DOSBox unavailable')
        with tempfile.TemporaryDirectory(prefix='w3r-dos-ui-') as temporary:
            root=Path(temporary);reference=root/'reference'
            subprocess.run([sys.executable,str(ROOT/'scripts/ui_reference.py'),'--data',os.environ['WOLF3D_DATA'],'--output',str(reference)],check=True,capture_output=True)
            env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy')
            result=subprocess.run(['dosbox','-c','config -set cpu cycles=100000','-c',f'mount c {reference}','-c','c:','-c','ui.exe nowait','-c','exit'],env=env,capture_output=True,timeout=60)
            self.assertEqual(result.returncode,0,result.stderr.decode(errors='replace'))
            self.assertEqual((reference/'PIXELS.BIN').stat().st_size,30*64000)
            binary=root/'compare';flags=['-std=c11','-O1','-Wall','-Wextra','-Wpedantic','-Wconversion','-Wshadow','-Werror']
            if os.environ.get('SANITIZE')=='1':flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            sources=sorted((ROOT/'src/engine').glob('*.c'))
            subprocess.run(shlex.split(os.environ.get('CC','gcc'))+flags+['-I'+str(ROOT/'src'),str(ROOT/'tests/dos_ui_reference.c'),*map(str,sources),str(ROOT/'src/platform/data_file.c'),str(ROOT/'src/platform/save_file.c'),'-lm','-o',str(binary)],check=True)
            subprocess.run([str(binary),os.environ['WOLF3D_DATA'],str(reference/'PIXELS.BIN')],check=True)
