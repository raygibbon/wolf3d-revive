"""Synthetic-only fixtures; optional WOLF3D_DATA tests owned data in place."""
import os
import re
from pathlib import Path
import shlex
import struct
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


def dictionary():
    nodes = []
    level = list(range(256))
    while len(level) > 1:
        parent = []
        for i in range(0, len(level), 2):
            parent.append(256 + len(nodes))
            nodes.append((level[i], level[i + 1]))
        level = parent
    return b"".join(struct.pack("<HH", *n) for n in nodes) + bytes(4)


def encode(raw):
    # Balanced test tree: symbol bits MSB-first, stored in LSB-first stream bytes.
    return bytes(int(f"{v:08b}"[::-1], 2) for v in raw)


def fixtures(directory, lowercase=False, ui=False):
    pic = struct.pack("<HH", 320, 200) * 132
    pixels = bytes((x * 13 + y * 7) % 256 for y in range(200) for x in range(320))
    planar = bytes(pixels[y * 320 + x] for plane in range(4)
                   for y in range(200) for x in range(plane, 320, 4))
    chunks = [struct.pack("<I", len(raw)) + encode(raw) for raw in (pic, planar)]
    offsets = [0xffffff] * 150
    offsets[0], offsets[87], offsets[149] = 0, len(chunks[0]), sum(map(len, chunks))
    if ui:
        sizes = {10: (152,48), 11: (24,16), 12: (24,16), 18: (104,16),
                 28: (200,24), 29: (200,24), 38: (32,8), 39: (32,8), 40: (32,8), 87: (320,200),
                 89: (320,200), 90: (224,56)}
        dims = bytearray(struct.pack("<HH",8,8)*132)
        raw = {87: planar}
        for chunk,(w,h) in sizes.items():
            struct.pack_into("<HH",dims,(chunk-3)*4,w,h)
            if chunk != 87: raw[chunk] = bytes([chunk])*(w*h)
        for number,height in ((1,10),(2,13)):
            font = bytearray(770)
            struct.pack_into("<H",font,0,height)
            for ch in range(1,256):
                width = 3+ch%3
                struct.pack_into("<H",font,2+ch*2,len(font))
                font[514+ch] = width
                font.extend(bytes(1 if ch != 32 and (x+y+ch)%3 else 0
                                  for y in range(height) for x in range(width)))
            raw[number] = font
        raw[0] = dims
        offsets = [0xffffff]*150
        chunks = []
        cursor = 0
        for chunk,content in sorted(raw.items()):
            offsets[chunk] = cursor
            compressed = struct.pack("<I",len(content))+encode(content)
            chunks.append(compressed); cursor += len(compressed)
        offsets[149] = cursor
    data = {"VGADICT.WL6": dictionary(),
            "VGAHEAD.WL6": b"".join(x.to_bytes(3, "little") for x in offsets),
            "VGAGRAPH.WL6": b"".join(chunks)}
    # The title milestone only checks presence/readability of non-graphics archives.
    data.update({name: b"synthetic placeholder" for name in
                 ("VSWAP.WL6", "GAMEMAPS.WL6", "MAPHEAD.WL6", "AUDIOT.WL6", "AUDIOHED.WL6")})
    for name, content in data.items():
        (directory / (name.lower() if lowercase else name)).write_bytes(content)
    return pixels


def original_palette():
    """Independent oracle from the tracked OMF resource, not the generated C table."""
    data = (ROOT / "WOLFSRC/OBJ/GAMEPAL.OBJ").read_bytes()
    pos = 0
    while pos < len(data):
        length = int.from_bytes(data[pos + 1:pos + 3], "little")
        if data[pos] == 0xa0:
            assert sum(data[pos:pos + 3 + length]) % 256 == 0
            payload = data[pos + 3:pos + 2 + length]
            assert payload[:3] == b"\x01\x00\x00"
            assert len(payload[3:]) == 768
            return payload[3:]
        pos += length + 3
    raise AssertionError("Missing original palette")


def expected_rgba(pixels):
    pal = original_palette()
    expanded = bytes(v * 4 + v // 16 for v in pal)
    return b"".join(expanded[i * 3:i * 3 + 3] + b"\xff" for i in pixels)


class EngineTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.build = tempfile.TemporaryDirectory(prefix="w3r-tests-")
        cls.addClassCleanup(cls.build.cleanup)
        cls.output = Path(cls.build.name)
        flags = ["-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Wconversion", "-Wshadow", "-Werror", "-g"]
        if os.environ.get("SANITIZE") == "1":
            flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
        cc = shlex.split(os.environ.get("CC", "gcc"))
        core = [str(p) for p in sorted((ROOT / "src/engine").glob("*.c"))]
        subprocess.run(cc + flags + ["-I" + str(ROOT / "src"), str(ROOT / "tests/engine_tests.c"),
                                    *core, str(ROOT / "src/platform/data_file.c"), str(ROOT / "src/platform/save_file.c"), "-lm", "-o", str(cls.output / "engine-tests")], check=True)
        subprocess.run(cc + flags + ["-I" + str(ROOT / "src"), str(ROOT / "tests/ui_tests.c"),
                                    *core, str(ROOT / "src/platform/data_file.c"), str(ROOT / "src/platform/save_file.c"), "-lm", "-o", str(cls.output / "ui-tests")], check=True)
        subprocess.run(cc + flags + ["-I" + str(ROOT / "src"), str(ROOT / "tests/game_tests.c"),
                                    *core, str(ROOT / "src/platform/data_file.c"), str(ROOT / "src/platform/save_file.c"), "-lm", "-o", str(cls.output / "game-tests")], check=True)
        subprocess.run(cc + flags + ["-I" + str(ROOT / "src"), str(ROOT / "tests/combat_tests.c"),
                                    *core, str(ROOT / "src/platform/data_file.c"), str(ROOT / "src/platform/save_file.c"), "-lm", "-o", str(cls.output / "combat-tests")], check=True)
        subprocess.run(cc + flags + ["-I" + str(ROOT / "src"), str(ROOT / "tests/campaign_tests.c"),
                                    *core, str(ROOT / "src/platform/data_file.c"), str(ROOT / "src/platform/save_file.c"), "-lm", "-o", str(cls.output / "campaign-tests")], check=True)
        subprocess.run(cc + flags + ["-I" + str(ROOT / "src"), str(ROOT / "tests/boss_tests.c"),
                                    *core, str(ROOT / "src/platform/data_file.c"), str(ROOT / "src/platform/save_file.c"), "-lm", "-o", str(cls.output / "boss-tests")], check=True)
        subprocess.run(cc + flags + ["-I" + str(ROOT / "src"), str(ROOT / "tests/finale_tests.c"),
                                    *core, str(ROOT / "src/platform/data_file.c"), str(ROOT / "src/platform/save_file.c"), "-lm", "-o", str(cls.output / "finale-tests")], check=True)
        cls.engine = cls.output / "engine-tests"
        cls.compiler = cc + flags
        cls.binary = Path(os.environ.get("WOLF3D_BINARY", ROOT / "build/cart-sdl/linux/wolf3d-revive"))
        if not cls.binary.is_file():
            raise RuntimeError("Build the app with CART first (or set WOLF3D_BINARY)")

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="w3r-data-")
        self.addCleanup(self.temp.cleanup)
        self.data = Path(self.temp.name)
        self.pixels = fixtures(self.data)

    def app(self, success=False, *extra):
        result = subprocess.run([str(self.__class__.binary), "--data", str(self.data),
                                 "--check-assets", *extra], text=True, capture_output=True)
        self.assertEqual(result.returncode, 0 if success else 1, result.stdout + result.stderr)
        return result

    def test_memory_huffman_framebuffer_palette(self):
        subprocess.run([str(self.engine)], check=True)

    def compare_assets(self, directory, pixels):
        dump = self.data / "decoded.bin"
        subprocess.run([str(self.engine), str(directory), str(dump)], check=True)
        actual = dump.read_bytes()
        self.assertEqual(actual[:64000], pixels)
        self.assertEqual(actual[64000:], expected_rgba(pixels))

    def test_synthetic_archive_cache_and_pixels(self):
        self.app(True)
        self.compare_assets(self.data, self.pixels)

    def test_original_ui_synthetic(self):
        fixtures(self.data,ui=True)
        subprocess.run([str(self.output / "ui-tests"),str(self.data)],check=True)
        result = subprocess.run([str(self.binary),"--data",str(self.data),"--check-ui"],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stderr)

    def test_mixed_case_files(self):
        for p in self.data.iterdir():
            p.rename(p.with_name("".join(c.lower() if i%2 else c.upper() for i,c in enumerate(p.name))))
        self.app(True)

    def test_ambiguous_case_files(self):
        p = self.data / "VGADICT.WL6"
        raw = p.read_bytes(); p.unlink()
        (self.data / "vGaDiCt.WL6").write_bytes(raw)
        (self.data / "VgAdIcT.wl6").write_bytes(raw)
        self.assertIn("ambiguous",self.app().stderr)

    def test_campaign_progression(self):
        from game_fixtures import campaign_files
        campaign_files(self.data)
        subprocess.run([str(self.output / "campaign-tests"), str(self.data)], check=True)

    def test_all_original_encounter_codes(self):
        from game_fixtures import game_files, map_files
        game_files(self.data)
        for code in (160, 178, 179, 196, 215, 99):
            with self.subTest(code=code):
                map_files(self.data, lambda p: p[1].__setitem__(15*64+15, code))
                result = subprocess.run([str(self.binary), "--data", str(self.data), "--check-level"],
                                        capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)

    def test_special_actors(self):
        from game_fixtures import boss_files
        boss_files(self.data)
        subprocess.run([str(self.output / "boss-tests"), str(self.data)], check=True)

    def test_special_actor_states_match_dos_source(self):
        pattern = r"statetype (s_(?:boss|gretel|blinky|clyde|pinky|inky)\w*)\s*=\s*({[^;]+});"
        def states(path):
            return {name: re.sub(r"\s+", "", value) for name, value in
                    re.findall(pattern, path.read_text(errors="replace"))}
        expected = states(ROOT / "WOLFSRC/WL_ACT2.C")
        self.assertEqual(len(expected), 46)
        self.assertEqual(states(ROOT / "src/engine/WL_STATE.c"), expected)

    def test_finales_and_projectiles(self):
        from game_fixtures import finale_files
        finale_files(self.data)
        subprocess.run([str(self.output / "finale-tests"), str(self.data)], check=True)

    def test_remaining_boss_states_match_dos_source(self):
        pattern = r"statetype (s_(?:schabb|gift|fat|fake|mecha|hitler|needle|fire|rocket|smoke|boom|bj|deathcam)\w*)\s*=\s*({[^;]+});"
        def states(paths):
            return {name: re.sub(r"\s+", "", value) for path in paths for name, value in
                    re.findall(pattern, path.read_text(errors="replace"))}
        expected = states([ROOT / "WOLFSRC/WL_ACT2.C"])
        self.assertGreater(len(expected), 100)
        self.assertEqual(states([ROOT / "src/engine/WL_ACT2.c", ROOT / "src/engine/WL_VICTORY.c"]), expected)

    def test_missing_special_actor_sprites(self):
        from game_fixtures import boss_files
        boss_files(self.data)
        names = re.findall(r"\bSPR_\w+", (ROOT / "src/engine/WL_SPRITES.h").read_text())
        path = self.data / "VSWAP.WL6"
        saved = path.read_bytes()
        count, start, end = struct.unpack_from("<HHH", saved)
        for shape in ("SPR_BOSS_DEAD", "SPR_GRETEL_SHOOT3", "SPR_CLYDE_W2", "SPR_HYPO3",
                      "SPR_ROCKET_8", "SPR_FIRE2", "SPR_HITLER_DEAD", "SPR_BJ_JUMP4", "SPR_DEATHCAM"):
            with self.subTest(shape=shape):
                page = start + names.index(shape)
                self.assertLess(page, end)
                bad = bytearray(saved)
                struct.pack_into("<I", bad, 6 + page*4, 0)
                struct.pack_into("<H", bad, 6 + count*4 + page*2, 0)
                path.write_bytes(bad)
                self.level_check()

    def test_special_actor_limits_and_invalid_spawns(self):
        from game_fixtures import game_files, map_files
        game_files(self.data)
        for code in (214, 197, 224):
            with self.subTest(code=code):
                def populate(planes, count):
                    for i in range(count):
                        planes[1][(10+i//50)*64+1+i%50] = code
                map_files(self.data, lambda p: populate(p, 150))
                result = subprocess.run([str(self.binary), "--data", str(self.data), "--check-level"],
                                        capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                map_files(self.data, lambda p: populate(p, 151))
                self.assertIn("Invalid/too many actors", self.level_check().stderr)
                map_files(self.data, lambda p: p[1].__setitem__(15*64, code))
                self.assertIn("Invalid/too many actors", self.level_check().stderr)

    def test_original_combat(self):
        from game_fixtures import combat_files
        combat_files(self.data)
        subprocess.run([str(self.output / "combat-tests"), str(self.data)], check=True)

    def test_game_synthetic(self):
        from game_fixtures import game_files
        game_files(self.data)
        subprocess.run([str(self.output / "game-tests"),str(self.data)],check=True)
        result=subprocess.run([str(self.binary),"--data",str(self.data),"--check-level"],text=True,capture_output=True)
        self.assertEqual(result.returncode,0,result.stderr)

    def level_check(self):
        result=subprocess.run([str(self.binary),"--data",str(self.data),"--check-level"],text=True,capture_output=True)
        self.assertEqual(result.returncode,1,result.stdout+result.stderr)
        return result

    def test_invalid_objects(self):
        from game_fixtures import game_files,map_files
        game_files(self.data)
        def crowded(planes,code,count):
            for i in range(count): planes[1][(10+i//50)*64+1+i%50]=code
        mutations=[lambda p: p[1].__setitem__(1*64+6,108),
                   lambda p: p[1].__setitem__(4*64+62,112),
                   lambda p: crowded(p,108,151),lambda p: crowded(p,24,401)]
        for index,mutate in enumerate(mutations):
            with self.subTest(case=index): map_files(self.data,mutate);self.level_check()

    def test_offscreen_sprite_validation(self):
        from game_fixtures import game_files
        game_files(self.data)
        path=self.data/"VSWAP.WL6";saved=path.read_bytes()
        count,first,last=struct.unpack_from("<HHH",saved)
        page=last-1;offset=struct.unpack_from("<I",saved,6+page*4)[0]
        bad=bytearray(saved);struct.pack_into("<H",bad,offset,64)
        path.write_bytes(bad);self.level_check()
        bad=bytearray(saved);struct.pack_into("<I",bad,6+page*4,0);struct.pack_into("<H",bad,6+count*4+page*2,0)
        path.write_bytes(bad);self.level_check()

    def test_game_mixed_case(self):
        from game_fixtures import game_files
        game_files(self.data)
        for p in self.data.iterdir():
            p.rename(p.with_name("".join(c.lower() if i%2 else c.upper() for i,c in enumerate(p.name))))
        result=subprocess.run([str(self.binary),"--data",str(self.data),"--check-level"],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stderr)

    def test_invalid_spawns_and_walls(self):
        from game_fixtures import game_files,map_files
        game_files(self.data)
        mutations=[lambda p: p[1].__setitem__(2*64+2,0),
                   lambda p: p[1].__setitem__(3*64+3,19),
                   lambda p: p[0].__setitem__(2*64+2,1),
                   lambda p: p[0].__setitem__(1*64+1,64),
                   lambda p: p[0].__setitem__(1*64+1,65535)]
        for i,mutate in enumerate(mutations):
            with self.subTest(case=i):map_files(self.data,mutate);self.level_check()

    def test_overlapping_patrol_reservations(self):
        from game_fixtures import game_files, map_files
        game_files(self.data)
        def overlap(p):
            p[1][10*64+10]=108
            p[1][10*64+11]=114
        map_files(self.data,overlap)
        result=subprocess.run([str(self.binary),"--data",str(self.data),"--check-level"],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stderr)

    def test_static_inside_wall_is_original_map_data(self):
        from game_fixtures import game_files, map_files
        game_files(self.data)
        map_files(self.data, lambda p: p[1].__setitem__(1*64+6,69))
        result=subprocess.run([str(self.binary),"--data",str(self.data),"--check-level"],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stderr)

    def test_floor_beside_door_jamb(self):
        from game_fixtures import game_files, map_files
        game_files(self.data)
        map_files(self.data, lambda p: p[0].__setitem__(1*64+6,107))
        result=subprocess.run([str(self.binary),"--data",str(self.data),"--check-level"],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stderr)

    def test_unavailable_level_keeps_menu(self):
        fixtures(self.data,ui=True)
        env=dict(os.environ,SDL_VIDEODRIVER="dummy",SDL_RENDER_DRIVER="software",XDG_DATA_HOME=str(self.data/"user"))
        result=subprocess.run([str(self.binary),"--data",str(self.data),"--menu","--frames","2"],env=env,text=True,capture_output=True)
        self.assertEqual(result.returncode,0,result.stderr)
        self.assertIn("New Game unavailable",result.stderr)

    def test_malformed_map_archive(self):
        from game_fixtures import game_files
        game_files(self.data)
        path=self.data/"GAMEMAPS.WL6"; saved=path.read_bytes()
        variants=[]
        for at,value in ((8,0xffffffff),(8+18,63),(8+20,63),(8+12,1)):
            bad=bytearray(saved)
            struct.pack_into("<I" if at==8 else "<H",bad,at,value);variants.append(bad)
        start=struct.unpack_from("<I",saved,8)[0]
        for offset,value in ((start,0),(start,3),(start,65534),(start+2,0)):
            bad=bytearray(saved);struct.pack_into("<H",bad,offset,value);variants.append(bad)
        variants.extend((saved[:1],saved[:-1]))
        # Plane overlaps and an invalid first Carmack back-reference.
        bad=bytearray(saved);bad[12:16]=bad[8:12];variants.append(bad)
        start=struct.unpack_from("<I",saved,8)[0]
        bad=bytearray(saved);bad[start+2:start+5]=bytes((1,0xa7,0));variants.append(bad)
        for index,bad in enumerate(variants):
            with self.subTest(case=index):path.write_bytes(bad);self.level_check()
        path.write_bytes(saved)
        head=self.data/"MAPHEAD.WL6";data=head.read_bytes()
        for raw in (data[:1],data[:10],data[:2]+struct.pack("<I",0xffffffff)+data[6:]):
            head.write_bytes(raw);self.level_check()

    def test_invalid_map_semantics(self):
        from game_fixtures import game_files,map_files
        game_files(self.data)
        # Decode-preserving literal mutations at the first plane's first RLEW value.
        path=self.data/"GAMEMAPS.WL6";saved=path.read_bytes()
        start=struct.unpack_from("<I",saved,8)[0]
        # First plane begins expanded length, RLEW length, then tag/count/value.
        bad=bytearray(saved);struct.pack_into("<H",bad,start+8,107)
        path.write_bytes(bad);self.assertIn("perimeter",self.level_check().stderr)

    def test_malformed_vswap(self):
        from game_fixtures import game_files
        game_files(self.data)
        path=self.data/"VSWAP.WL6";saved=path.read_bytes()
        variants=[saved[:1],saved[:6],saved[:-1]]
        for at,fmt,value in ((0,"<H",5000),(2,"<H",9),(4,"<H",11),(6,"<I",1),(10,"<I",0xffffffff),(46,"<H",4095)):
            bad=bytearray(saved);struct.pack_into(fmt,bad,at,value);variants.append(bad)
        for index,bad in enumerate(variants):
            with self.subTest(case=index):path.write_bytes(bad);self.level_check()

    def test_lowercase_files(self):
        for p in self.data.iterdir(): p.rename(p.with_name(p.name.lower()))
        self.app(True)

    def test_missing_required_files(self):
        for name in ("VSWAP.WL6", "GAMEMAPS.WL6", "MAPHEAD.WL6", "AUDIOT.WL6", "AUDIOHED.WL6", "VGAHEAD.WL6", "VGADICT.WL6", "VGAGRAPH.WL6"):
            with self.subTest(name=name):
                path = self.data / name
                saved = path.read_bytes(); path.unlink()
                self.assertIn(name, self.app().stderr)
                path.write_bytes(saved)

    def test_empty_and_truncated_files(self):
        for name in ("VGADICT.WL6", "VGAHEAD.WL6", "VGAGRAPH.WL6", "MAPHEAD.WL6"):
            path = self.data / name
            saved = path.read_bytes()
            for size in (0, 1, len(saved) - 1):
                if name == "MAPHEAD.WL6" and size != 0: continue
                with self.subTest(name=name, size=size):
                    path.write_bytes(saved[:size]); self.app()
            path.write_bytes(saved)

    def test_invalid_dictionary(self):
        path = self.data / "VGADICT.WL6"; saved = path.read_bytes()
        for value in (511, 65535, 510):
            with self.subTest(value=value):
                bad = bytearray(saved); struct.pack_into("<H", bad, 254 * 4, value)
                path.write_bytes(bad); self.app()

    def test_bad_offsets(self):
        path = self.data / "VGAHEAD.WL6"; saved = path.read_bytes()
        for index, value in ((0, 0xffffff), (1, 999999), (87, 1), (149, 0xffffff), (149, 1)):
            with self.subTest(index=index, value=value):
                bad = bytearray(saved); bad[index * 3:index * 3 + 3] = value.to_bytes(3, "little")
                path.write_bytes(bad); self.app()

    def test_bad_picture_table(self):
        path = self.data / "VGAGRAPH.WL6"; saved = path.read_bytes()
        bad = bytearray(saved); struct.pack_into("<I", bad, 0, 1)
        path.write_bytes(bad)
        self.assertIn("picture table", self.app().stderr)
        bad = bytearray(saved)
        # Width field for TITLEPIC (picture 84), in our eight-bit synthetic tree.
        offset = 4 + (87 - 3) * 4
        bad[offset:offset + 2] = encode(struct.pack("<H", 316))
        path.write_bytes(bad)
        self.assertIn("320x200", self.app().stderr)

    def test_truncated_huffman_stream_and_expansion_bomb(self):
        path = self.data / "VGAGRAPH.WL6"; saved = path.read_bytes()
        offset = 532
        for size in (0, 63999, 64001, 0xffffffff):
            bad = bytearray(saved); struct.pack_into("<I", bad, offset, size)
            with self.subTest(size=size): path.write_bytes(bad); self.app()
        # Truncate inside a stream, fixing the terminal offset so the decoder is reached.
        path.write_bytes(saved[:-1])
        head = bytearray((self.data / "VGAHEAD.WL6").read_bytes())
        head[-3:] = (len(saved) - 1).to_bytes(3, "little")
        (self.data / "VGAHEAD.WL6").write_bytes(head)
        self.assertIn("Truncated", self.app().stderr)

    def test_cli_errors(self):
        for args in ([], ["--data"], ["--bogus"], ["--frames", "-1"], ["--frames", "0"], ["--frames", "nonsense"]):
            with self.subTest(args=args):
                result = subprocess.run([str(self.__class__.binary), *args], capture_output=True)
                self.assertEqual(result.returncode, 1)

    def test_sdl_video_readback(self):
        sdk = Path(os.environ.get("SDL_SDK", ROOT.parent / "cart/vendor/SDL"))
        library = sdk / "build/libSDL3.a"
        if not library.is_file():
            self.skipTest("SDL_SDK must point to a built static SDL3 checkout for video tests")
        program = self.output / "video-tests"
        sources = [str(p) for p in sorted((ROOT / "src").rglob("*.c")) if p.name != "main.c"]
        subprocess.run(self.compiler + ["-I" + str(ROOT / "src"), "-I" + str(sdk / "include"),
                       str(ROOT / "tests/video_tests.c"), *sources, str(library), "-pthread", "-lm",
                       "-o", str(program)], check=True)
        env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_RENDER_DRIVER="software")
        subprocess.run([str(program), str(self.data / "readback.bmp")], env=env, check=True)

    def test_frontend_sdl_events(self):
        sdk = Path(os.environ.get("SDL_SDK", ROOT.parent / "cart/vendor/SDL"))
        library = sdk / "build/libSDL3.a"
        if not library.is_file():
            self.skipTest("SDL_SDK must point to a built static SDL3 checkout")
        fixtures(self.data,ui=True)
        program = self.output / "frontend-tests"
        sources = [str(p) for p in sorted((ROOT / "src").rglob("*.c")) if p.name != "main.c"]
        subprocess.run(self.compiler + ["-I" + str(ROOT / "src"),"-I" + str(sdk / "include"),
                       str(ROOT / "tests/frontend_sdl_tests.c"),*sources,str(library),"-pthread","-lm",
                       "-o",str(program)],check=True)
        env = dict(os.environ, SDL_VIDEODRIVER="dummy",SDL_RENDER_DRIVER="software")
        subprocess.run([str(program),str(self.data)],env=env,check=True)

    def test_finale_sdl_events(self):
        sdk = Path(os.environ.get("SDL_SDK", ROOT.parent / "cart/vendor/SDL"))
        library = sdk / "build/libSDL3.a"
        if not library.is_file():
            self.skipTest("SDL_SDK must point to a built static SDL3 checkout")
        from game_fixtures import finale_files
        from audio_fixtures import audio_files
        finale_files(self.data)
        audio_files(self.data)
        program = self.output / "finale-sdl-tests"
        sources = [str(p) for p in sorted((ROOT / "src").rglob("*.c")) if p.name != "main.c"]
        subprocess.run(self.compiler + ["-I" + str(ROOT / "src"), "-I" + str(sdk / "include"),
                       str(ROOT / "tests/finale_sdl_tests.c"), *sources, str(library), "-pthread", "-lm",
                       "-o", str(program)], check=True)
        env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_RENDER_DRIVER="software", SDL_AUDIODRIVER="dummy")
        subprocess.run([str(program), str(self.data)], env=env, check=True)
        if os.environ.get("WOLF3D_DATA"):
            subprocess.run([str(program), os.environ["WOLF3D_DATA"], "owned"], env=env, check=True)

    def test_boss_sdl_events(self):
        sdk = Path(os.environ.get("SDL_SDK", ROOT.parent / "cart/vendor/SDL"))
        library = sdk / "build/libSDL3.a"
        if not library.is_file():
            self.skipTest("SDL_SDK must point to a built static SDL3 checkout")
        from game_fixtures import boss_files
        boss_files(self.data)
        program = self.output / "boss-sdl-tests"
        sources = [str(p) for p in sorted((ROOT / "src").rglob("*.c")) if p.name != "main.c"]
        subprocess.run(self.compiler + ["-I" + str(ROOT / "src"), "-I" + str(sdk / "include"),
                       str(ROOT / "tests/boss_sdl_tests.c"), *sources, str(library), "-pthread", "-lm",
                       "-o", str(program)], check=True)
        env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_RENDER_DRIVER="software")
        subprocess.run([str(program), str(self.data)], env=env, check=True)
        if os.environ.get("WOLF3D_DATA"):
            subprocess.run([str(program), os.environ["WOLF3D_DATA"], "owned"], env=env, check=True)

    def test_campaign_sdl_events(self):
        sdk=Path(os.environ.get("SDL_SDK",ROOT.parent/"cart/vendor/SDL"))
        library=sdk/"build/libSDL3.a"
        if not library.is_file():
            self.skipTest("SDL_SDK must point to a built static SDL3 checkout")
        from game_fixtures import campaign_files
        campaign_files(self.data)
        program=self.output/"campaign-sdl-tests"
        sources=[str(p) for p in sorted((ROOT/"src").rglob("*.c")) if p.name!="main.c"]
        subprocess.run(self.compiler+["-I"+str(ROOT/"src"),"-I"+str(sdk/"include"),
                       str(ROOT/"tests/campaign_sdl_tests.c"),*sources,str(library),"-pthread","-lm",
                       "-o",str(program)],check=True)
        env=dict(os.environ,SDL_VIDEODRIVER="dummy",SDL_RENDER_DRIVER="software")
        subprocess.run([str(program),str(self.data)],env=env,check=True)
        path=self.data/"MAPHEAD.WL6"
        head=bytearray(path.read_bytes());struct.pack_into("<I",head,6,0xffffffff);path.write_bytes(head)
        subprocess.run([str(program),str(self.data),"--missing-next"],env=env,check=True)

    def test_game_sdl_events(self):
        sdk=Path(os.environ.get("SDL_SDK",ROOT.parent/"cart/vendor/SDL"))
        library=sdk/"build/libSDL3.a"
        if not library.is_file():self.skipTest("SDL_SDK must point to a built SDL3 checkout")
        from game_fixtures import game_files
        game_files(self.data)
        program=self.output/"game-sdl-tests"
        sources=[str(p) for p in sorted((ROOT/"src").rglob("*.c")) if p.name!="main.c"]
        subprocess.run(self.compiler+["-I"+str(ROOT/"src"),"-I"+str(sdk/"include"),
                       str(ROOT/"tests/game_sdl_tests.c"),*sources,str(library),"-pthread","-lm","-o",str(program)],check=True)
        env=dict(os.environ,SDL_VIDEODRIVER="dummy",SDL_RENDER_DRIVER="software")
        subprocess.run([str(program),str(self.data)],env=env,check=True)

    @unittest.skipUnless(os.environ.get("WOLF3D_DATA"), "Set WOLF3D_DATA for owned-data oracle validation")
    def test_owned_steam_data(self):
        directory = Path(os.environ["WOLF3D_DATA"])
        head = (directory / "VGAHEAD.WL6").read_bytes()
        archive = (directory / "VGAGRAPH.WL6").read_bytes()
        raw_dict = (directory / "VGADICT.WL6").read_bytes()
        tree = [struct.unpack_from("<HH", raw_dict, i * 4) for i in range(255)]
        start = int.from_bytes(head[87 * 3:88 * 3], "little")
        end = int.from_bytes(head[88 * 3:89 * 3], "little")
        self.assertEqual(struct.unpack_from("<I", archive, start)[0], 64000)
        planar = bytearray(); node = 254
        for byte in archive[start + 4:end]:
            for bit in range(8):
                code = tree[node][(byte >> bit) & 1]
                if code >= 256: node = code - 256
                else:
                    planar.append(code); node = 254
                    if len(planar) == 64000: break
            if len(planar) == 64000: break
        self.assertEqual(len(planar), 64000)
        pixels = bytearray(64000)
        for plane in range(4): pixels[plane::4] = planar[plane * 16000:(plane + 1) * 16000]
        self.compare_assets(directory, pixels)
        subprocess.run([str(self.output / "ui-tests"),str(directory),"owned"],check=True)
        dump=self.data/"owned-map-texture.bin"
        subprocess.run([str(self.output / "game-tests"),str(directory),"owned",str(dump)],check=True)
        subprocess.run([str(self.output / "campaign-tests"),str(directory),"owned"],check=True)
        subprocess.run([str(self.output / "combat-tests"),str(directory),"owned"],check=True)
        from game_fixtures import original_map_oracle
        self.assertEqual(dump.read_bytes(),original_map_oracle(directory))


if __name__ == "__main__":
    unittest.main()
