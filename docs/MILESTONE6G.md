# Milestone 6G: original DOS demo synchronization

All four recordings in the tested retail WL6 data now match the instrumented
original DOS executable for every command frame at Classic view size 15, with
DOS sound disabled. Their durations are 2,764, 7,596, 4,560 and 6,624 original
70 Hz tics. This is a measured comparison, not just a survival/duration check.

## Reference and investigation

The initial `Demo_Progress()` implementation compiled with CART before gameplay
changes. Eight Classic regression cases passed, including added assertions for
map, total/consumed commands, raw button bits, null/inactive queries, read-only
RNG behavior and stopped playback. The initial owned recordings died after
1,048, 928, 1,532 and 396 tics respectively.

The reference is the historical `WOLFSRC/WOLF3D.EXE` supplied with this repository,
paired with `WOLFSRC/WOLF3D.MAP`. These files have SHA-256 identities:

- EXE: `8614a3e4623b66181bf0aa88fc5a4ca36cb97fe5ef8c3b903954dfaaf683ab9b`
- MAP: `b3058257601f156fee914dcc926b5857753326ac0f1eada65bb0856a7b3f1928`

`scripts/demo_reference.py` assembles a small 16-bit logger with GNU binutils
and builds an instrumented **copy** of that DOS executable in a private output
directory. It checks both identities before applying address-based hooks,
rebuilds the MZ relocation table, and leaves tracked `WOLFSRC` untouched. It
replaces the attract-menu loop with calls to the original `PlayDemo(0..3)` and
skips startup acknowledgement. A wrapper calls the original `UpdateSoundLoc`
and then records the completed frame, preserving registers, flags and segments.
Player/actor thinkers, weapon logic, map loading, collision, renderer, RNG and
recorded commands in the DOS executable are not replaced.

This is executable instrumentation, **not a fresh compilation of the Borland C
engine**. A temporary source copy was tried with installed Open Watcom 2.0;
compilation stopped at missing Borland headers and `_seg` pointer extensions.
The original binary plus its matching symbols provides the actual DOS reference
without substituting the native port's code or changing historical source.
DOSBox 0.74-3 executed it. No physical DOS computer was used.

The runner copies only the eight required original archives into its private
working directory and creates a private DOS configuration: view size 15,
sound/music/digitized modes off. It does not copy the player's saves or settings.
The normal reference run retains the original four-tic real-time wait. Optional
`--fast` skips only that wait, retaining the original `TimeCount` assignments and
`tics=DEMOTICS`. Every existing record byte of the 5,386-frame timed trace was
compared with the accelerated trace and matched. The final runner adds the door
count to each record. Neither runner is linked into ordinary gameplay.

## First divergences and corrections

The first full-trace difference on the initial branch is visibility at command
1 in **each** recording: the port explicitly marked the player's starting cell
visible; the DOS raycaster only marks cells crossed by rays. This can affect
pickups. Actor projection also used `width/2` rather than DOS `width/2-1`.

The first projected actor difference and first player/inventory/RNG difference
were located independently for each initial recording (command numbers are
one-based, four original tics per command):

| Demo / source map | First actor projection difference | First primary-state difference | Native vs DOS |
| --- | ---: | ---: | --- |
| 0 / 37 (E4M8) | 51 / tic 204 | 33 / tic 132 | ammunition 88 vs 80 |
| 1 / 43 (E5M4) | 135 / tic 540 | 190 / tic 760 | ammunition 2 vs 1 |
| 2 / 56 (E6M7) | 12 / tic 48 | 167 / tic 668 | ammunition 23 vs 22 |
| 3 / 31 (E4M2) | 20 / tic 80 | 72 / tic 288 | ammunition 5 vs 4; RNG index 156 vs 157 |

After correcting each cause, tracing continued from the first newly differing
frame. The resulting shared gameplay corrections follow the original sources:

- `WL_DRAW.C` / `WL_DR_A.ASM`: remove the artificial starting-cell visibility;
  restore `centerx` for actor/tile projection, rotation and weapon targeting;
  preserve `FL_VISABLE` when projection is too close or a state has no shape.
  DOS clears that flag only on the non-visible branch.
- `WL_AGENT.C T_Attack` / `WL_PLAY.C PollControls`: a fresh attack press during
  an attack clears the current button state. That cleared state becomes the
  next frame's held state, including the attack's ending frame. Keep raw
  recorded bits separate from this latch. Use actions are suppressed during
  attacks with the same held-state semantics. This fixes subsequent shots
  without changing weapon tables, ammunition grants or demo commands.
- `WL_STATE.C KillActor`: clear the actual death cell after setting `tilex/y`
  from position. DOS can retain an old destination reservation; purging every
  reference to the dead actor changed door and movement behavior.
- `WL_ACT1.C`: door obstruction uses the marked grid, including corpses, and
  original center-row/column tests. Opening fully releases the reservation in
  the doorway. Closing checks its occupied cell rather than an enlarged
  physical bounding square. Continue the original blocked-door timer instead
  of clamping at 300, with explicit signed-16-bit wrap semantics.
- `WL_AGENT.C TryMove` and `WL_STATE.C TryWalk` (native `WL_OBJECT.c`): honor
  grid occupancy rather than testing every physical actor or independently
  re-blocking a door whose integer marker has been replaced by an actor.
  For demo 3 the remaining movement divergence was command 734, tic 2936:
  the native Y position stayed at 3,637,211 while DOS reached 3,664,601. The
  DOS doorway contained an actor reservation rather than a solid integer tag.

The RNG table/index algorithm, recorded bytes, movement constants, ray traversal,
weapon tables, 320×200 framebuffer and normal one-tic/70 Hz stepping are unchanged.
No immunity, extra ammunition, synthetic movement or per-map/demo workaround was
introduced. Save version/layout remain version 1; validation now accepts the
legitimate 16-bit door timer range instead of rejecting blocked timers over 300.

## Trace schema and repeatable comparison

`tests/demo_trace.c` is a developer harness, separate from the application. It
queries `Demo_Progress()` and advances exactly one four-tic command per step.
It emits a starting snapshot and then one snapshot after each completed frame:

- `F`: demo, consumed command count, original tic clock, player X/Y/angle,
  health, ammunition, RNG index, raw buttons and signed recorded X/Y commands,
  weapon, attack frame and attack countdown.
- `A`: actor order, stable state **name**, position/angle, destination tile,
  class, tic count, hit points, direction, speed/distance, temporary state,
  flags, activity and projected position/height. The number of rows is the
  actor count; no process addresses enter the portable trace.
- `V`, `M`, `Q`, `D`: visibility, runtime tiles, actor reservation slots and
  door orientation/lock/action/timer/position.

The DOS logger captures the corresponding original structures, current command
bytes and RNG index; `tests/demo_trace_tools.py` decodes explicit little-endian
16/32-bit fields and resolves state pointers using the original MAP. Bounds and
actor-list cycles are checked. It streams records rather than retaining a whole
trace in memory. Comparisons use completed frames, not the optional native
starting snapshot.

Only unused DOS door timers are normalized: `SpawnDoor` does not initialize the
field, and `DoorOpening` resets it before `dr_open` can use it. Timers **are**
compared while open. The port steps the player directly, so its unused actor
state pointer/class/activity fields are not compared with DOS's `s_player` /
`s_attack` object representation. Player position, controls, inventory and
weapon timing are compared explicitly. Actor activity is compared as a boolean.

`tests/fixtures/dos_demo_hashes.json` contains source identities and a SHA-256
hash of the complete canonical frame for all 5,386 commands. It contains no
recorded commands, maps, textures, audio or executable bytes. Owned-data tests
assert every hash and every expected duration; another data revision is skipped
explicitly until its own reference is captured. Synthetic tests check repeatable
tracing and four-tic cadence. Existing Classic tests still exercise one/four-tic
host batching, input isolation, abort/focus behavior and malformed/missing demos.

Example private workflow from the repository root:

```sh
python3 scripts/demo_reference.py --data "$WOLF3D_DATA" \
  --output build/checks/dos-reference
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy dosbox \
  -c 'config -set cpu cycles=100000' \
  -c 'mount c build/checks/dos-reference' -c c: \
  -c 'trace.exe nowait' -c exit

gcc -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror \
  -Isrc -I../cart/vendor/SDL/include tests/demo_trace.c \
  src/engine/*.c src/platform/*.c ../cart/vendor/SDL/build/libSDL3.a \
  -pthread -lm -o build/checks/demo-trace
build/checks/demo-trace "$WOLF3D_DATA" > build/checks/native-demo.csv
python3 scripts/compare_demo_traces.py \
  --dos build/checks/dos-reference/TRACE.BIN \
  --native build/checks/native-demo.csv
```

Raw DOS traces include original commands and map-derived state. Keep them and
private reference executables out of commits/distributions. The builder accepts
an ignored `build/` subdirectory or an external private output directory, refuses
non-empty destinations, and never writes into the original game-data directory.

## Results and limits

| Demo | Commands / tics | Final DOS/native health | Final ammunition / RNG index |
| --- | --- | ---: | --- |
| 0 | 691 / 2764 | 0 | 75 / 22 |
| 1 | 1899 / 7596 | 100 | 0 / 127 |
| 2 | 1140 / 4560 | 0 | 16 / 191 |
| 3 | 1656 / 6624 | 0 | 72 / 48 |

Deaths on the final commands in demos 0, 2 and 3 are also present in the DOS
reference. They are not suppressed to satisfy the duration checks.

Verification on Linux with the legitimate owned data:

| Check | Result |
| --- | --- |
| `./build/tools/cart build --profile cart-sdl` | Passed |
| Strict full-application GCC C11, `-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror` | Passed, including ASan/UBSan build |
| `WOLF3D_DATA=... python3 -m unittest discover -s tests -v` | **108 passed**, 407.772 s |
| Same complete suite with `SANITIZE=1`, `UBSAN_OPTIONS=halt_on_error=1` and instrumented `WOLF3D_BINARY` | **108 passed**, 931.948 s; no sanitizer findings |
| DOSBox timed/accelerated references; current runner/native comparison | All 5,386 command frames match |
| Synthetic repeatable trace, malformed clock/list/truncation checks, progress API | Passed |
| Door release, original corner/center rules, 16-bit timer wrap and timer-over-300 save round trip | Passed |
| Existing controlled finale/continuation tests for all six episodes | Passed in both full suites |
| CART SDL3 and desktop output-directory regressions | Passed |
| Linux X11/software-renderer five-frame E1M1 smoke, isolated preferences | Passed |
| Strict MinGW C11 full application and trace/Classic harness cross-links, SDL3 3.4.18 SDK | Passed |
| Wine native trace vs DOS | All 5,386 frame records match |
| Wine SDL dummy-video Classic owned-demo/progress/input-abort harness and application `--check-level` | Passed |

The regular and sanitizer commands use private fixture/save directories; the
manual native smoke uses an isolated `XDG_DATA_HOME`. The owned audio sanitizer
case takes several minutes of CPU time. Native Windows, hardware audio/DOS
listening comparisons and a fresh Borland C executable build were not performed.

Limits remain explicit:

- This establishes equality of the listed simulation/visibility/grid fields for
  these four recordings, this retail archive revision, view size 15 and silent
  DOS reference settings. Other viewport sizes, releases and audio configurations
  have not received equivalent DOS traces. It is not a full six-episode DOS
  equivalence claim or a framebuffer/CRT comparison.
- DOS `UpdateFace()` skips RNG work while `GETGATLINGSND` is playing. The native
  port retains its existing audio-independent gameplay RNG behavior. Enabled-DOS
  sound timing and that potential synchronization difference remain unaudited;
  ordinary SDL audio on/off independence continues to be regression-tested.
- Native HUD episode/floor numbering normalizes global demo map indices rather
  than displaying DOS global numbers such as floor 38. Safe scaler/fizzle clipping
  and remaining menu/high-score differences from the Classic audit remain.
- Windows results use MinGW and Wine; no physical Windows or DOS hardware test
  is claimed. The original C engine was not rebuilt with Borland/Open Watcom.
