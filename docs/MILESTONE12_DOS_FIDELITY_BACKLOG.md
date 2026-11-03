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

- [ ] DOS seven-entry defaults, score/floor tie qualification and episode display.
- [ ] Editable player names, bounded original font width, Enter/Escape behavior.
- [ ] Portable atomic score persistence independent of saves/configuration.
- [ ] Final death and victory → qualification → name entry/timeout → attract.
- [ ] Explicit End Game behavior compared against historical CP_EndGame.

## Batch C — startup and hidden features

- [ ] Audit sign-on indicator fills against historical hardware detection;
  preserve real artwork and explicitly document portable limitations.
- [ ] Startup presentation/source comparison (existing Milestone 11 groundwork).
- [ ] Robert's Jukebox, startup M activation, six-song bank selection.
- [ ] MLI cheat and BAT Easter egg, acknowledgement and input isolation.
- [ ] Audit debug activation and supported portable functions; document hardware
  functions that cannot meaningfully be restored.

## Batch D — interface

- [ ] Save/load interstitials and overwrite confirmation parity.
- [ ] Mouse-driven DOS control-panel navigation.
- [ ] Keyboard repeat, hotkeys, edit cursor and menu return paths.
- [ ] Controller options/support audit.
- [ ] Edition-specific help/menu layouts and intentional modernizations.

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
overwrite parity remain unchecked above. Continue with Batch B. Private logs belong under ignored `build/`; owned data
is available locally but must never be committed. No commercial assets added.
