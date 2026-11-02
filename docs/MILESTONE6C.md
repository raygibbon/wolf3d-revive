# Milestone 6C — save/load and intermissions

Six named slots, portable native saves, transactional restoration, menu pause/
resume and animated original intermissions are implemented. Baseline: clean tree,
CART build and all 51 existing regressions passed with owned data (235 seconds). Original
WL_MAIN.C SaveTheGame/LoadTheGame, WL_MENU.C save/load UI and WL_INTER.C
LevelCompleted/BJ_Breathe were reviewed alongside the active code and the
6/6A/6B/build reports. The requested six slots take precedence over the
archived GOODTIMES menu's ten slots.

## State audit and checklist

- [x] All gamestate fields: inventory, attack/weapon animation, health/lives,
  keys, score/thresholds, episode/floor/difficulty, counts, level clock and
  boss kill-camera coordinates.
- [x] Eight completed-floor statistics records.
- [x] Player and every allocated actor: class, coordinates, movement, health,
  direction, flags, alert/reaction fields, state ID and tic counter. Projectiles
  and dying bosses use the same explicit fields and state registry.
- [x] Actor pool high-water mark, free holes, original linked ordering and
  exact tile reservations. Occupancy cannot be rebuilt solely from positions:
  overlapping patrol destinations and actor slot reuse affect continuation.
- [x] Static objects/collected items and blocking cells; runtime wall/floor
  areas, pushable markers, moving wall phase, position, coordinates/direction.
- [x] Doors including lock/type, position, action and hold timer.
- [x] RNG table index, face-count RNG timer, attack latch, chaingun face latch,
  movement angle fraction, noise flag and last thrust.
- [x] Logical digitised mode, preserving original 5/140-tic boss state delays
  independently of output hardware and volume.
- [x] Immutable original map restored from resource-identified game archives.
- [x] Exclude/rebuild presentation palette, ray buffers, input queues, wall
  heights, PCM/OPL state and device pointers. Actor view/visibility fields
  remain saved because the next attack can consume the previous frame's view.

Saving is limited to living normal gameplay, including attacks, active
projectiles and boss death states before a victory camera begins. Death,
intermissions and victory cinematics are not save points. This removes any
need to serialize the cinematic framebuffer or live camera pointers.

## Using saves

Escape during living gameplay pauses the simulation and opens the original
control panel. Save Game opens six slots; arrows select, Enter edits/selects,
Backspace deletes and Escape cancels. Descriptions use the existing ASCII
keyboard/font, at most 31 characters and 121 pixels. A blank edit receives an
E#/Floor description. Replacing any existing file, including corrupt or
unsupported versions, requires Y; N/Escape preserves it. Load Game also works
from a fresh application's main menu. Errors remain inside the slot screen;
cancelling restores the paused game. Back to Game resumes it without advancing
the game clock while menus/fades were displayed.

`SDL_GetPrefPath(NULL,"wolf3d-revive")` supplies a writable per-user directory,
normally `~/.local/share/wolf3d-revive/` on Linux (`XDG_DATA_HOME` respected) or
`%APPDATA%/wolf3d-revive/` on Windows. The six files are `slot1.w3s` through
`slot6.w3s`. Back up these files to preserve progress. No game archives or saves
are written into Steam, the source tree or `WOLFSRC/`. Failure to obtain/create
the user directory disables save/load and reports the reason. There is no
persistent configuration in this milestone.

## Version 1 format

Every disk integer is an explicit four-byte little-endian word; signed values
use a defined two's-complement encoding. Booleans are words 0/1. C structure
padding, pointer widths, process addresses and host endianness never enter the
file. The payload is bounded to 256 KiB before allocation.

| Header byte | Value |
| --- | --- |
| 0 | Seven ASCII bytes `W3RSAVE`, then NUL |
| 8 | Format version, currently 1 |
| 12 | Header length, 96 bytes |
| 16 | Exact payload byte length |
| 20 | Zero-based slot identifier |
| 24 | IEEE CRC32 of the complete file, with this word zeroed |
| 28, 32, 36 | Zero-based episode, floor and difficulty |
| 40 | 32-byte NUL-terminated printable ASCII description |
| 72, 76, 80 | CRC32 identities of MAPHEAD, GAMEMAPS and VSWAP |
| 84 | Original logical digitised-device mode, 0/1 |
| 88–95 | Reserved, written zero |

Payload order is deliberately fixed: the 28 game words, eight six-word floor
statistics records, RNG/device mode/actor high-water/static/door counts, player,
allocated actor records, statics, doors, moving-wall fields, the 64×64 grids
(x-major), noise/thrust and hidden combat/movement controls. `WL_SAVE.c`'s
`GAME_INTS`, `ACTOR_INTS` and `Payload` define each field in this order. Actor
records contain 20 data words plus state and next identifiers; pool slots also
have an allocation boolean. Grid cells contain wall, area, actor reference,
static-block and pushable words. Missing actor references are signed -1;
0–149 refer to pool indices. Player.next heads the preserved list.

`WL_SAVE_STATES.inc` lists 303 stable state identifiers; 0 means NULL. Append
new identifiers without reordering existing ones; incompatible changes require
a format version bump. No unknown version is guessed or deserialized. Metadata
inspection reads the header and streams CRC verification without constructing a
simulation, with distinct empty/corrupt/unsupported/unavailable labels. No DOS
save import is offered, and future version migration is not implemented.
CRC32 detects damage and resource mismatches; it is not authentication.

## Write and load transactions

Writes capture a snapshot and validate it before creating a same-directory,
exclusive temporary file. The complete file is written, flushed, synced and
closed before replacement. POSIX overwrite uses `rename`; an unapproved new
slot uses `link` for atomic create-if-absent, preventing a concurrent writer
from being overwritten. The temporary name uses process ID and a local counter,
never gameplay RNG. Windows uses UTF-8-to-wide filesystem paths, `_commit` and
`MoveFileExW` with write-through and optional replacement. Failure removes the
temporary file and retains the previous slot. POSIX directory syncing is best
effort after commit; ultimate crash/power-loss guarantees depend on filesystem
and OS support. Unapproved creation can fail on filesystems without hard links.

Loading stages and validates the entire snapshot before replacing the active
map: lengths, checksum, version/slot/metadata, resource identity, counts, enums,
state IDs, coordinates, door-wait indices, linked-list cycles/membership,
occupancy references, door/pushwall relationships, reachable-map enclosure,
inventory and hidden timers.
Original map decoding and sprite/wall resource validation also complete before
commit. `CA_ReadMap` and `CA_InstallMap` separate staging from the last fallible
map allocation. Pointer reconstruction and simulation installation then require
no allocation. Failure leaves the live simulation, clock, RNG and previous save
unchanged. Resource caches may warm during validation.

Pool holes and high-water allocation are preserved, rather than respawning a
level. This matters when a projectile or morphed boss takes the next free slot.
Actor tile reservations are restored exactly, including overlapping original
patrol reservations. Previous-frame visibility/view fields remain available
for attacks on the first resumed tic. A paused redraw uses local actor copies
and does not collect pickups or activate actors.

## Audio and intermissions

Loading stops obsolete effects, restarts the correct level song and invalidates
queued SDL output. Current music/effects volumes and mute settings remain in
force; PCM/OPL/device state is rebuilt, never serialized. The saved logical
5/140-tic boss-delay mode is restored independently of mute, `--no-audio` or
physical output failure. Existing emu8950 synthesis is unchanged.

`WL_INTER.c` now resumes the original counting loops at their sound waits and
30-VBL boundaries. Time counts in 500-point steps, ENDBONUS1 plays every 50
iterations, and ratio blips every ten percentage points. ENDBONUS2 ends ordinary
counts; perfect/zero ratios wait 30 tics before PERCENT100/NOBONUS. Perfect ratios
add 10,000 points. Counters otherwise advance immediately, as in the source,
rather than acquiring an arbitrary per-frame speed. Display and sound sequencing
use 70-Hz tics and the original 140-Hz AdLib effect lengths, even with muted or
unavailable output. No audio-device queue length controls the simulation.

BJ alternates breathing frames after the source's initial >10 then >35-tic
intervals. A key during counting shows final values and awards the bonus once;
a separate acknowledgement leaves. Natural completion awards once, records
ratios and waits for acknowledgement. Secret floors show their original 15,000
bonus. Counting, breathing and sound events never consume gameplay RNG. Menu
fades and the existing fixed-tic presentation remain native resumable routines,
not a DOS instruction-timing emulation.

## Verification

Verified on 2026-11-02:

- Final CART Linux build passes (38 application C translation units). The full
  **64-case regression suite with owned data passes** (305.648 seconds), including
  all 240 map/difficulty combinations and six controlled SDL finale paths.
- The final **13 save/intermission cases pass with ASan/UBSan** (55.923 seconds)
  and without sanitizers (37.963 seconds), including the later enclosure and
  door-wait validation. The application also builds with strict warnings as
  errors and sanitizers and passes an owned E1M1 five-frame SDL smoke test.
- The full application/C/SDL ASan/UBSan run passes **63 cases** (791.936
  seconds), with `UBSAN_OPTIONS=halt_on_error=1` and no sanitizer reports. It
  started before the additional six-episode continuation case was added; the
  final 13-case run above includes that case and all final save-validator and
  intermission changes. All 64 current cases therefore have sanitizer coverage.
- CART's SDL3 and desktop-output-directory regression scripts pass.
- All 38 C units cross-compile/link with strict MinGW C11 warnings as errors
  against the official SDL3 3.4.18 MinGW SDK. The Windows SDL save/menu executable
  passes write/overwrite/cancel/corrupt-slot checks and a separate-process load
  under Wine, including a UTF-8 directory component. **Wine is not native Windows
  verification.** The Windows application also passes five-frame startup under
  Wine. A Windows-produced save loads in Linux and reserializes to an identical
  complete payload. SDKs, prefix and executables stay under ignored `build/checks/`.
- Original-art six-slot selection, name editing, overwrite/load panels and
  counting/final intermission captures were inspected. BJ frame changes are
  separately checked at the original 11/36-tic boundaries. Captures remain
  private and ignored. Linux application startup creates its SDL user-data
  directory with `XDG_DATA_HOME` redirected away from the game-data directory.

The reused SDL static library is not sanitizer-instrumented. The established
Milestone 6B physical Linux listening check remains documented in
[MILESTONE6B.md](MILESTONE6B.md); this milestone adds automated sequencing and
SDL output checks, not a new human DOS A/B listening claim.

Tests added in `test_save.py`, `save_tests.c`, `save_sdl_tests.c` and
`intermission_tests.c` cover:

- All six slots and explicit header/CRC layout; registry lookup for every state.
- Separate-process load and continued simulation, including six episodes with
  synthetic and owned archives; complete payload/RNG comparison against an
  uninterrupted run with identical inputs.
- Mid-attack inventory, health/lives/keys/score, collected items, active doors and
  pushwalls, projectile movement, dead bosses, freed actor holes and respawn.
- Truncation, bad lengths/checksums/versions, wrong source identity, malformed
  state/object/grid references and counts, with unchanged live-state payloads.
- Read-only directory and partial-write failure; previous slot equality,
  temporary cleanup and competing create-if-absent writers.
- SDL menu events, name editing, overwrite/cancel, empty/corrupt/unsupported
  slots, failed load preservation, pause/resume and UTF-8 save-directory paths.
- Current audio settings, level music, obsolete effects/queued-output generation
  and logical boss delay restoration.
- Exact intermission sound ordering/waits, tempo, skip/acknowledge, single awards
  and unchanged RNG, using independently generated 140-Hz test effects.

## Limits

Save points are restricted as described above. Music restarts at the level's
beginning after load; it does not resume at an exact sample/OPL phase. Descriptions
use the existing unshifted ASCII input mapping, without Unicode text editing.
There is no timestamp, cloud storage, DOS compatibility or generic migration
framework. Native Windows physical-device testing and exhaustive DOS A/B audio
comparison remain outstanding. Controlled campaign tests are not fully navigated
human playthroughs of all episodes. Existing licensing and attribution remain
unchanged; no commercial asset bytes or screenshots are committed.

