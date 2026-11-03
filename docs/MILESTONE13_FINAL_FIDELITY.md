# Milestone 13: final DOS fidelity audit and release hardening

Branch: `feature/13-final-dos-fidelity`, based on fetched `origin/master`
`9e3c489` after PR #16. Review date: 2026-11-04. The starting working tree was clean.
`WOLFSRC/` is read-only; no commercial data, captures or executable bytes are
added. This is a conservative audit/hardening pass, not certification of complete
DOS equivalence or every future platform.

## Confirmed defects and corrections

1. **Missing transition inventory.** `scripts/audit_transitions.py` existed,
   but both required inputs were absent. Added the authoritative
   [JSON inventory](dos_state_transitions.json) and generated
   [review document](DOS_STATE_TRANSITIONS.md): 106 directed transitions,
   46 phases, all 28 native UI states and all 10 original `exit_t` values.
   Records link original functions, native implementation and scoped evidence,
   with explicit artwork, palette, timing, sound, input and remaining findings.
   Connected journeys cover startup/attract, new game, loading, pause, storage,
   death/restart/final death, ordinary/secret progression, victory/articles,
   qualification/name entry and quit. Caller-dependent panel return paths are
   described in the rows; this is not enumeration of every input permutation.

2. **Audit validation disappeared under Python optimization.** The previous
   validator used `assert` for all checks. Replaced these with explicit failures,
   added independent required-route coverage, nonempty palette/implementation/
   evidence checks, original-reference hash coverage, repository-bounded paths,
   active-state mapping and nonempty connected journeys. Missing, malformed or
   stale inventory/document files fail with a nonzero exit and clear diagnostic.
   `--write` regenerates only after validation. Four new Python cases cover
   missing/invalid/stale files, deleted required routes, source/hash/reference/
   enum/evidence corruption and `python -O`. No assets are needed for this audit.

3. **Established quickload unnecessarily showed Get Psyched.** Original
   `WL_MENU.C CP_CheckQuick(sc_F9)` invokes `CP_LoadGame(1)`, which sets
   `loadedgame=true` only around `LoadTheGame`, then clears it and returns to
   the existing play loop. An ordinary `CP_LoadGame(0)` retains the restart
   route through `WL_GAME.C GameLoop/PreloadGraphics`. Native `Save_Load` calls
   `GameRestoreClock`, which requested preloading for both cases. Clear only
   that request after a successful established quickload in `UI_DISK`.
   Ordinary loads, first-use slot selection, floor changes and debug warp keep
   their loading presentation. No save format or simulation rule changes.

   The Classic harness previously drained any loading screen before asserting
   the endpoint. Its new guard checks every frontend tic **before** that helper,
   including the pending-request interval, during established quickload. The
   synthetic and owned shortcut cases failed on the unfixed code, then passed
   with the correction. Corruption/revalidation and ordinary loading tests stay
   enabled. Focused Classic result: 11 passed, 49.010 seconds.

## Executed original DOS presentation evidence

The existing private runner verifies the historical EXE/MAP SHA-256 identities,
patches only its private copy, and executes original drawing routines under
DOSBox. Extended it from 20 to **30 full 320×200 indexed frames**, zero differing
pixels (1,920,000 compared indexed pixels):

- Existing nine quit sayings at both WindowH 200 and 160: 18 frames.
- Existing original Saving and Loading windows: two frames.
- Original `LatchDrawPic(GETPSYCHEDPIC)` at (48,56): one frame.
- Original `PreloadUpdate(current,5)` for current 0 through 5: six frames,
  including intermediate advancement, black remainder, 0x37 fill, 0x32 top
  highlight and final-column rule.
- Original Pause latch picture at (128,64): one frame.
- Original `Message(CURGAME)` drawing at WindowH 200: one frame.
- Original `DrawHighScores` with seven historical defaults: one frame.

The runner calls original `VW_UpdateScreen` before sampling latch drawings and
exports all four VGA planes. Native comparisons independently unplane every
pixel. Original default scores are encoded into private CONFIG records; the
user's CONFIG/saves are never copied or changed. The instrumentation remains
inside the existing bounded replacement DemoLoop region; no original drawing
routine is replaced. All generated executable/capture/data files are temporary
or ignored. There is no proprietary fixture in Git.

These are **controlled drawing comparisons**, with the Get Psyched/Pause/new-game
art on a common menu background. They are not continuous first/subsequent/secret
loading, gameplay Pause composites, animated high-score entry, fade/palette,
death-turn/fizzle or victory/article frame comparisons. The reference config
has audio disabled. Pixel equality does not establish palette/time/audio parity.

Existing `loading_tests.c` separately steps the actual native frontend clock,
checks partial real page reads, 30/70/30-tic waits, fresh acknowledgement,
original planar art/DAC values, ordinary/secret-entry/return/save loading,
weapon removal/death sound event, shortest arc/180° tie/speed/final angle,
intermediate red fizzle, acknowledgement and restart without Get Psyched.
Its ten attacker/start cases at all 16 Classic view sizes run against both
synthetic and owned archives: 320 death scenarios. Those are source-derived
native assertions, not newly executed-DOS death-frame evidence.

Expanded loading geometry coverage adds exact 21:9 (1680×720) and 32:9
(2560×720) to 960×720, 1280×720, 1280×800 and the previous 1720×720 case.
Each checks the final Enhanced geometry while black, the same presentation
rectangle through every reveal tic and the first gameplay frame, and unchanged
window dimensions. Alternating integer/fit settings exercise the DOS canvas
independently from Enhanced gameplay. An initial expanded test run omitted the
new expected logical heights; the test matrix now keeps width/height/expected
height together. No renderer change was necessary.

## Gameplay, audio and release review

| Area | Original / native review and regression evidence | Result and boundary |
| --- | --- | --- |
| Movement/turning | `WL_AGENT.C ControlMovement` / native controls and movement; four original demo state traces | Fixed-point/70 Hz and four-tic demo rules retained; no demonstrated new mismatch in sound-off reference scope. |
| Weapons/ammo | `WL_AGENT.C T_Attack`, `attackinfo`, inventory functions / `WL_COMBAT.c`; combat/demo tests | Firing, switching, clip limits, pickup quantities and attack clocks retained. |
| BJ face/damage | `WL_AGENT.C UpdateFace/TakeDamage`, `WL_PLAY.C` palette shifts / native combat/palette tests | Health/face/flash rules covered; enabled-DOS gatling-sound RNG exception remains below. |
| Enemies/bosses | `WL_STATE.C`, `WL_ACT2.C` / native actor/state/projectile/victory code; demo and six controlled finales | Alert/attack/pain/death, projectile and logical digitized death-delay assertions retained. Not six complete human playthroughs. |
| Doors/pushwalls | `WL_ACT1.C` / native door/grid/pushwall code; demo/campaign/save tests | Original timing, occupancy, secret counting and continuation retained. |
| Awards/exits | `WL_AGENT.C GivePoints/GiveExtraMan/GetBonus`, `WL_GAME.C ElevatorBackTo/GameLoop` / combat/campaign | 40000-point extra-life thresholds, nine-life cap, awards, ordinary and secret routing retained. |
| Music/effects | `ID_SD.C` schedules and original song/digitized maps / `ID_SD.c`, `sdl_audio.c`; audio tests | IMF 700 Hz/effect 140 Hz and PCM rate, priorities, track transitions, volumes, pause, disabled output and RNG isolation covered. |
| FM/PCM balance | Existing ×4 FM gain in the wide accumulator before saturation; PCM path unchanged | No measured physical-device evidence warrants changing it. Software loop/headroom tests are not a physical loudness calibration. |
| Asset safety | `ID_CA.c`, `ID_MAP.c`, `ID_PM.c`, `ID_SD.c`; malformed/truncated/archive bounds tests | Bounded expansion, offsets, sprite posts, map dimensions and allocation failure paths retained. No new decoding defect demonstrated. |
| Storage | Native save/config/high-score staging and `save_file.c` atomic writes; corruption/failed-write/restart tests | Validate before applying state; preserve protected/unsupported files and failed replacement. No DOS format compatibility added. |
| Lifecycle | `main.c`, SDL audio/video/input; repeated game/load/floor/finale and memory counters | Owned resources released on stop/shutdown; audio failure can continue silently. Focus/capture/fullscreen recovery covered by SDL regressions. |
| Rendering | Classic framebuffer, viewport/HUD tests; Enhanced snapshot/render/display tests | 320×200 UI with corrected 4:3 canvas, all view sizes 4–19; widescreen gameplay shares original rules, interpolation stays optional. |

No synthesizer, renderer, simulation, file-format or dependency replacement was
needed. Strict C11 and SDL3/software rendering remain intact. Native Windows,
macOS, WebAssembly and Android viability is preserved at the source/architecture
level; only available Linux and MinGW/Wine builds are tested here. No Android
controls were added.

## Actual commands and results

Final runs use owned registered WL6 data, GCC with `CC='gcc -O1'`, the existing
static SDL3 SDK and temporary fixture/storage directories. Logs are ignored
under `build/m13/`. The original DOS executable and reused SDL archive are not
sanitizer-instrumented.

```sh
./build/tools/cart build --profile cart-sdl
python3 scripts/audit_transitions.py --write
python3 scripts/audit_transitions.py --check
python3 -O scripts/audit_transitions.py --check
WOLF3D_DATA=/path/to/owned/WL6 \
WOLF3D_BINARY="$PWD/build/cart-sdl/linux/wolf3d-revive" CC='gcc -O1' \
python3 -m unittest discover -s tests -v

gcc -std=c11 -O1 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror \
  -fsanitize=address,undefined -fno-omit-frame-pointer -g \
  -Isrc -I../cart/vendor/SDL/include \
  src/main.c src/engine/*.c src/platform/*.c \
  ../cart/vendor/SDL/build/libSDL3.a -pthread -lm \
  -o build/m13/wolf3d-revive-sanitize
SANITIZE=1 UBSAN_OPTIONS=halt_on_error=1 CC='gcc -O1' \
WOLF3D_DATA=/path/to/owned/WL6 \
WOLF3D_BINARY="$PWD/build/m13/wolf3d-revive-sanitize" \
python3 -m unittest discover -s tests -v
```

| Check | Current result / private log |
| --- | --- |
| CART SDL3 build | Passed; `cart-final.log` |
| Full normal suite | **152 passed**, no skips/failures, **592.691 s**; `full-normal.log` |
| Full ASan/UBSan suite | **152 passed**, no skips/findings, **1131.372 s**; `full-sanitize.log` |
| Audit validation | 106 transitions / 46 phases / 28 UI states / 10 DOS exits; normal, direct executable and `python -O` checks pass |
| Focused quickload regression | Failed before fix in both synthetic/owned shortcut cases; all 11 Classic cases passed after fix, 49.010 s |
| Expanded focused loading/death | Both synthetic/owned cases passed, 96.663 s; `loading-final.log` |
| Executed original DOS UI | 30 complete indexed frames identical; also included in full normal/sanitizer suites |
| Fresh executed original demos | All 5386 commands matched current native state trace; `dos-demo.log` |
| Strict native / MinGW C11 | GCC 14.2 sanitized application; all 50 units compiled and Windows executable linked with MinGW GCC 14; `mingw.log` |
| Application smoke / native Linux | Normal and sanitized checks/menu/level/audio-disabled cases pass; X11/software captures inspected; `smoke.log`, `native-x11.log` |
| Windows under Wine | Four UI/level/menu/loading smoke commands passed; `wine.log` |
| CART regressions | SDL3 and desktop-output-directory scripts passed; `cart-sdl3.log`, `cart-desktop.log` |
| Source/data protection | No `WOLFSRC/` change; no commercial assets or executables staged; published commit trees checked against local trees |

The fresh DOS reference comparison used:

```sh
python3 scripts/demo_reference.py --data "$WOLF3D_DATA" \
  --output build/m13/dos-demo-reference --fast
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy dosbox \
  -c 'config -set cpu cycles=100000' \
  -c 'mount c build/m13/dos-demo-reference' -c c: \
  -c 'trace.exe nowait' -c exit
gcc -std=c11 -O1 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror \
  -Isrc -I../cart/vendor/SDL/include tests/demo_trace.c \
  src/engine/*.c src/platform/*.c ../cart/vendor/SDL/build/libSDL3.a \
  -pthread -lm -o build/m13/demo-trace
build/m13/demo-trace "$WOLF3D_DATA" > build/m13/native-demo.csv
python3 scripts/compare_demo_traces.py \
  --dos build/m13/dos-demo-reference/TRACE.BIN \
  --native build/m13/native-demo.csv
```

MinGW compiled every `src/**/*.c` independently with
`x86_64-w64-mingw32-gcc -std=c11 -O1 -Wall -Wextra -Wpedantic -Wconversion
-Wshadow -Werror`, `-Isrc` and the existing SDL3 3.4.18 MinGW SDK include path,
then linked all 50 objects with that SDK's `-lSDL3 -lm`.
The resulting Windows executable passed `--check-ui`, `--check-level`,
`--menu --frames 10` and `--level --frames 240` under Wine/dummy drivers.
The reused private Wine prefix reported an unsupported pre-existing config;
it preserved that file and continued successfully with defaults.

Normal and sanitized Linux executables passed `--check-assets`, `--check-ui`,
`--check-level`, `--menu --frames 30`, `--level --frames 240` and the same level
smoke with `--no-audio`, using dummy software video/audio and isolated
`XDG_DATA_HOME`. Native Linux X11/software finite menu/level runs also passed
with isolated preferences; the private 960×720 Get Psyched readback was visually
inspected. This does not replace physical fullscreen/device/input testing.
CART's `tests/test_sdl3.sh` and `tests/test_desktop_dirs.sh` passed in the sibling
CART checkout using this repository's absolute `build/tools/cart` path.

All four original recordings are checked against the existing independent DOS
reference hashes in both Classic and Enhanced: 691/1899/1140/1656 commands,
2764/7596/4560/6624 tics, total 5386 commands. This pass replays the recordings
against those references. A fresh private `demo_reference.py --fast` DOSBox
execution also matched the current native trace for all 5386 commands and the
expected four durations (`build/m13/dos-demo.log`). Fast instrumentation skips
wall-clock waits while preserving original four-tic simulation and sound-off
settings; this is not enabled-sound or physical-time evidence.

The first exploratory full run overlapped test/oracle edits and is not final
validation (its 20/29-frame expectation differed during the runner extension).
Only completed final runs are reported above. No failing assertion was removed
or weakened. The quickload failure-before/fix-after and expanded geometry test
correction are preserved in private logs.

## Remaining differences and unverified claims

The inventory tracks eight finding groups and concrete closure criteria.
The following prevent a blanket DOS-equivalence or hardware-release claim:

- Gameplay entry uses a native palette fade and death restart redraws
  immediately, instead of the DOS `fizzlein` reveal. Red death fizzle deliberately clips to the viewport instead
  of DOS's inclusive edge overdraw. These known presentation differences remain.
- Native heap cache capacity and reads per update replace DOS main/EMS/XMS/disk
  cadence. Disk activity artwork follows native work phases rather than DOS
  serialization chunks. No claim of identical preload totals/duration.
- Continuous DOS death, fades, victory/ending and animated high-score entry
  remain independently unverified. The controlled drawings above do not close
  those findings, and there is no physical VGA/CRT/DAC comparison.
- Physical OPL/Sound Blaster balance and latency are uncalibrated. Original DOS
  `UpdateFace` skips RNG while `GETGATLINGSND` plays; native gameplay intentionally
  remains independent of enabled audio. Silent demo parity does not cover that
  enabled-sound branch. DOS death also waits for active FM/PC sound completion
  after acknowledgement; native has no separate completion gate. Its early-ack
  timing relative to FM completion is unverified. Native HUD normalizes global
  demo floor numbering.
- Six atomic native saves, separate versioned scores/TOML settings, native
  controls/display/volume panels and separate UI entropy remain intentional.
  There is no DOS ten-slot save/config import or byte-compatible storage.
- Registered GOODTIMES WL6 is the supported reference. Shareware locks,
  SPEAR/JAPAN/SPANISH layouts and non-GOODTIMES help need matching owned data
  and separate source/artwork verification. BIOS text mode, ISA, EMS/XMS,
  Sound Source/gameport diagnostics and DOS hardware debuggers are not emulated.
  Native joystick/gamepad calibration/binding and non-US text/IME are absent.
- SDL dummy fullscreen/integer/fit/focus checks and Wine are not physical
  native-Windows, ultrawide/high-DPI/multimonitor or live audio-device testing.
  macOS, WebAssembly and Android builds/devices were not available for execution.

Shell Git/gh had no authentication credentials. Publication uses the connected
GitHub app, preserving the small commit messages and checking each remote tree
SHA against the corresponding local tree. The original local commits are kept
on an archive branch when reconciling the app-created commit metadata.

The changes are ready for review with these limits. A stable engine regression
result must not be described as historical equivalence beyond its evidence.

## Pointer-alignment follow-up — 2026-11-04

A user reported menu selection drifting away from the visible mouse pointer.
The SDL menu path accumulated relative movement with the original DOS strict
60-count threshold. That implements DOS stepping but cannot remain aligned with
a freely visible desktop cursor. Native menus now hit-test the absolute pointer
against their displayed rows. SDL converts window coordinates through the
renderer’s logical presentation, including DPI/letterbox/integer scaling; the
corrected 320×240 canvas is then mapped to the 320×200 interface framebuffer.
Captured gameplay continues to use relative motion.

Hover only changes selection on fresh mouse input. Idle frames do not repeat
movement or override keyboard selection. Button coordinates are processed even
without a prior motion event; clicks in borders or disabled rows cannot activate
the old selection. Main, episode/difficulty, sound/volume, controls/bindings,
display, save/load and jukebox rows use the same core hit-test. Text entry,
confirmation and view-preview actions retain their scoped input behavior.

The Classic interface regression now tests actual SDL events at 960×720,
1280×720, 1280×800, 1001×601 and 2560×720, in both integer and fit modes, with
synthetic and owned data. Deliberately unrelated relative motion in those events
must not affect the pointed row. Checks include stationary/keyboard ownership,
black borders, disabled rows and a click without prior motion. The earlier
DOS-threshold assertions are replaced by assertions for this requested native
input contract.

Validation: CART SDL3 build passed; **12 Classic cases passed** (39.847 s),
the same **12 passed under ASan/UBSan** (68.232 s, no findings), and **24 related
controls/display/DOS-menu/save cases passed** (84.231 s). All 50 application
units passed strict MinGW C11 compilation and linked. The four inventory
regressions passed after regenerating source links. Private logs:
`mouse-classic-final2.log`, `mouse-classic-sanitize.log`,
`mouse-regressions.log`, `mouse-mingw.log` and `mouse-audit.log`.

These follow-up checks supplement the preceding 152-case full runs; those full
runs predate this input adaptation. Historical DOS threshold stepping is now an
intentional difference, rather than a claim about current native mouse behavior.
