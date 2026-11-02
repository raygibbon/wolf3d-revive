# Milestone 6D — Classic and Modern controls

Work is on `feature/6d-wasd-controls`, based on the initial GitHub WASD commit
`22a20ea`. PR #1 remains available for review; master and WOLFSRC are unchanged.

## Audit and revision

The initial branch added a second `ControlMovement()` call for A/D. It built,
but combined Alt+arrow and A/D could apply two strafes in one tic. Forward plus
strafe also produced diagonal acceleration and repeated collision/tile-trigger
work. There was no preset, relative mouse input or settings UI.

Classic defaults now use the existing DOS control path, compared with
`WOLFSRC/WL_AGENT.C` and `WL_PLAY.C`: the original turn-angle fraction, 35/70
keyboard rate, 150 forward/strafe and 100 backward movement scales, collision
sliding and original Alt+forward diagonal behaviour remain. Modern combines
translation before one collision move and one tile-interaction update per tic.
Each action is boolean across its alternative bindings, opposite directions
cancel and combined strafe sources are clamped to one direction.

Modern diagonals preserve the forward/backward/strafe speed ratio, with their
vector length limited to the larger single-axis speed. Integer rounding may
shorten a diagonal by a few map-coordinate units (less than 0.01% of a tile).
`thrustspeed` uses that bounded speed, so walking diagonally does not accidentally
acquire the enemy hit-chance behaviour of running. Classic's original diagonal
behaviour is deliberately retained. If dedicated strafe keys are assigned in
Classic, they use the combined translation path while active.

Both presets still run the same player, combat, actor, collision and map logic
at 70 Hz. Input collection never draws from gameplay RNG or changes the clock.

## Default mappings

Classic is the initial preset, preserving existing behaviour. Settings last for
the current application session, across new games and loads. No configuration
file or new serialization format is introduced.

| Action | Classic | Modern |
| --- | --- | --- |
| Forward / backward | Up / Down (keypad 8 / 2) | W / S, with Up / Down alternatives |
| Turn left / right | Left / Right (keypad 4 / 6) | Mouse X, Left / Right (keypad 4 / 6) |
| Strafe left / right | Alt + Left / Right | A / D; Alt + arrows also work |
| Run | Either Shift | Either Shift |
| Fire | Either Ctrl | Left mouse button or either Ctrl |
| Use/open | Space | Right mouse button or Space |
| Weapons | 1–4 | 1–4 |
| Menu/pause | Escape | Escape |
| Mouse | Disabled initially | Enabled initially |

Mouse can also be enabled in Classic. There is no vertical mouse movement,
freelook, acceleration curve or aim smoothing. The original slower backward
speed and original Shift-running speeds remain.

## Bindings and menus

Control in the main menu opens preset, mouse enable, sensitivity, invert-X,
reset and binding settings, using the existing fonts, colours and 320×200
renderer. Up/Down selects a row; Left/Right or Enter changes a value. Switching
presets resets bindings and mouse options to that preset's defaults. Reset
Defaults restores the currently selected preset.

The key-binding page has two alternatives for each action, including turning,
dedicated strafing, run, Alt-strafe, fire, use, weapons and pause. Up/Down browses
all actions, automatically paging five rows at a time. Left/Right chooses the
alternative. Enter waits for a key, Delete clears that slot and Escape cancels
capture or returns to the preceding page. Conflicts leave the old binding intact
and ask for another key; no existing action is silently unbound. Escape remains
a global pause/cancel fallback even when the configurable Menu binding changes.
Mouse buttons have fixed left-fire/right-use assignments when enabled.

`WL_CONTROLS.c/.h` holds the small action-to-two-keys table and pending input.
Physical key identifiers match SDL3 USB scancodes; the gameplay core does not
include SDL headers. Letters in the UI label physical QWERTY positions, rather
than translated Unicode characters. This works independently of Linux/Windows
keyboard layout; users can bind their preferred physical positions. Menu
navigation and save-description typing retain their existing ID_IN mapping.
There is no text-entry/layout rewrite in this milestone.

SDL key events feed physical gameplay keys and the separate, original menu
scan state. Legacy set-1 engine/test input calls are translated at that boundary.
This preserves keypad/modifier alias behaviour in menus while allowing separate
left/right modifier assignments for gameplay. Rapid make/break input survives
until one simulation tic consumes it. Transitions clear pending input so held
menu keys/buttons cannot start movement or fire on resume.

## Mouse timing and capture

`sdl_input.c` uses `SDL_SetWindowRelativeMouseMode`, with no OS-specific input
API. Capture is enabled only for living gameplay with mouse enabled, after
fade-in. Menus, intermissions, death/victory presentation, focus loss and shutdown
release it. Resume/respawn/focus regain restore it when appropriate. Capture
transitions discard old mouse motion; mouse buttons held across menus require
release and a fresh press. Keys received while unfocused are ignored. The game
continues its existing simulation while unfocused; focus loss does not add an
automatic pause rule.

Sensitivity ranges from 1 to 40, default 10. Each relative horizontal count adds
`sensitivity / 20` degrees: the default is 0.5 degrees/count. Invert X reverses
its sign. Fractional event counts accumulate, and whole twentieth-degree units
enter the existing angle-fraction accumulator on the next 70-Hz tic. Rendering
frames never rotate the player directly. Pending motion is consumed once, not
repeated across catch-up tics. Non-finite events are ignored; unusually large
events and pending rotation are bounded to prevent overflow.

Relative-mode failure logs a clear error and leaves keyboard controls usable.
It is retried after a focus/capture lifecycle change, rather than logging on
every presentation frame. SDL remains responsible for the actual platform
capture and relative-event implementation.

## Saves and scope

Save version 1 and audio formats are unchanged. The existing saved angle fraction
also preserves fractional mouse turning already applied to the simulation.
Pending keys/clicks/motion are transient input and are cleared by loading.
Current control settings stay in force after load, just as current audio
volumes do; they are not restored from the save file. Tests compare complete
save payloads after equivalent Classic/Modern forward input and after continuing
a Modern save with identical new input.

No audio synthesis, assets, fullscreen/widescreen, alternative artwork, external
input framework or original WOLFSRC file was changed.

## Verification

Verified on 2026-11-02:

- CART Linux build passes; all 40 application C translation units also compile
  with strict C11 warnings as errors in the sanitizer and MinGW builds.
- Full **67-case regression suite with owned WL6 data passes** (364.220 seconds),
  including all 240 map/difficulty combinations and all six controlled finale
  paths, audio loops, menus and saves.
- Full **67-case ASan/UBSan suite passes** (842.379 seconds), with
  `UBSAN_OPTIONS=halt_on_error=1` and no sanitizer reports. The final additional
  controls checks also pass separately under sanitizers (17.607 seconds).
- CART's SDL3 and desktop-output-directory regression scripts pass.
- The application and controls harness cross-link against the official SDL3
  3.4.18 MinGW SDK. Under an isolated Wine prefix, the controls harness passes
  Classic/Modern movement, settings, conflicts, mouse capture/buttons, focus,
  pause/resume and save continuation. Its complete Windows save payloads match
  for equivalent Classic/Modern input and uninterrupted/restored Modern input.
  Windows application startup also passes five frames with dummy drivers.
  These are **Wine API tests, not native Windows physical-input verification**.
- Final sanitizer application startup passes an owned E1M1 five-frame smoke
  test with SDL dummy video/audio. Controls tests use both independently
  generated synthetic archives and original owned files, without modifying
  the game-data directory.

The new controls harness covers:

- Classic forward/back/turn/keypad/Alt/run behaviour and Modern WASD translation.
- Opposite directions, duplicate alternatives, Alt plus dedicated strafe,
  bounded diagonal/run speeds, thrust-speed classification and static collision.
- Mouse count accumulation, sensitivity/inversion, disabled mouse, quick clicks,
  edge-triggered use, large/non-finite motion and valid angle wrapping.
- Conflicting/invalid bindings, reset/preset switching, weapon rebinding, custom
  pause keys and independent global menu navigation.
- Original-enemy/RNG and full-save-payload equality for equivalent forward input
  in both presets; unchanged RNG during event accumulation; deterministic Modern
  save/load and retention of current controls when loading a different preset.
- SDL event-driven settings/binding pages, conflict feedback, relative-mode
  enable/release, focus loss/regain, ignored unfocused keys, held menu buttons,
  pause-clock preservation and clean shutdown.

Original-art controls and binding panels were captured and visually inspected;
private captures and SDK/test binaries stay in ignored `build/checks/`.

The pre-existing SDL gameplay test was corrected to emit focus-regain events
before expecting new keyboard input. Its former sequence intentionally sent
keys while unfocused, which the new input boundary correctly ignores.

## Remaining verification gaps

Session-only settings reset when the application exits. No TOML persistence,
Unicode key-name rendering, mouse-button rebinding or gamepad support is added.
Native Windows physical keyboard/mouse testing remains required; MinGW and Wine
checks do not establish that. SDL's reused static library is not sanitizer
instrumented. Exhaustive multi-monitor, unusual mouse hardware and all keyboard
layouts are not claimed. Existing campaign coverage uses controlled encounters,
not full human playthroughs. A hands-on Linux test has been offered in the open
desktop window. The user volunteered to test; hands-on feedback is still pending
at the time of this report. No successful human-input test is claimed.
