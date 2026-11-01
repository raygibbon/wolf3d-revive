# Source inventory and staged modernisation

This inventory was recorded during milestone 1, 2026-11-01. The original
`WOLFSRC/` tree remains untouched. Milestone 2 now ports the original title
graphics path with memory, cache/decompression and SDL video support; see
[milestone 2 results](MILESTONE2.md). Milestone 3 adds the original menus,
fonts, input and supported attract flow; see [current results](MILESTONE3.md).
Milestone 4 now implements E1M1 maps/VSWAP, wall tracing, movement and normal
doors; see [current gameplay-slice results](MILESTONE4.md). Remaining items
below are the incremental engine plan, not claims of completed gameplay.

## Source-file inventory

Reviewed the following headers, relevant implementation sections, assembly
exports and the Borland project records. This is a porting inventory, not a
complete behavior audit of every conditional edition.

| Files inspected (under WOLFSRC/) | Portability issue and intended change |
| --- | --- |
| `ID_HEAD.H`, `ID_HEADS.H`, `WL_DEF.H`, `VERSION.H`, `WOLFVER.H` | Borland uppercase system headers, `SYS\STAT.H`, `ALLOC.H`, `DOS.H`, `MEM.H`, `IO.H`, `VALUES.H`; duplicate types; 16-bit `unsigned`, 32-bit DOS `long`; `far`, `huge`, `_seg`, pragmas. Introduce explicit-width types and ordinary pointers by subsystem, not blanket keyword macros. |
| `ID_MM.C/H`, `ID_PM.C/H` | Paragraph allocation, segment arithmetic, near/far heaps, EMS interrupt 67h, DOS int 21h, XMS int 2Fh and driver calls. Replace with owned heap allocations and bounded VSWAP page buffers while retaining `MM_*` / `PM_*` call sites where practical. |
| `ID_CA.C/H` | DOS file handles and far reads, native structs, 24-bit VGA offsets, Huffman, Carmack and RLEW decoding. Keep decompressor names; use bounded byte readers, explicit little-endian values, checked sizes and errors for truncated streams. |
| `ID_VL.C/H`, `ID_VL_A.ASM`, `ID_VH.C/H`, `ID_VH_A.ASM` | BIOS video calls, VGA ports, A000 framebuffer segments, four planes, latches and page offsets. Translate drawing operations to a 320×200 byte-per-pixel buffer and preserve original palette/fade precision. |
| `WL_DRAW.C`, `WL_DR_A.ASM`, `WL_SCALE.C`, `WL_ASM.ASM` | Inline assembly `FixedByFrac`, `AsmRefresh` ray traversal, segment-addressed textures, generated x86 scaling code/far calls, CPU checks. Translate incrementally to C and compare ray hits, wall heights and pixels before optimization. |
| `ID_IN.C/H` | IRQ1 scan codes/ports 60h, 61h, PIC acknowledgement, int 33h mouse, joystick port 201h. Map SDL physical keys to original set-1 codes, preserve down/up state, relative mouse motion/buttons, clear held state on focus loss. |
| `ID_SD.C/H`, `ID_SD_A.ASM` | PIT/IRQ0 timing, PC speaker ports, AdLib registers, Sound Blaster DMA/interrupts and Sound Source. Separate 70 Hz gameplay from audio service clocks; replace hardware output with PCM and an interchangeable OPL interface. The historical `SDL_*` function prefix here predates Simple DirectMedia Layer. |
| `ID_US_1.C`, `ID_US_A.ASM` | User/UI helpers, native config/save structures, table-based RNG with DOS time seeding. Port the original 256-byte table and index sequence to C, with an explicit deterministic seed for tests. |
| `WL_MAIN.C` | DOS startup/disk queries, config, `SaveTheGame` / `LoadTheGame`, linked palette/signon. Separate startup and path selection; serialize integers explicitly and encode actor/state references as stable IDs. Native pointer dumps are not portable saves. |
| `WL_PLAY.C`, `WL_GAME.C` | `TimeCount` waits, four-tic demos, input sampling, level setup and demo pointer casts. Preserve frame-level tic batching and random call ordering; do not assume one update per rendered frame or replace each batch with independent one-tic updates. |
| `WL_AGENT.C`, `WL_ACT1.C`, `WL_ACT2.C`, `WL_STATE.C` | Player/collision, doors/pushwalls, enemies and state tables. Keep behavior and names; audit integer truncation, signed shifts/overflow, and pointer/sentinel uses in `actorat` before changing types. |
| `WL_MENU.C/H`, `WL_INTER.C`, `WL_TEXT.C` | UI, waits, artwork/text resources and edition branches; retain original flow on the portable drawing/input interfaces. |
| `GFXV_WL6.H`, `AUDIOWL6.H`, `MAPSWL6.H`, `WOLF3D.PRJ`, `README/RELEASE.TXT` | Generated resource indices and historical build dependency records. Medium memory model/TASM, linked object resources, original Borland C++ 3.0 toolchain; generated constants must match the selected data edition. |

`C0.ASM`, `H_LDIV.ASM` and historical hacks/tools (`DETECT.C`, `MUNGE.C`,
`CONTIGSC.C`, `WOLFHACK.C`, `OLDSCALE.C`, `WHACK_A.ASM`, `JABHACK.ASM`) are
not candidates for indiscriminate inclusion in a modern executable. Audit
their actual required symbols first. Original `OBJ/GAMEPAL.OBJ` and
`OBJ/SIGNON.OBJ` are 16-bit linked resources, not portable SDL assets. Recover
and attribute palette constants separately; do not silently link DOS objects.

## Compatibility constraints

`fixed` is originally a 32-bit DOS `long`. `FixedByFrac` multiplies a two's
complement 16.16 coordinate by a signed-magnitude fraction; ordinary signed
16.16 multiplication is not a drop-in replacement. Test signs, zero, cardinal
angles, extrema and truncation against assembly semantics with wide unsigned
intermediates before replacing it. LP64 Linux and LLP64 Windows disagree on
`long`; no disk structure may rely on either native ABI.

`TickBase` is 70 in `ID_SD.H`; `MAXTICS` is 10 and `DEMOTICS` is 4 in
`ID_HEADS.H`. `CalcTics` clamps long frames and subtracts excess elapsed time.
Preserve that policy, pause resets and demo pacing. Presentation can be
independent, but changing simulation batching can change movement rounding.

Edition macros include `SPEAR`, `SPEARDEMO`, `UPLOAD`, `GOODTIMES`, `JAPAN`,
`JAPDEMO`, `SPANISH`, `ARTSEXTERN`, `DEMOSEXTERN`, `CARMACIZED`, linked-header
switches and debug/profile switches. The checked-in defaults include
`GOODTIMES`, external art/demos and Carmack compression; they are not proof of
Steam WL6 compatibility. Establish one explicit WL6 profile against the user's
actual files before broadening edition support. Case-sensitive include names
and DOS EOF bytes also need targeted treatment.

Data-directory support belongs at one file-open boundary, taking
`--data /absolute/path` without changing global working directory. The initial
WL6 set is VSWAP, VGAHEAD/VGADICT/VGAGRAPH, MAPHEAD/GAMEMAPS and
AUDIOHED/AUDIOT. Original code also uses variant names such as MAPTEMP and
extension selection. Validate the whole set and resource indices; recognize
case variants on Linux, reject missing/mixed editions, and never infer success
from file existence alone. Original DOS-save import and portable-save output
must be explicit, separate format decisions.

## Platform boundary design

Keep original `ID_*`/`WL_*` ownership and names as files are ported. New SDL
implementation files will live separately under `src/platform/`; do not add
SDL headers to actor or raycasting logic. Move a module into the active build
only once its dependency slice compiles and has meaningful checks. Preserve
history with moves rather than replacing the tree with a reference project.

The video boundary accepts indexed pixels, stride and a 256-entry palette.
The SDL_Renderer backend owns window/renderer/streaming RGB texture, palette
expansion, nearest filtering, centered 4:3 viewport/letterboxing, resize and
fullscreen. This deliberately corrects VGA pixel aspect: stretching 320×200
into 4:3 differs from square-pixel 8:5. Keep palette-only updates visible.
The same boundary can later support SDL_GPU without changing game logic.

Input exports set-1 key events/states and relative mouse deltas/buttons. Timing
supplies monotonic counters and sleep; tests inject virtual time. Audio owns
SDL PCM buffering; an OPL adapter accepts timestamped register writes and
generates samples. Silence must still advance sequencing. No audio clocks
should derive from presentation frequency. No platform API is implemented
beyond the original UI slice in milestone 3. Monotonic timing and keyboard
input are now implemented; audio and gameplay interfaces remain future work.

## Reference findings

Reviewed [wolf3d-lib architecture](https://github.com/bobapplemac/wolf3d-lib/blob/main/docs/architecture.md),
[porting guide](https://github.com/bobapplemac/wolf3d-lib/blob/main/docs/porting-guide.md),
`src/WG_FIXED.c`, source/test layout and licensing inventory. Useful patterns
are bounded little-endian resource parsing, indexed framebuffer ownership,
host callbacks, a headless test host and preserved live/demo tic batching.
Its C99 conversion is a reference for our C11 work, not a replacement core.

Reviewed [wolf3d-portable's SDL3 host](https://github.com/bobapplemac/wolf3d-portable/blob/main/platforms/sdl3/WG_SDL3.c),
README, layout and third-party notices. It maps SDL scancodes to set-1 keys,
uses a streaming RGBA texture with nearest sampling and logical presentation,
and supplies PCM through an SDL audio stream. These demonstrate the proposed
separation; no implementation was copied. For audio, the library guide keeps
700 Hz IMF and 140 Hz effects independent of the game clock and negotiates
the application PCM rate. Follow that separation when restoring `ID_SD`.

## Small reviewable milestones

1. **Prepared here:** Git safeguards, inventory, C11 type baseline, CART/SDL3
   dependency probe. Original game is not yet compilable.
2. **Portable-core slices:** milestone 2 ports the 149-chunk WL6 title path,
   memory manager, bounded graphics decompression and indexed video. Next port
   `US_RndT` and fixed-point helpers with oracle vectors, then CA map-plane
   and PM VSWAP decoding against synthetic malformed streams and owned data.
   Data path selection is implemented for the title. Progress toward a playable core
   subsystem by subsystem; avoid a mass header rewrite.
3. **Video:** title presentation, original palette, resize, nearest scaling and
   buffer tests are implemented. Milestone 3 adds original UI primitives, fonts,
   ordinary pictures, menus and fades. Fullscreen remains later work.
4. **Input/gameplay:** integrate IN/timing and restore player, doors, weapons,
   enemies and demos; compare deterministic traces and original tic batching.
5. **Audio:** SDL PCM, original digitized samples and sequences, then reviewed
   OPL integration with independent rational sample clocks.
6. **Compatibility:** original WL6 map start/playthrough, secrets, doors,
   enemies, save/load and demo comparisons using external owned data.
7. **Enhancements only after parity:** optional widescreen/high refresh,
   gamepad, settings and SDL_GPU. Keep original gameplay as the default.
