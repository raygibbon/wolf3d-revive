"""Bounded TOML, atomic output, profiles, native preferences and app restarts."""
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import tomllib
import unittest
from test_engine import ROOT
from game_fixtures import finale_files

class ConfigTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        sdk=Path(os.environ.get('SDL_SDK',ROOT.parent/'cart/vendor/SDL'))
        if not (sdk/'build/libSDL3.a').is_file(): raise unittest.SkipTest('SDL SDK unavailable')
        cls.build=tempfile.TemporaryDirectory(prefix='w3r-config-build-');cls.addClassCleanup(cls.build.cleanup)
        cls.binary=Path(cls.build.name)/'config-tests'
        flags=['-std=c11','-Wall','-Wextra','-Wpedantic','-Wconversion','-Wshadow','-Werror','-g','-DWOLF3D_CONFIG_TEST','-DWOLF3D_DISPLAY_TEST']
        if os.environ.get('SANITIZE')=='1': flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        sources=sorted(p for p in (ROOT/'src').rglob('*.c') if p.name!='main.c')
        subprocess.run(shlex.split(os.environ.get('CC','gcc'))+flags+['-I'+str(ROOT/'src'),'-I'+str(sdk/'include'),str(ROOT/'tests/config_tests.c'),*map(str,sources),str(sdk/'build/libSDL3.a'),'-pthread','-lm','-o',str(cls.binary)],check=True)
        cls.app=Path(os.environ.get('WOLF3D_BINARY',ROOT/'build/cart-sdl/linux/wolf3d-revive'))
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory(prefix='w3r-config-');self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name);self.user=self.root/'user';self.user.mkdir();self.data=self.root/'data';self.data.mkdir()
        self.env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_RENDER_DRIVER='software',SDL_AUDIODRIVER='dummy',XDG_DATA_HOME=str(self.root/'xdg'))
        self.path=self.user/'config.toml'
    def run_config(self,mode='inspect',data=None,user=None):
        r=subprocess.run([str(self.binary),str(user or self.user),str(data or self.data),mode],env=self.env,text=True,capture_output=True)
        self.assertEqual(r.returncode,0,r.stdout+r.stderr)
        return json.loads(r.stdout.strip().splitlines()[-1])
    def write(self,text): self.path.write_bytes(text.encode() if isinstance(text,str) else text)
    def test_missing_defaults(self):
        s=self.run_config();self.assertEqual(s['result'],0);self.assertEqual((s['preset'],s['view'],s['music'],s['effects']),(0,15,70,85));self.assertFalse(self.path.exists())
    def test_partial_modern_defaults(self):
        self.write('[controls]\npreset="modern"')
        s=self.run_config();self.assertEqual(s['result'],1);self.assertEqual(s['preset'],1);self.assertTrue(s['profiles'][1]['mouse']);self.assertEqual(s['profiles'][1]['bindings'][0],[26,82])
    def test_values_comments_crlf_and_unknown(self):
        self.write('schema_version = 1\r\n[ controls ] # comment\r\npreset = "mod\\u0065rn"\r\nmouse_enabled=false\r\nmouse_sensitivity=+17\r\n[unknown]\nvalue="a#b" # comment\n[ audio ]\nmusic_volume=0x64\neffects_volume=8_5\nmusic_enabled=false\n[classic]\nview_size=0b10011\n[display]\nwindow_width=0x3e8\nwindow_height=600\ninteger_scaling=true')
        s=self.run_config();self.assertEqual(s['result'],1);self.assertEqual((s['preset'],s['music'],s['effects'],s['view'],s['width'],s['height']),(1,100,85,19,1000,600));self.assertFalse(s['profiles'][1]['mouse']);self.assertFalse(s['music_on'])
    def test_unsupported_values_fall_back(self):
        self.write('[controls]\npreset="unknown"\nmouse_sensitivity=999\nmouse_enabled="false"\n[audio]\nmusic_volume=-1\neffects_volume=101\neffects_enabled=0\n[display]\nwindow_width=0\nwindow_height=999999\n[classic]\nview_size=3\n')
        s=self.run_config();self.assertEqual(s['result'],1);self.assertEqual((s['preset'],s['music'],s['effects'],s['view'],s['width'],s['height']),(0,70,85,15,960,720));self.assertTrue(s['effects_on'])
    def test_visual_settings_and_invalid_values(self):
        self.write('[display]\nrenderer="high_resolution"\nrender_auto=true\nrender_width=1920\nrender_height=1080\naspect_ratio="32:9"\nfilter="linear"\nsmooth_rendering=true\nvsync=true\n')
        s=self.run_config();self.assertEqual((s['high_resolution'],s['render_auto'],s['render_width'],s['render_height'],s['aspect'],s['linear'],s['smooth']),(1,1,1920,1080,5,1,1))
        self.write('[display]\nrenderer="invalid"\nrender_auto=1\nrender_width=1234\nrender_height=999\naspect_ratio="0:0"\nfilter="cubic"\nsmooth_rendering="yes"\nvsync="yes"\n')
        s=self.run_config();self.assertEqual((s['high_resolution'],s['render_auto'],s['render_width'],s['render_height'],s['aspect'],s['linear'],s['vsync'],s['smooth']),(0,0,640,480,0,0,0,0))

    def test_obsolete_frame_limit_migration(self):
        self.write('[display]\nsmooth_rendering=true\npresentation_fps=165\n')
        state=self.run_config('migrate');self.assertTrue(state['smooth'])
        self.assertNotIn('presentation_fps',tomllib.loads(self.path.read_text())['display'])

    def test_duplicate_and_malformed_files(self):
        cases=['[controls]\npreset="modern"\npreset="classic"','[controls]\npreset="modern"\n[controls]\nmouse_enabled=true','[audio]\nx=1\nx=2','[audio] broken','[audio\nmusic_volume=1','[audio]\nmusic_volume=1junk','[audio]\nmusic_volume=01','[audio]\nmusic_volume=1__0','[audio]\nmusic_volume=9223372036854775808','[audio]\nmusic_volume=-9223372036854775809','[audio]\nmusic_volume=0xffffffffffffffffffff','[controls]\npreset="modern','[controls]\npreset="bad\\q"','controls=1\n[controls]\npreset="modern"','[display]\nwindow_width =','[audio]\nmusic_volume=1.2', '["audio"]\nmusic_volume=1']
        for text in cases:
            with self.subTest(text=text):
                self.write(text);s=self.run_config();self.assertEqual(s['result'],2);self.assertEqual(s['preset'],0);self.assertEqual(s['music'],70);self.assertEqual(self.path.read_text(),text)
    def test_binary_encoding_and_limits(self):
        for data in (b'[audio]\nx=1\x00',b'#\xff',b'#\xc0\xaf',b'#\xed\xa0\x80',b'#\xf4\x90\x80\x80',b'#'+b'x'*1025,b'#'+b'x'*65537,b'[audio]\n'+b'k'*64+b'=1',b'[audio]\n'+b'\n'.join(f'k{n}=1'.encode() for n in range(257)),b'\n'.join(f'[s{n}]'.encode() for n in range(65))):
            with self.subTest(size=len(data)):
                self.write(data);self.assertEqual(self.run_config()['result'],2)
    def test_unknown_schema_preserved(self):
        self.write('schema_version=2\n[controls]\npreset="modern"');s=self.run_config();self.assertEqual((s['result'],s['preset']),(3,0))
    def test_binding_swap_and_unbind(self):
        self.write('[controls]\npreset="modern"\nbind_0_0=22\nbind_1_0=26\nbind_4_0=0\n')
        s=self.run_config();self.assertEqual(s['profiles'][1]['bindings'][0],[22,82]);self.assertEqual(s['profiles'][1]['bindings'][1],[26,81]);self.assertEqual(s['profiles'][1]['bindings'][4],[0,0])
    def test_invalid_binding_group_is_atomic(self):
        for bad in ('bind_8_0=8','bind_8_0=58','bind_8_0=69','bind_8_0=41','bind_8_0=1','bind_8_0=511','bind_8_0=-1','bind_8_0=512','bind_8_0="x"','bind_14_2=2','bind_9999999999999999999999_0=2','bind_0_0trailing=20','bind_00_0=20'):
            with self.subTest(bad=bad):
                self.write('[controls]\npreset="classic"\nmouse_sensitivity=17\nbind_0_0=8\n'+bad)
                s=self.run_config();p=s['profiles'][0];self.assertEqual(p['bindings'][0],[82,96]);self.assertEqual(p['bindings'][8],[224,228]);self.assertEqual(p['sensitivity'],17)
    def test_profile_switch_and_explicit_reset(self): self.run_config('profiles')
    def test_roundtrip_and_complete_schema(self):
        s=self.run_config('roundtrip');self.assertTrue(s['fullscreen']);self.assertTrue(s['integer']);self.assertEqual((s['width'],s['height']),(1000,600))
        parsed=tomllib.loads(self.path.read_text());self.assertEqual(parsed['schema_version'],1);self.assertEqual(parsed['classic_controls']['bind_0_0'],8);self.assertEqual(parsed['modern_controls']['bind_0_0'],22)
        self.write('[display]\nwindow_width=1\nwindow_height=1\ninteger_scaling=true');tiny=self.run_config();self.assertEqual((tiny['width'],tiny['height'],tiny['integer']),(1,1,1))
    def test_window_aspects_survive_restart(self):
        for width,height in ((640,480),(960,720),(1280,960),(960,540),(1280,720),(1600,900),(1920,1080),
                             (960,600),(1280,800),(1600,1000),(1920,1200),(1680,720),(2560,1080),
                             (1920,540),(2560,720),(1001,601)):
            with self.subTest(size=(width,height)):
                self.write(f'[display]\nwindow_width={width}\nwindow_height={height}\nrenderer="high_resolution"\naspect_ratio="16:10"\ninteger_scaling=true\n')
                self.run_config('migrate')
                restored=self.run_config()
                self.assertEqual((restored['width'],restored['height'],restored['aspect'],restored['integer'],restored['high_resolution']),
                                 (width,height,3,1,1))
    def test_atomic_failures_preserve_file(self): self.run_config('atomic');self.assertEqual(list(self.user.glob('*.tmp-*')),[])
    @unittest.skipIf(os.name=='nt','POSIX permission test')
    def test_read_only_preferences_preserved(self):
        self.run_config('save');before=self.path.read_bytes();self.user.chmod(0o555)
        try: self.assertFalse(self.run_config('save')['saved']);self.assertEqual(self.path.read_bytes(),before)
        finally: self.user.chmod(0o755)
    @unittest.skipIf(os.name=='nt','POSIX symlink test')
    def test_atomic_replace_does_not_follow_asset_symlink(self):
        asset=self.data/'AUDIOT.WL6';asset.write_bytes(b'private sentinel');self.path.symlink_to(asset);self.assertTrue(self.run_config('save')['saved']);self.assertEqual(asset.read_bytes(),b'private sentinel');self.assertFalse(self.path.is_symlink())
    def test_competing_configuration_writers(self):
        expected=[]
        for mode in ('seed','save'):
            target=self.root/mode;target.mkdir();self.run_config(mode,user=target);expected.append(tomllib.loads((target/'config.toml').read_text()))
        children=[subprocess.Popen([str(self.binary),str(self.user),str(self.data),mode],env=self.env,stdout=subprocess.PIPE,stderr=subprocess.PIPE) for mode in ('seed','save')*3]
        for child in children:
            out,err=child.communicate();self.assertEqual(child.returncode,0,(out+err).decode())
        self.assertIn(tomllib.loads(self.path.read_text()),expected);self.assertEqual(list(self.user.glob('*.tmp-*')),[])
    def test_invalid_and_long_paths(self): self.run_config('paths')
    @unittest.skipIf(os.name=='nt','POSIX read permission test')
    def test_unreadable_config_is_not_missing(self):
        self.run_config('save');self.path.chmod(0)
        try:self.assertEqual(self.run_config()['result'],4)
        finally:self.path.chmod(0o600)
    def test_display_failure_and_pending_intent(self): self.run_config('display')
    def test_unicode_config_directory(self): self.run_config('unicode');self.assertTrue((self.user/'café-設定/config.toml').is_file())
    def test_bounded_random_corruption(self): self.run_config('fuzz')
    def test_game_save_rng_and_boss_music(self):
        finale_files(self.data);self.run_config('game');self.assertEqual((self.user/'slot1.w3s').read_bytes()[96:],(self.user/'slot2.w3s').read_bytes()[96:]);self.assertTrue(tomllib.loads(self.path.read_text())['audio']['music_enabled'])
    def test_owned_game_independence(self):
        if not os.environ.get('WOLF3D_DATA'): self.skipTest('Owned WL6 data unavailable')
        self.run_config('game',data=Path(os.environ['WOLF3D_DATA']));self.assertEqual((self.user/'slot1.w3s').read_bytes()[96:],(self.user/'slot2.w3s').read_bytes()[96:])
    def app_run(self,env=None):
        r=subprocess.run([str(self.app),'--data',str(self.data),'--menu','--no-audio','--frames','2'],env=env or self.env,text=True,capture_output=True)
        return r
    def test_application_restart_read_only_data(self):
        finale_files(self.data);prefs=Path(self.env['XDG_DATA_HOME'])/'wolf3d-revive';prefs.mkdir(parents=True);self.run_config('seed',user=prefs);before=tomllib.loads((prefs/'config.toml').read_text());files={p.name:p.read_bytes() for p in self.data.iterdir()};self.data.chmod(0o555)
        try:
            for _ in range(3):
                r=self.app_run();self.assertEqual(r.returncode,0,r.stderr);self.assertEqual(tomllib.loads((prefs/'config.toml').read_text()),before)
            self.assertEqual(files,{p.name:p.read_bytes() for p in self.data.iterdir()})
        finally:self.data.chmod(0o755)
    def test_application_invalid_config_not_replaced(self):
        finale_files(self.data);prefs=Path(self.env['XDG_DATA_HOME'])/'wolf3d-revive';prefs.mkdir(parents=True)
        for content in (b'[audio]\nmusic_volume=1\nmusic_volume=2',b'schema_version=999'):
            (prefs/'config.toml').write_bytes(content);r=self.app_run();self.assertEqual(r.returncode,0,r.stderr);self.assertIn('Configuration:',r.stderr);self.assertEqual((prefs/'config.toml').read_bytes(),content)
    def test_application_missing_config_and_startup_failure(self):
        finale_files(self.data);r=self.app_run();self.assertEqual(r.returncode,0,r.stderr);prefs=Path(self.env['XDG_DATA_HOME'])/'wolf3d-revive/config.toml';self.assertEqual(tomllib.loads(prefs.read_text())['display']['window_width'],960)
        before=prefs.read_bytes();r=self.app_run(dict(self.env,SDL_VIDEODRIVER='nonexistent-test-driver'));self.assertNotEqual(r.returncode,0);self.assertEqual(prefs.read_bytes(),before)
    def test_sdl_preference_path(self):
        self.run_config('prefpath');self.assertTrue((Path(self.env['XDG_DATA_HOME'])/'wolf3d-revive/config.toml').is_file())
