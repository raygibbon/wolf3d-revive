# Milestone 12: DOS fidelity completion

Branch: `feature/12-dos-fidelity-completion`. The requested checklist was absent
from the starting branch; this document reconstructs its batches from the task.
Historical `WOLFSRC/` is read-only. Checks below distinguish implementation,
regression verification and physical DOS comparison.

## Batch A — personality and menus

- [x] Nine quit sayings and DOS selection behavior (`WL_MENU.C endStrings`,
  `CP_Quit`, `CP_CheckQuick` → `MenuQuitMessage`, `QuickDialog`). Verified by synthetic and owned-data Classic regressions. Preserve C precedence `a & (7 + (b & 1))`. Separate
  UI entropy intentionally protects gameplay/demo RNG order.
- [x] Confirmation cursor, Y/N/Escape, answer-release wait and sound
  (`Confirm` → `BeginConfirmation`/`HandleConfirmation`). Shared main/quick/new
  dialogs verified; save overwrite uses the same implementation in Batch D.
- [x] Original Pause picture at (128,64), music off/on, input clearing and clock
  freeze (`WL_PLAY.C CheckKeys` → `UI_PAUSE`). Verified by synthetic and owned-data Classic regressions.
- [x] New-game confirmation after episode selection, before skill selection
  (`CP_NewGame`, `CURGAME` → `QUICK_NEW`). Declining returns to the main menu;
  quick-save selection resets only after accepting a difficulty. Verified by synthetic and owned-data Classic regressions.
- [x] Menu transitions/audio: original dark-red fades and movement/select/
  Escape sounds retained. `CP_Quit`/`CP_CheckQuick` silence music and effects
  before fading out; focused regression verifies both streams stop. Exhaustive
  physical DOS audio comparison is separately pending in Batch E.

## Batch B — high scores and completion

- [x] DOS seven-entry defaults, strict score/floor tie qualification and episode
  display (`ID_US_1.C Scores`, `WL_INTER.C CheckHighScore/DrawHighScores` →
  `WL_HIGHSCORES.c`, `WL_INTER.c`). Exact ties preserve existing ordering.
- [x] Editable names with original 57-character/100-pixel pre-insertion limits,
  I-bar cursor, arrows/Home/End/Backspace/Delete, Enter accepts, Escape retains
  the inserted blank name (`US_LineInput` → `HandleHighScoreEntry`).
- [x] Portable atomic `highscores.w3h` persistence, independent of saves/config.
  Seven explicit little-endian records, version/count and checksum validation.
  Invalid/future files are preserved; write errors keep session rankings and
  show a readable message. No DOS CONFIG.WL6 import.
- [x] Final death and victory → original 30-tic black fades → qualification →
  name entry or 500-tic wait → title/attract (`WL_GAME.C GameLoop` → `UI_RESULTS`). Historical source has
  **no separate Game Over card** in these paths; do not invent one.
- [x] Explicit End Game sets zero lives and enters the existing death sequence,
  then qualifies the score (`CP_EndGame` → `QUICK_END`).

Verification: strict application compilation passed. `test_classic.py`: all
nine synthetic/owned tests passed (34.935 s), including a second process reading
persisted rankings, exact ties, edit insertion, cancellation, corrupt-file
protection, final End Game path and unchanged RNG. Full engine/finale regression
validation is recorded in Batch E. Shifted punctuation and native editor
repeat support are covered in Batch D.

## Batch C — startup and hidden features

- [x] Sign-on device lights use `WL_MENU.C IntroScreen` coordinates/colors:
  mouse (164,82), synthesis-only (164,128), digitized (164,151), 12×2, index 14.
  Front end supplies actual SDL mouse / loaded audio capabilities. No invented
  conventional/EMS/XMS sizes, joystick, Sound Source or physical ISA detection.
  Those hardware-specific indicators remain intentionally unfilled.
- [x] Startup retains Milestone 11 artwork, FinishSignon prompt and rating/title
  ordering; native DOS memory-manager startup order is not simulated.
- [x] Robert's Jukebox (`WL_MAIN.C DoJukebox` → `DemoLoopJukebox`): all 18 labels,
  original track indices, three six-song banks, cursor, selection and Escape.
  Held M at startup, M at sign-on, or `--jukebox` enters it. Portable bank choice
  uses monotonic time instead of DOS hundredths; exit returns to title.
- [x] MLI and BAT (`WL_PLAY.C CheckKeys` → `HiddenKeys`): exact inventory,
  zero score, +42000 tics, original text, paused acknowledgement/input isolation.
- [x] Portable debug scope audited: `--goobers`, original Backspace+LeftShift+Alt unlock,
  Tab+G/H/I/E/F/C/P/Q/W/X implemented (god mode, hurt, items, end level, coordinates,
  counts, picture pause, quit, episode-local warp and original placeholder). Damage bypass is false by default and isolated
  from demos; it is session-only, preserving save format. Warp validates floors 1–10, preserves inventory and resets entry score, floor
  counters and music through normal setup. The DOS page-cache shape/memory
  viewers (EMS/XMS/address/last-hit fields), VGA border-register control and
  simulation/VBL timing debuggers are not emulated; these hardware diagnostics
  have no equivalent in the portable fixed-step engine.

Verification: `test_classic.py` all 10 tests passed (44.546 s); `test_signon.py`
all 16 passed (13.197 s), strict C11. Coverage includes MLI penalty/RNG, BAT,
unlock, invulnerability/hurt, every jukebox bank, original sign-on pixels outside
supported overlays, prompt, input clearing and exact 4:3/widescreen geometry.
Physical DOS audio timing and non-WL6 edition layouts remain unverified.

## Batch D — interface

- [x] `DrawLSAction`/`DiskFlopAnim` → original Saving/Loading windows and both
  disk pictures. Separate nonblocking start/work/finish phases for slot and
  quick operations, no simulation/RNG advancement. Portable atomic I/O remains
  synchronous within its work phase; disk-flip cadence follows those phases,
  not DOS serialization-chunk timing. Six existing slots/save version retained.
- [x] Overwrite uses original `GAMESVD`, shared blink/release/sounds. Invalid
  slot errors retain native validation and explanatory messages.
- [x] Mouse menu navigation follows `ReadAnyControl`: accumulated movement,
  strict >60-count threshold, horizontal priority, left accept/right cancel.
  Honors mouse-enabled preference; confirmation answers remain keyboard-only.
  No pointer warp, absolute hit-testing or stretched coordinates introduced.
- [x] Keyboard review: save slots gain existing held-navigation repeat; shifted
  US punctuation, uppercase and Caps Lock are captured when typed. Menus retain
  original hotkeys and key-repeat filtering; menu Pause toggles transient music playback without changing preferences or
  restarting the IMF stream (`CheckPause`/`SD_MusicOff`/`SD_MusicOn`). Gameplay
  dialogs use 160-pixel message height, full menus 200, and original Message's
  final-line padding rule. Non-US text input/IME remains unimplemented.
- [x] Controller audit: current engine has no SDL joystick/gamepad binding or
  calibration layer. DOS gameport/Gravis rows are not advertised as functional.
  Restoring usable controller input requires a separate native input feature;
  no fake device detection/settings were added.
- [x] Edition audit: this port selects GOODTIMES registered WL6 layout. F1 boss
  screen, six episodes and original 320×200 UI are retained. Shareware locks,
  SPEAR/JAPAN/SPANISH layouts and non-GOODTIMES help need matching archive
  layouts and verification; no art/chunk remapping is guessed. Native sound
  volumes, display/preferences, binding editor and six portable slots remain
  intentional modernizations independent of Enhanced rendering.

Verification: all 11 Classic synthetic/owned cases passed (45.112 s); all 13
save tests passed (41.183 s), strict C11 warnings-as-errors. New checks exercise
mouse thresholds/priority/buttons, keyboard-only confirmation, shifted text,
menu music toggle, both disk frames, quick operation failures and restart/load.
Synthetic fixtures gained invented disk chunks; no commercial assets added.

## Batch E — verification matrix

- [x] Full strict C11 regression suite, owned-data tests, no unexpected skips.
- [x] ASan/UBSan application and regressions.
- [x] All four original demos/5386 reference commands, Classic and Enhanced.
- [x] 320×200 UI / aspect-correct 4:3 in widescreen output.
- [x] Enhanced independence, save/config compatibility and 70 Hz/RNG checks.
- [x] Executed-DOS visual comparison: 20 exact dialog frames under DOSBox.
- [ ] Physical DOS audio/CRT comparison and exhaustive remaining UI states.
- [x] Final limitations and edition matrix, source tree/Git safety checks.

## Remaining scope and resume point

Portable registered GOODTIMES WL6 work in Batches A–D is implemented and
regression-tested within the limits above. The next unchecked work is physical
DOS audio/CRT comparison, followed by edition-specific layouts when matching
owned data/reference executables are available. Do not reintroduce a Game Over
card: this source's GameLoop transitions directly to CheckHighScore.

| Area | Verified result / remaining difference |
| --- | --- |
| DOS visual oracle | Original DOS executable under DOSBox; 20 full frames, zero differing pixels. This covers quit and Loading/Saving dialogs, not every UI state. |
| DOS audio | Native original music/effect decoding, 700/140 Hz schedules, priorities, all 27 loop durations and RNG isolation covered. Exact physical DAC/OPL output and enabled-DOS menu timing remain unverified. |
| Hardware startup | Original artwork and real runtime capability lights; no fabricated DOS memory totals, EMS/XMS, ISA, gameport or Sound Source detection. |
| Input | Original keyboard/mouse behavior plus native editor repeats. No joystick/gamepad input implementation or non-US text/IME. Disabled device rows remain disabled. |
| Debug tools | Portable G/H/I/E/F/C/P/Q/W/X supported after original unlock. DOS page-cache/EMS/XMS/address viewers, VGA border registers and timing debuggers are not simulated. |
| Jukebox | All 18 songs and three banks; monotonic bank choice and title return replace DOS time/startup plumbing. Exhaustive selected/playing visual/audio comparison remains open. |
| Saves/preferences | Existing six slots, save version 1, atomic native storage, TOML settings and binding/display controls retained. No DOS ten-slot/config import. Scores have a separate versioned file. |
| Editions | GOODTIMES registered WL6 reference. Shareware/SPEAR/JAPAN/SPANISH/help layouts require matching assets and independent validation. |
| Video | Original 320×200 UI with 4:3 aspect correction; Enhanced rendering remains independent. Safe edge clipping remains; physical CRT, native Windows, ultrawide/high-DPI hardware testing is not established. |

All commercial art/captures/reference executables remain private in temporary
or ignored build directories. WOLFSRC is unchanged; commits are confined to the
requested feature branch. No push or merge is part of this work.

### Source-review follow-ups

Portable debug warp and complete object counts are covered by the hidden-feature
regression. God mode now suppresses health loss while retaining the original
flash/status effects. New-game refusal/main-menu return and quick-slot reset
were corrected against CP_NewGame. The focused Classic suite passed all 11
cases (44.956 s), audio 5 synthetic cases passed (owned case skipped in that
focused invocation), and all three synthetic/owned controls cases passed
(14.098 s). The latter now holds a mouse button across menu entry: a fresh
menu click is intentionally actionable after Batch D.

An actual DOS executable oracle now passes **20 full 320×200 frames with zero
pixel differences**: all nine quit messages at WindowH 200 and 160, and original
Loading/Saving windows. `scripts/ui_reference.py` checks historical EXE/MAP
hashes before private instrumentation; `ui_reference.s` calls original
SetupControlPanel, Message and DrawLSAction, then exports all four VGA planes
under DOSBox. No original drawing routine is replaced. `test_dos_ui_reference.py`
compares every pixel against Revive, using only privately supplied owned WL6
archives; its temporary commercial files/captures are removed afterwards.

### Portable input follow-up

MinGW's math classification macro narrowed double arguments under strict
conversion warnings. Mouse-motion validation now uses portable double limits,
with NaN/infinity regression coverage. SDL typematic events are admitted only
for editable text/navigation in names and warp input, matching the role of DOS
keyboard make repeats; gameplay, menu confirmation and held-key isolation keep
their existing event rules. All 11 focused Classic cases passed (52.051 s).
The final quit-audio source check also passed all 11 Classic cases (79.583 s).
Normal/sanitizer full suites and final portability checks are recorded below.

### Completion timing verification

`ID_VH.H VW_FadeIn/VW_FadeOut` use 30 tics and black; `WL_MENU.H` menu fades
use 10 tics and dark red. Completion/score transitions now retain that
original distinction. After this source correction, all 41 engine tests passed
(232.644 s), including six episode finales and final death, and all 11 Classic
cases passed (67.539 s). Final strict native/MinGW builds, Wine interaction and
the sanitized application UI/menu/level smoke checks also passed.

## Final validation — 2026-11-03

| Check | Result |
| --- | --- |
| Final full normal suite | **146 passed**, no skips/failures, 550.678 s |
| Full ASan/UBSan suite | **146 passed**, no skips/findings, 1381.710 s |
| Final completion timing follow-up | 41 engine and 11 Classic cases passed; final sanitized Classic rerun: 11 passed, 77.738 s |
| Executed original DOS visual oracle | 20 frames / 1,280,000 indexed pixels identical, including both WindowH values and Loading/Saving |
| Original demo synchronization | All four recordings / 5386 commands, every traced frame, Classic and Enhanced, reference view size 15 / DOS sound off |
| Strict C11 | GCC 14.2 and MinGW GCC 14, conversion/shadow/pedantic warnings as errors; CART SDL build passed |
| Latest sanitized application | UI validation and finite menu/E1M1 smoke checks passed |
| Windows executable under Wine | UI validation, mouse/text, hidden features, high-score persistence/restart and F1–F10 save/load/end/quit paths passed |
| Native Linux X11/OpenGL | 1920×1200 output: 1600×1200 corrected canvas, 160-pixel black side borders, every 5×6 source texel equal; capture visually inspected |
| Display/render compatibility | Synthetic/owned geometry, widescreen/ultrawide, all view sizes, HUD/death effects, snapshots, saves/preferences, fixed 70 Hz and RNG isolation passed |
| Git/reference protection | WOLFSRC unchanged, no commercial assets added, feature-branch-only commits, no merge or master commits |

The full normal pass above ran after the final completion-fade correction.
The full sanitizer run was supplemented by the final Classic rerun and latest
instrumented application smoke checks. Native C tests and application were
instrumented; the reused SDL static SDK and original DOS executable were not.
Wine/dummy-driver checks do not establish physical native-Windows behavior.

Reproduce the normal suite after the standard CART build:

```sh
WOLF3D_DATA=/path/to/owned/WL6 \
WOLF3D_BINARY="$PWD/build/cart-sdl/linux/wolf3d-revive" CC='gcc -O1' \
python3 -m unittest discover -s tests -v
```

Use the sanitizer build in [BUILD.md](BUILD.md#tests), then set `SANITIZE=1`,
`UBSAN_OPTIONS=halt_on_error=1` and `WOLF3D_BINARY` to that instrumented binary.
The original-DOS pixel test additionally needs DOSBox and GNU binutils; it skips
explicitly when owned data or DOSBox is absent. No skip occurred in these runs.

Private evidence: `build/m12-final-head-full.log`, `build/m12-frozen-sanitize.log`,
`build/m12-final-head-classic-sanitize.log`, `build/m12-completion-engine.log`,
`build/m12-native.log`, `build/m12-final-app-sanitize.log`,
`build/m12-wine-final.log` and `build/m12-cart-final.log`. These remain ignored.
Physical audio/CRT comparison and the explicitly unsupported edition/hardware
features above remain open; this milestone does not claim universal DOS parity.
