# Milestone 11: original DOS sign-on

**Branch:** `feature/11-dos-signon-screen`

Normal startup can display the authentic DOS sign-on image with its id
Software, Wolfenstein 3D and Activision logos, red background and original
memory/hardware panel. No replacement artwork or commercial asset is added
to the repository. The optional image must come from an owned installation.

## Supplying the artwork

`SIGNON.RAW` belongs in the directory passed to `--data`. It is exactly
**64,000 bytes**: 320x200 row-major palette indices, top row first, with no
header, padding, compression or appended palette. It uses the game's original
256-colour VGA `gamepal` (768 six-bit channel values), already preserved by the
engine. Every byte is a pixel index; this is not planar VGA memory, RGB data,
a screenshot, a WL6 graphics chunk or a setup-program screen.

The supported automatic extraction uses the owned Steam/Activision WL6 DOS
`wolf3d.exe` (108,779 bytes). Run from the repository root, with Python 3.9+:

```sh
python3 scripts/extract_signon.py \
  "/path/to/owned/Wolfenstein 3D/base/wolf3d.exe" \
  "/path/to/owned/Wolfenstein 3D/base/SIGNON.RAW"
```

You can also pass the Steam install directory (or its `base` directory)
instead of the executable. To recover the image linked by the historical
Wolf source release:

```sh
python3 scripts/extract_signon.py WOLFSRC /private/path/SIGNON.RAW
# Also accepts WOLFSRC/OBJ/SIGNON.OBJ or the repository root directly.
```

The extractor discovers recognised files in those directories, preferring a
supported Steam executable when available. This is an explicit local setup
step; the game itself still reads `SIGNON.RAW` and does not run Python at launch.
The source **C files alone do not contain the artwork**: the historical
`OBJ/SIGNON.OBJ` asset is required. That source-release image contains a different
publisher logo; use the Steam version for the Activision logo. The original
pixels are preserved for either version.

The source object's SHA-256 is
`dd66c2a2c7744988ad7700c826674d6747859bde9baf817cba7399f99937b376`;
its recovered pixels hash to
`4fd711cdde7929645a46dd572ab61feacc8d3b79844250835b0ce87312a2c999`.
The tool reconstructs its OMF LEDATA records, checking record lengths,
checksums when present, segment bounds, overlapping data and complete coverage
of all 64,000 pixels. It never modifies the object or embeds its data in code.

Alternatively extract into an existing private staging directory, then copy
`SIGNON.RAW` beside the WL6 archives. The output must be new: existing files
are never overwritten, and the original executable is never modified or run.
Output inside the source tree is restricted to ignored `build/`, `data/` or
`game-data/` directories. `SIGNON.RAW` is also ignored by Git. Do not force-add
artwork, commercial executables or captures.

The tool verifies all three SHA-256 identities before writing:

| Item | SHA-256 |
| --- | --- |
| Owned compressed DOS EXE | `48d9594649f330956e86ea1892bab76b2d83cfd4038e262ff8f3383ba945ec09` |
| Decompressed MZ load image, 254,014 bytes | `d397db84048d18bbb90df0ba0db88bdf5982ce3cac868bec13f117e61fb74ff0` |
| Original 320x200 pixels | `d3c2f25b82b24f3513be29ee12bd5dc71ab1b0ac737d5111cbd36150927fa6f7` |

For that exact executable, the pixels begin at load-image offset `0x23760`.
The bounded decoder handles the executable's LZEXE stream in Python, without
external programs. It checks input bounds, output size, backwards references
and checksums. Unknown or modified executable revisions are rejected rather
than searched heuristically. This decoder only recovers the load image; it
does not reconstruct an executable or relocation table.

The LZEXE format was checked against [Fabrice Bellard's original
reference](https://bellard.org/lzexe/) and independently decoded using
[UNLZEXE](https://github.com/mywave82/unlzexe). The owned executable has a
modified startup stub that the unmodified UNLZEXE signature check rejects;
a private reference copy accepted its verified LZ91 header. Both decoders
produced identical load-image bytes. No third-party decompressor source is
bundled. The owned load image also contains the exact original VGA palette
used by Revive.

### Other revisions: supported manual import

Automatic profiles cover the checksum-listed Steam executable and source object. Other releases
can supply the same RAW format if their original linked image can be identified
reliably. Publisher logos can differ; do not substitute newly drawn artwork.

For an **uncompressed DOS MZ executable with its matching linker MAP**:

1. Locate the `_signon` (or `introscn`) symbol and its 64,000-byte segment in
   that exact build's MAP. Do not apply addresses from another revision.
2. Read the EXE header size: the little-endian word at file offset `0x08`,
   multiplied by 16. The map's `segment:offset` address is relative to the MZ
   load image. File position is `header_size + segment*16 + offset`.
3. Copy exactly 64,000 bytes at that position into a new `SIGNON.RAW`. Check
   that the entire range lies within the executable's declared MZ image,
   excluding any appended debug data. The source build must use the same VGA
   palette. Do not deinterleave or flip the pixels.
4. If the executable is packed, first unpack it with a tool supporting that
   packer/revision. Re-read the resulting MZ header size and use the matching
   original load-image map. A screen capture cannot reliably recover palette
   indices or the untouched linked image.

Without a matching map, object module or verified extraction profile, this
manual route cannot identify an arbitrary executable safely. Leave the optional
artwork absent rather than guessing offsets. The game still starts normally.

## Historical implementation and compatibility

`WOLFSRC/ID_HEADS.H` aliases `introscn` to the far symbol `signon`.
`WOLF3D.PRJ` links `OBJ/SIGNON.OBJ`; the historical linker MAP identifies
`SIGNON.BIN` in the 64,000-byte `SignonSeg` FAR_DATA segment. The image was
converted to an OMF object and linked into the executable, separately from the
WL6 graphics archives. `SignonScreen()` sets `gamepal`, calls `VL_MungePic()`
on the row-major image to convert it for DOS VGA planes, copies it to screen,
then releases its original storage. Revive copies the original row-major bytes
directly into the existing indexed framebuffer. `WOLFSRC/` is unchanged.

DOS startup order was sign-on → subsystem initialization → `IntroScreen()`
memory/card fills → `FinishSignon()` acknowledgement → attract startup,
including the optional PG13 rating and title sequence. Revive loads the
resources needed for its original font first, then presents:

**Sign-on → keypress → optional PG13 rating → title → existing attract loop.**

- `FinishSignon()` clears `(0,189,300,11)` with the original top-left background
  index, centres font 0 at y=190 and draws **Press a key** in palette index 14
  (yellow). On acknowledgement it draws **Working...** in index 10 (green).
  The rest of the linked image, including the publisher logos, is untouched.
- The original unfilled Main/EMS/XMS bars and Mouse/Joystick/AdLib/Sound
  Blaster/Sound Source boxes are preserved. They are **not measurements** or
  failed detection results. Revive does not claim to detect DOS conventional,
  EMS/XMS memory or ISA cards, and does not fill them with invented numbers.
  This deliberately retains the untouched artwork instead of simulating a
  DOS hardware inventory. Modern controls and audio remain available normally.
- Sign-on waits indefinitely for fresh acknowledgement input. Input already
  pending when it opens is cleared. The dismissal key, held-key repeats,
  keyboard text and mouse acknowledgement do not leak to the next page.
- The sign-on uses the existing palette fade-in. Its dismissal fades to black,
  followed by the existing rating/title fade and a fresh page deadline. Music
  is silent during sign-on and the existing intro track resumes afterwards.
- `--menu` and `--level` bypass this historical startup. Sign-on is never
  inserted into in-game menu transitions or repeated by the attract loop.
- Missing optional artwork is skipped. Wrong lengths/read errors produce a
  clear diagnostic; invalid input never partially replaces the framebuffer.
  The application reports malformed supplied artwork as a startup error.

## Presentation

The existing SDL3 presentation path treats `UI_SIGNON` as DOS interface content,
never an enhanced world. The framebuffer stays 320x200; the logical canvas is
320x240, correcting DOS VGA pixel aspect to 4:3. Fit/Integer scaling uses the
existing menu preference, with black unused output and no SDL window resize.

Fit examples, allowing SDL's final pixel rounding:

| Window / output | Centred artwork |
| --- | --- |
| 1920x1200 | 1600x1200, 160-pixel side borders |
| 1920x1080 | 1440x1080, 240-pixel side borders |
| 1280x800 | approximately 1067x800 |
| 3440x1440 | 1920x1440 |
| 3840x1080 | 1440x1080 |

No gameplay renderer, fixed 70 Hz simulation, RNG, demo decoding, HUD or
Milestone 9 performance code was changed.

## Validation

All checks used strict C11 with `-Wall -Wextra -Wpedantic -Wconversion
-Wshadow -Werror` (GCC 14.2). Both the application and SDL harness compiled.

| Check | Result |
| --- | --- |
| Full existing suite plus new sign-on tests, owned WL6 available | 136 passed, no skips (547.585 s) |
| Full ASan/UBSan suite, application and test binaries instrumented | 136 passed across 13 modules, no skips or findings (518.173 s) |
| Focused extraction/sign-on suite | 13 passed normally and under sanitizers |
| Original DOS demos, classic and enhanced | All four traces / 5386 commands unchanged |
| DOS menu pixel comparisons, rendering hashes, controls, save/load | Passed |
| Strict Linux / MinGW and CART SDL builds | Passed |
| Native Linux and Windows executable under Wine | Passed within the display limits below |

The normal suite used:

```sh
WOLF3D_BINARY="$PWD/build/checks/m11/wolf3d-revive" \
WOLF3D_DATA="/path/to/owned/WL6" CC='gcc -O1' \
python3 -m unittest discover -s tests -v
```

Sanitizer modules ran in four independent subprocesses using `SANITIZE=1`,
`CC='gcc -O1'`, `UBSAN_OPTIONS=halt_on_error=1` and an ASan/UBSan application
binary. Every module exit status and log was checked. Full suite logs, private
artwork and native/Wine captures are under ignored `build/checks/m11/`.

The later Python-only source-object/directory extraction update passed all
16 focused extraction/sign-on tests normally (11.609 s) and with the sanitizer
harness/application (22.719 s), without skips or findings. These include source
object recovery, directory detection and malformed OMF records. The full-suite
counts above describe the completed milestone validation before that follow-up;
the game engine and presentation code did not change in the follow-up.

Focused checks cover extraction identities and output protection, invented
LZEXE token streams, malformed references and bounds, valid/missing/truncated/
oversized RAW images, original font composition, palette accuracy, silent
sign-on, prompt interaction, held-key isolation, rating/title ordering,
`--menu`/`--level` bypass, all listed aspect geometries, black output borders,
zero window resize events and unchanged timing/RNG.

Native Linux SDL3 X11/OpenGL validation on a physical 1920x1200 panel passed:
every output pixel matched the owned indexed image at 5x6 texels, with exactly
160 black pixels on each side. The captured original-art layout was visually
inspected. Dismissal preserved window dimensions and emitted no resize events.
Strict Linux and MinGW builds, CART's SDL build and the Windows test executable
under Wine also passed. Wine used SDL 3.4.18 dummy/software rendering; Linux
used the local SDL 3.5.0 development SDK.

Physical ultrawide, high-DPI/mixed-DPI, multi-monitor and native Windows display
testing were unavailable. Exact widescreen and ultrawide geometries are covered
by SDL dummy/software tests; Wine does not establish physical Windows display
behaviour. Extraction was verified against the owned Steam/Activision executable and
the historical source object listed above; other publisher/release executables
require manual import or a future separately verified profile. Commercial art, diagnostics and captures
remain in private temporary or ignored build directories.

## Milestone 12 capability overlays

The later fidelity branch fills the original Mouse, AdLib-only and Sound
Blaster boxes when the front end reports an SDL mouse, loaded synthesis or
loaded digitized playback respectively. Positions/colors follow `IntroScreen`.
These indicate supported runtime equivalents, not detection of physical ISA
hardware. Memory/EMS/XMS, joystick and Sound Source remain unfilled. The
untouched-artwork validation above describes Milestone 11; current sign-on
regressions include these explicit, bounded overlays.
