# Milestone 4: E1M1 walls and movement

New Game now genuinely loads Episode 1, Level 1 from the user's WL6 archives,
renders its original walls and doors, and allows walking, turning, running,
strafing and normal-door operation. Escape ends the walk session and returns
to the original menu. This is a verified wall-and-movement slice, not a complete
combat-capable Wolfenstein game or a completed campaign playthrough.

## Build and run

From the repository root:

```sh
./build/tools/cart build --profile cart-sdl
./build/tools/cart run --profile cart-sdl -- \
  --data "$HOME/.steam/steam/steamapps/common/Wolfenstein 3D/base"
```

Press a key to open the menu, then choose New Game. It is enabled only after
map, texture, spawn, status-art and initial ray-render validation succeeds.
There are no invented episode/difficulty selection screens: the action starts
E1M1 directly. Other episodes and difficulty behavior are not exposed.

| Key | Action in the level |
| --- | --- |
| Up / Down | Forward / backward |
| Left / Right | Turn |
| Alt + Left / Right | Strafe |
| Shift | Original run speed |
| Space | Operate an adjacent normal door |
| Escape | End this walk session and return to the menu |

Space operates the door in the cardinal direction the player faces. Approach
within one tile before using it. Key-locked doors stay locked; inventory is not
implemented. The elevator switch, secret pushwalls and level completion are
not activated. Escape is not a pause/save/resume feature; New Game restarts
E1M1. Existing menu controls, attract screens, scores and quit confirmation
continue working.

Diagnostics (same required `--data` argument):

```sh
./build/cart-sdl/linux/wolf3d-redux --data /absolute/path/to/WL6 --check-level
./build/cart-sdl/linux/wolf3d-redux --data /absolute/path/to/WL6 --level
```

`--check-level` validates and renders the initial world without SDL window
creation. `--level` starts that world directly. `--menu`, `--check-assets`,
`--check-ui`, `--frames N`, `--capture PATH.bmp` and `--help` remain available.
The optional initial capture is at full palette brightness before fade-in.
Keep captures private and in ignored build output.

## Source port, in dependency order

| Phase / files | Original implementation and changes |
| --- | --- |
| Input fixes: `ID_IN.c/.h`, `WL_MENU.c/.h` | Shortcut presses are queued and consumed once, in order, instead of repeatedly reading stale LastASCII. Escape, Enter and Space edges are consumed together with Escape taking priority. Existing SDL physical aliases, key-up and focus-loss clearing remain intact. Quick movement taps persist until a simulation tic, even if both make/break arrive between presentations. |
| A: `ID_FILE.c/.h`, `ID_MAP.c/.h`, `ID_CA.c` | Original `ID_CA.C:CAL_CarmackExpand`, `CA_RLEWexpand`, `CA_CacheMap`; explicit little-endian MAPHEAD offsets, 38-byte map headers and all three 64×64 word planes. Carmack near/far references preserve overlapping forward copies and escaped tag bytes. Both expansion layers have source/output bounds. Raw disk planes remain available unchanged. |
| B: `ID_PM.c/.h` | Original `PML_OpenPageFile` and `PM_GetPage`: VSWAP count/sprite/sound boundaries, 32-bit page offsets and 16-bit lengths. Pages load on demand into stable MM-owned caches. Walls retain their original 64-column × 64-row, column-major byte order. Sprite post streams decode with a separate opacity mask and signed pixel-pool bias; sprite rendering is not yet connected. |
| C: `WL_DEF.h`, `WL_ASM.c`, `WL_DRAW.c`, `WL_SCALE.c` | Original `WL_MAIN.C:BuildTables/CalcProjection/SetupWalls`, `WL_DR_A.ASM:AsmRefresh`, `WL_DRAW.C:FixedByFrac/CalcHeight/Hit*Wall/Hit*Door/ScalePost`, `WL_SCALE.C:BuildCompScale`. The quadrant-specific x/y stepping, vertical/horizontal entry tests, door midpoint tests, texture orientation, distance projection and wide-post height reuse are retained. Portable C emits original source-pixel scaling runs instead of executable x86 bytes. |
| D: `WL_GAME.c`, `WL_AGENT.c`, `WL_PLAY.c`, `WL_ACT1.c` | Original `SetupGameLevel`, info-plane player spawn, door/jamb marking, `PollKeyboardMove`, `ControlMovement`, `Thrust`, `TryMove`, `ClipMove` and door open/close state. Runtime tilemap remains x-major. Player coordinates remain 16.16; turning preserves anglefrac and the original square collision radius/axis-slide order. |
| E: `WL_MAIN.c/.h`, `WL_MENU.c/.h`, `main.c` | Readiness-gated New Game, game-state startup/teardown, input routing, world fades, Escape-to-menu and direct/headless level diagnostics. Missing or malformed level resources disable New Game while preserving the working title/menu UI. |

All work is under `src/`; `WOLFSRC/` remains unchanged. The C translation is
from the original local source and assembly, not a tutorial DDA, Wolf4SDL or
wolf3d-lib substitution. No third-party implementation was imported. Original
module/function ownership and derivation are identified in comments.

## Preserved calculations and explicit boundaries

- `FixedByFrac` takes a two's-complement coordinate and a **signed-magnitude
  fraction**, using the low 16 bits of the second operand and its high sign
  bit. It is not generic signed 16.16 multiplication. Unsigned/wide arithmetic
  makes wrap/truncation explicit without signed-overflow UB. Cardinal table
  values follow the original float-store table construction.
- Ray traversal follows the original two linked grid walks and quadrant cases,
  including tie ordering and 16-bit fractional partial steps. Closed/moving
  doors intersect at tile centers; opening position changes both leading-edge
  visibility and texture coordinates. Door jamb pages use `PMSpriteStart-8`.
- The framebuffer remains 320×200 indexed VGA data. The source's default
  size-15 view is 240×120 at (40,20), above the original status-bar artwork and
  beveled play border. Wall height is still in quarter-pixel units; the scaler
  directory's two-pixel, minimum-size and large-index quantization, centering and
  clipping are reproduced. Ceiling/floor use original E1M1 palette indices.
- Keyboard controls use BASEMOVE=35 / RUNMOVE=70, MOVESCALE=150,
  BACKMOVESCALE=100, ANGLESCALE=20 and MINDIST=0x5800. No invented acceleration,
  diagonal normalization, floating-point player coordinates or FPS scaling was
  introduced.
- Each elapsed 70 Hz tic runs the original **one-tic** movement path; rendering
  is independent. This makes identical held-input intervals deterministic at
  different presentation schedules. At most MAXTICS=10 run per update; excess
  long-pause time is discarded. Variable-size DOS frame batching can round
  movement differently; this implementation does not claim bit-identical
  traces for every original host frame rate.
- Normal doors move by 1024 position units per tic, become traversable when
  fully open, and try to close after the original OPENTICS=300. Player overlap
  prevents closure. Locked-door rejection is retained with no keys granted.
  Area connectivity, actor obstruction and positional audio hooks await their
  original dependencies.

## Validation and ownership

Map loading checks map index, table/header size, dimensions, plane offsets,
overlap and file bounds, both expanded-length fields, and compressed references.
The world requires a solid perimeter, one collision-free player start, valid
wall/area codes, bounded door counts and valid door jambs. A failed map decode
does not replace an already cached map.

VSWAP validates directory bounds/order, sparse entries, page overlap, sprite/
sound boundaries and exact 4096-byte wall pages. Sprite decoding checks column
ranges, command bounds, doubled row endpoints, signed pool offsets and pixel
bounds. Files are capped at 32 MiB; VSWAP at 4096 pages. All allocations use the
existing 64 MiB payload-budget memory manager. Owners are stable handles;
world exit frees map/page caches without invalidating graphics/menu caches.
Unconfigured data-directory access fails instead of falling back to filesystem
root. All archive access is read-only and supports the existing case resolver.

## Tests and results

- CART GCC/C11 builds with the configured strict warnings and no new warnings;
  the math library is added to the platform link configuration. No CART source
  changes were necessary. Repeat builds report up to date.
- Twenty-eight unittest cases pass, including the prior graphics/menu/input/
  memory/Git tests, with ASan/UBSan on the application and C tests. The reused
  SDL static archive itself is not instrumented.
- Synthetic maps, fonts, pictures, VSWAP pages and post streams are generated
  temporarily by `tests/game_fixtures.py`. `game_tests.c` covers escaped tags,
  near/far overlap, truncation/overruns, malformed map/plane data and spawn/wall
  semantics, page boundaries/layout, signed sprite offsets, fixed-point extrema,
  cardinal/door ray hits, scaler samples, collision/sliding, running/turning/
  strafing, door timers/occupancy/locks and coarse/fine 70 Hz scheduling.
- `game_sdl_tests.c` drives New Game through actual SDL key events, then tests
  motion, quick taps, focus loss and simultaneous Escape/Enter returning safely
  to the menu. Invalid-level tests keep the original menu usable with New Game
  disabled. No synthetic success/gameplay screen replaces a failed load.
- Original Steam data loads in place. The decoded spawn is (29,57), angle 0
  (east). Integration walked through the original normal door at x=32 to
  (34,57); SDL captures at spawn and beyond the cell were visually inspected.
- An independent Python decoder matches all three real map planes and a wall
  page byte for byte. Every non-sparse Steam sprite page also passes bounded
  post decoding. The previous title/framebuffer/palette oracle still passes.
- All 23 active C modules pass MinGW C11 compile-only checks with warnings as
  errors. Native Windows linking/execution remains unverified.
- CART's SDL3 (including raylib/Android fixtures), output-directory, Android
  profile/icon and Linux macro regressions pass. Existing icon-copy warnings
  are unchanged. CART's pre-existing working-tree modifications were preserved.
- Whitespace, unchanged-original-source, Git safeguards and commercial-data/
  capture ignore checks pass. No data, fixture binaries or captures are added
  to Git. No commits, pushes, SDK downloads or Ironclaw changes were performed.

See [BUILD.md](BUILD.md) for reproducible normal/sanitizer commands. New tests
are in `game_tests.c`, `game_sdl_tests.c`, `game_fixtures.py` and the expanded
`test_engine.py`; existing frontend/video/Git tests remain active.

## Remaining original-engine work

Enemies and static objects are neither rendered nor collided with yet. Sprite
pages decode, but `DrawScaleds`/`ScaleShape`, actor state, combat, weapons,
pickups, keys and damage still need porting. HUD numerical fields are not
updated; only the original status artwork/play border are shown. No secret
pushwalls, elevator/exit progression, episode/difficulty flow, other playable
maps, saving/loading, demos or audio are claimed. Escape discards this level
session. Earlier UI animation/RNG omissions remain as documented in milestone 3.

DOS-reference captures and recorded traces are still needed for a formal
pixel/gameplay parity claim, particularly old Borland floating-point table
rounding and variable-tic batches. Current evidence establishes the direct
source translation, original data compatibility, original wall presentation,
wall/door collision and deterministic movement slice. Next port the original
static/sprite renderer and actor dependencies, then combat and progression.
