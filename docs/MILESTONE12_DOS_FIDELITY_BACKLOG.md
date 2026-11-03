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
  dialogs implemented; save overwrite migration remains in Batch D.
- [x] Original Pause picture at (128,64), music off/on, input clearing and clock
  freeze (`WL_PLAY.C CheckKeys` → `UI_PAUSE`). Verified by synthetic and owned-data Classic regressions.
- [x] New-game confirmation after episode selection, before skill selection
  (`CP_NewGame`, `CURGAME` → `QUICK_NEW`). Verified by synthetic and owned-data Classic regressions.
- [ ] Menu transitions/audio: existing dark-red menu fades, movement/select/
  Escape sounds retained. Exhaustive physical DOS audio comparison pending.

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
- [x] Final death and victory → qualification → name entry or 500-tic wait →
  title/attract (`WL_GAME.C GameLoop` → `UI_RESULTS`). Historical source has
  **no separate Game Over card** in these paths; do not invent one.
- [x] Explicit End Game sets zero lives and enters the existing death sequence,
  then qualifies the score (`CP_EndGame` → `QUICK_END`).

Verification: strict application compilation passed. `test_classic.py`: all
nine synthetic/owned tests passed (34.935 s), including a second process reading
persisted rankings, exact ties, edit insertion, cancellation, corrupt-file
protection, final End Game path and unchanged RNG. Full engine/finale regression
validation is running; Batch E records its final outcome. Shifted punctuation
and exhaustive text-entry parity remain in Batch D.

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
- [ ] Debug functions: `--goobers`, original Backspace+LeftShift+Alt unlock,
  Tab+G/H/I/E/F/C/P/Q implemented (god mode, hurt, items, end level, coordinates,
  counts, picture pause, quit). Damage bypass is false by default and isolated
  from demos; it is session-only, preserving save format. Warp/shape viewer,
  memory diagnostic, border-register and timing debugger functions remain.

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
  original hotkeys and key-repeat filtering; menu Pause toggles music. Gameplay
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

- [ ] Full strict C11 regression suite, owned-data tests, no unexpected skips.
- [ ] ASan/UBSan application and regressions.
- [ ] All four original demos/5386 reference commands, Classic and Enhanced.
- [ ] 320×200 UI / aspect-correct 4:3 in widescreen output.
- [ ] Enhanced independence, save/config compatibility and 70 Hz/RNG checks.
- [ ] Physical DOS visual/audio comparisons (do not claim from synthetic tests).
- [ ] Final limitations and edition matrix, source tree/Git safety checks.

## Resume notes

Batch A: `test_classic.py` passed all eight synthetic/owned cases (33.981 s),
strict C11 warnings as errors. Tests cover nine sayings, confirmation blink and
release gating, Pause clock/RNG isolation, new-game confirmation, F1–F10,
all viewport sizes and four full demo durations. Exhaustive audio and save
overwrite parity remain unchecked above. Batch B focused checks passed; Batch D focused checks passed; full normal/sanitizer suites running. Private logs belong under ignored `build/`; owned data
is available locally but must never be committed. No commercial assets added.
