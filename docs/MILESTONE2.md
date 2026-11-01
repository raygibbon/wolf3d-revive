# Milestone 2: original WL6 title graphics

The original title screen now displays from the user's Steam files in an SDL3
window. This is the title-only dependency slice of the original engine, not a
replacement engine or a complete gameplay port. No Wolf4SDL/wolf3d-lib code
was imported. `WOLFSRC/` is unchanged.

## Original path and ported modules

`WOLFSRC/WL_MAIN.C:DemoLoop` calls `CA_CacheScreen(TITLEPIC)` then updates the
display. The new entry point follows that path after starting memory/cache
services and selecting the original palette. The original title index is 87.

| New file | Original owner and changes |
| --- | --- |
| `src/engine/ID_MM.c`, `.h` | `WOLFSRC/ID_MM.C/H`: heap-backed stable owner handles replace paragraphs and far pointers. `MM_GetPtr`, `MM_FreePtr`, `MM_SetPurge`, `MM_SetLock`, `MM_SortMem`, startup/shutdown retain ownership and purge/lock roles. Allocations are zero-initialized, bounded by a 64 MiB payload budget, and checked. Purging nulls the registered owner; no relocation is necessary. |
| `src/engine/ID_CA.c`, `.h` | `WOLFSRC/ID_CA.C/H`: portable file access, explicit little-endian dictionary/24-bit offsets/chunk sizes, bounded `CAL_HuffExpand`, `CAL_SetupGrFile`, `CA_CacheGrChunk`, `CA_CacheScreen` and picture table. Huffman root 254, child indices 256..510 and LSB-first consumption match the original assembly; the DOS pointer-optimization step is unnecessary. |
| `src/engine/ID_VL.c`, `.h` | `WOLFSRC/ID_VL.C/H`: `VL_MemToScreen` translates four consecutive VGA planes into 320×200 indexed pixels. `VL_SetPalette` retains 0..63 DAC values. RGBA conversion expands six-bit channels by bit replication and supplies opaque alpha. |
| `src/engine/GFXV_WL6.h` | Original generated resource indices and constants copied from `WOLFSRC/GFXV_WL6.H`, with any DOS EOF marker removed. |
| `src/engine/gamepal.inc` | All 256 original colors from the tracked `GAMEPAL.OBJ` segment-1, offset-0 LEDATA payload (768 bytes). No Steam graphics embedded. |
| `src/platform/sdl_video.c`, `.h` | SDL-only window/renderer/streaming texture, indexed-to-RGBA upload, nearest sampling and centered 4:3 logical presentation. A 320×200 texture fills a 320×240 logical viewport. Resize, redraw, palette updates, Escape and close events are supported. |

Portable functions return success/failure and carry explicit buffer sizes where
the DOS versions assumed valid memory. Errors propagate to the entry point;
partially loaded chunks are freed and startup failures close the archive.
Owner handles must stay at a stable address until freed, matching the original
memory manager's registered-pointer contract. Shut down CA before MM.

Cache hits reuse the decoded buffer and clear its purge level, as in the
original. Title decode now goes through that cache and then `VL_MemToScreen`,
rather than writing Huffman output directly to hardware VGA planes. This keeps
the original decoded byte order while separating hardware presentation.
The graphics picture table and title remain resident until shutdown or purge.
This slice does not implement cache levels or EMS/XMS page management.

## Data and validation boundaries

`--data` accepts an explicit directory, including paths with spaces. The loader
recognizes Steam's uppercase names and all-lowercase copies. It never changes
the current directory, copies an archive or writes configuration/saves there.

The supported graphics profile is the checked-in 149-chunk WL6 layout:
450-byte VGAHEAD, 255-node dictionary (1020 bytes, or 1024 with the unused final
entry), 132 picture dimensions and a 320×200 title. Missing/malformed graphics,
invalid dictionary references/cycles, sparse/missing title data, nonmonotonic
or out-of-file offsets, truncated input and oversized expansions fail with a
diagnostic and exit status 1. Input files are limited to 32 MiB and individual
compressed/expanded chunks to 4 MiB. The terminal header offset must match the
archive size. Files are read on demand rather than loading the entire archive.

The full expected WL6 file set is checked for presence, nonempty size and
readability: VSWAP, VGAHEAD/VGADICT/VGAGRAPH, MAPHEAD/GAMEMAPS and
AUDIOHED/AUDIOT. Only graphics formats are parsed in this milestone. Presence
of maps/audio does not establish their semantic validity or full edition
compatibility. Arbitrary case mixtures and different WL6 resource layouts are
not supported yet; unsupported layouts fail explicitly instead of guessing.

## Other changed files

- `src/main.c`: replaces the milestone-1 probe with title startup/lifecycle;
  `--data`, `--check-assets`, finite `--frames`, optional private `--capture`.
- `cart.toml`: keeps existing targets/SDL profiles and removes the probe-only
  define; recursive CART discovery builds all five active C files.
- `src/w3r_types.h`: baseline comment updated; fixed-width definitions retained.
- `.gitignore`: additionally ignores BMP/PPM captures of game artwork.
- `tests/engine_tests.c`, `tests/video_tests.c`, `tests/test_engine.py`:
  synthetic fixtures, C units, loader/CLI failures, SDL readback and optional
  owned-data comparison. Existing Git-protection tests are unchanged.
- README and BUILD/PORTING/LICENSING guides now describe the implemented title
  path, limits, provenance and reproduction commands.

CART required no new changes. Its pre-existing milestone-1 filesystem fix,
test and testing-document modifications were preserved without alteration.
No compiler install, dependency download, commit or push was performed.
Origin and upstream protections remain configured, including the existing
invalid `upstream` push URL `DISABLED` and `.githooks` pre-push guard.

## Verification results

- GCC/C11 CART build with all configured strict warnings: passed without new
  warnings; repeat builds report up to date.
- Actual Steam WL6 title: loaded in place, displayed in an SDL3 window, and a
  local render capture visually inspected at 960×720. Captures stay in ignored
  build output. No commercial data or rendered artwork was added to Git.
- Sixteen unittest cases passed with `WOLF3D_DATA` enabled, also passing with
  ASan/UBSan for the application and C test binaries. Includes bounded Huffman
  malformed-stream corpus, ownership/purge/lock/shutdown, invalid files/offsets,
  dimensions, expansion limits, cache hits and repeated startup/shutdown.
- Independent Python decoder matched every indexed title byte and every RGBA
  byte from the C path against the real Steam archive. Palette expectations
  came independently from the original tracked OMF resource. Synthetic
  archive round trips compare all pixels as well.
- SDL dummy/software readback verified nearest color boundaries, palette-only
  updates, Escape handling, wide 1000×600 and tall 600×1000 resize viewports
  (800×600 at x=100, and 600×450 at y=275 respectively).
- MinGW C11 compile-only checks with warnings as errors passed for all five
  active C files. Windows linking/runtime remain untested.
- Existing CART SDL3 regressions (including raylib/Android fixtures) and its
  output-directory regression passed. No CART source changes were necessary.
- Git whitespace checks and unchanged `WOLFSRC/` checks passed.

## Remaining work

The static title remains until Escape/close. Original fades, the 15-second
attract sequence, menus, fonts, PCM/OPL, simulation timing and gameplay are not
implemented. There is no fullscreen toggle or SDL_GPU path. Next extend
original VL/VH/CA drawing to menus and fonts, followed by bounded map/VSWAP
loading and deterministic fixed-point/RNG helpers for the gameplay port.
Game movement, doors, enemies and audio stay out of this graphics milestone.
