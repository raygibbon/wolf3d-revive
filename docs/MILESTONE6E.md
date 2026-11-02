# Milestone 6E — Native SDL3 display

Work is on `feature/6e-native-display`, completing the initial `dc803ae`
implementation in PR #2. The branch was fetched and checked out from a clean
working tree, and reviewed against current `origin/master` (`51d767d`, merged
Milestone 6D). Local master, WOLFSRC, commercial assets and save formats are
unchanged. PR #2 remains open for review and is not merged.

## Review and fullscreen implementation

The initial change added a 320×240 logical presentation and two shortcuts. It
used an optimistic fullscreen boolean, logged synchronous failures but did not
reconcile asynchronous window-manager transitions, and forwarded shortcut
releases and the chord's Alt press into the input path. SDL integer presentation
also crops windows smaller than its logical size unless the application
supplies a fallback.

`sdl_video.c` now queries `SDL_GetWindowFlags()` for actual fullscreen state.
`SDL_SetWindowFullscreenMode(window, NULL)` selects borderless desktop
fullscreen before `SDL_SetWindowFullscreen()`. No exclusive display mode or
desktop resolution change is requested. Only an outstanding request has a
target state; that target is never reported as the actual state.

Window requests are asynchronous. Polling/presentation reconciles SDL flags
and observed window dimensions without blocking the simulation in
`SDL_SyncWindow()`. A three-second deadline reports rejected/constrained
requests and clears pending state. A later external/window-manager change is
still reflected by the next status query. Synchronous failures preserve the
previous scaling preference and remembered size, log SDL's error and show an
error in the settings panel. A successful change clears the error.

The last ordinary windowed size is remembered in window points. Fullscreen,
maximized and minimized dimensions do not overwrite it. Leaving fullscreen
restores that size; choosing a size while fullscreen changes the remembered
size for the next windowed transition. Manual resizing is supported, including
non-4:3 windows. No persistent configuration file is introduced.

## Aspect ratio and scaling

The original VGA setup in `WOLFSRC/ID_VL.C` starts with BIOS mode 13h and
configures planar VGA access. Its 320×200 game image was intended for a 4:3
display, with pixels taller than square pixels. Displaying those samples as a
square-pixel 320×200 image would instead produce 8:5 proportions.

The existing framebuffer and texture remain **320×200**. The nearest-filtered
texture fills a **320×240** SDL logical canvas: a vertical factor of 240/200,
or 6/5. This corrects presentation without changing drawing coordinates,
raycasting, artwork, view size or gameplay. Comparison here is against the
original VGA setup and expected display geometry, not a physical DOS/CRT
side-by-side test.

For drawable output dimensions `W × H`, fit scale is
`s = min(W / 320, H / 240)`. SDL centers the resulting `320s × 240s` viewport,
with at most pixel rounding at its edges. Integer mode uses `floor(s)` when
`s >= 1`, leaving black borders around whole multiples of the corrected
canvas. Integer scaling refers to that canvas: the 6/5 correction means raw
source rows do not necessarily have equal integer heights. At a canvas scale
divisible by five, source rows have equal whole-pixel heights.

If either drawable dimension is below 320×240, integer mode temporarily uses
fit scaling instead of SDL's minimum 1× integer viewport, which would crop.
For outputs below 4×3 pixels, presentation clears to black rather than drawing
an unusable zero-size rounded viewport. The renderer clears the entire target
black before drawing; wide and tall windows both retain 4:3 content. Texture
uploads still occur only when framebuffer or palette content changes.

## Settings and shortcuts

The former disabled Change View row is now **Display**, using the original
fonts, colours, panel drawing and menu transitions in the 320×200 framebuffer.
It is available at the main menu and while gameplay is paused:

| Setting | Behaviour |
| --- | --- |
| Fullscreen | Toggle borderless desktop fullscreen/windowed |
| Integer scaling | Toggle integer canvas scaling/fit |
| Window size | Select 1×–5×: 320×240 through 1600×1200 window points |
| Reset defaults | Windowed, fit, 3× (960×720) |

Up/Down selects; Left/Right or Enter changes a value. Reset requires Enter.
Escape returns to the preceding menu. A custom resize shows its dimensions;
arrows return to the preset range. Pending changes show “Switching...”, and
the panel reports failures and the small-window fit fallback. Settings apply
for the current session and survive game loads; they are not serialized.

Alt+Enter, including keypad Enter and either Alt key, toggles fullscreen.
F11 toggles integer scaling and is reserved from gameplay bindings. Shortcut
down, repeat and matching release events are consumed before menu/game input.
Only the Alt keys actually participating in a chord are suppressed; the other
Alt remains usable. Focus loss clears shortcut suppression. Unfocused
shortcuts are ignored. Fullscreen requests discard stale keyboard/button-edge
input so Enter cannot select a menu item or trigger an assigned action.
Held keyboard controls need a fresh press after such a transition.

## Capture, DPI and gameplay boundary

The window uses `SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY`.
Window presets/restoration use `SDL_GetWindowSize()` points. Scaling decisions
use `SDL_GetRenderOutputSize()` drawable pixels, rather than logical output
dimensions or window points. SDL owns the centered viewport and recomputes it
as the drawable changes. No extra renderer or graphics API is introduced.

Relative mouse mode releases while a display request is pending. It returns
only for focused living gameplay with mouse enabled; menus, focus loss and
pause retain the existing release rules. Old relative motion and input edges
are discarded across transitions. Held mouse buttons require release before
they can fire/use again, preserving Milestone 6D's protection.

Display state is outside the simulation and save payload. The small menu
backend has two get/set callbacks, so headless gameplay does not depend on
SDL video. Display operations do not call game RNG, advance simulation tics,
change the original 70-Hz clock, alter actors or rescale raycasting inputs.
Sound scheduling and save version 1 remain unchanged.

## Verification

Tests use independently generated archives and the player's installed original
WL6 data read-only. No game data, screenshots, SDKs or binaries are committed.
The new `tests/display_tests.c` / `tests/test_display.py` cover:

- Fullscreen entry/exit, external SDL state changes, custom window-size
  restoration, preset selection while fullscreen and reset defaults.
- Integer/fit viewport calculations and actual pixel readback of black bars
  and image colours in wide/tall windows, from 1×1 to 1920×1080.
- Drawable/window-pixel dimension agreement, high-density window flag and no
  redundant framebuffer uploads during presentation-only changes.
- Shortcut repeats/releases, keypad/right-Alt chords, opposite-Alt usability,
  F11 binding rejection and ignored unfocused shortcuts.
- Injected fullscreen, presentation and size failures, denied asynchronous
  requests, timeout recovery and subsequent successful changes. Fault hooks
  exist only in builds defining `WOLF3D_DISPLAY_TEST`.
- Original-art menu navigation, paused display changes, focus/capture
  release/regain, resume, unchanged RNG/tics during display calls and complete
  save-payload equality after equivalent gameplay with/without display changes.

Linux dummy video does not implement minimize; that optional test checks
restore only when minimize is supported. It does not pretend the operation
succeeded on an unsupported driver.

The full **70-case normal regression suite with owned WL6 data passes**
(314.104 seconds). This includes all 240 original map/difficulty combinations,
all six controlled finale paths, existing controls/audio tests and save/load
regressions. The full **70-case ASan/UBSan suite also passes** (756.221 seconds),
with `UBSAN_OPTIONS=halt_on_error=1` and no sanitizer reports. Application and
test C builds use `-std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow
-Werror`; sanitizer builds additionally use address/undefined sanitizers and
frame pointers. Commands and SDK setup are in [BUILD.md](BUILD.md).

Additional completed checks on 2026-11-02:

- CART Linux build and CART SDL3/desktop-output-directory regressions pass.
- The final three focused display tests pass under ASan/UBSan with owned data
  (20.328 seconds), including the explicit repeat-event check.
- All 41 application C units and the display harness cross-link with strict
  C11 warnings as errors using MinGW and the official SDL3 3.4.18 SDK.
  Windows geometry and synthetic gameplay/display tests pass under Wine with
  dummy video/software/audio drivers. Complete save payloads match after
  continuous gameplay versus equivalent gameplay with display changes.
  A five-frame owned-data Windows application smoke also passes under Wine.
- A separate Linux X11 desktop smoke check passes actual asynchronous
  fullscreen entry at 1920×1200, integer scaling, restoration to 960×720,
  resize to 1000×600 and mouse capture before/after transitions. It uses the
  application's SDL display API and normal event/presentation path, without
  blocking synchronization in that path, then closes itself.
- The owned-data Display panel was captured and visually inspected. The
  original fonts fit all four settings and help text without overlapping.
  Captures remain private in ignored `build/checks/`.
- Normal and ASan/UBSan Linux application startup pass five owned E1M1 frames
  with dummy video/audio and isolated user-data directories.

## Known limits

Physical high-DPI/2× monitors, multi-monitor moves, unusual desktop window
managers and native Windows hardware have not been tested. Dummy and the
available Linux desktop both exposed 1× pixel density; tests validate drawable
calculations and the high-density flag, not a real Retina/2× transition.
MinGW/Wine checks establish API/build coverage, not physical Windows behaviour.
The desktop smoke is automated and does not establish a human keyboard/mouse
test or DOS visual equivalence. The reused SDL static archive is not sanitizer
instrumented. Campaign regressions cover all maps/difficulties and controlled
finale paths; full human playthroughs are not claimed. Settings are session-only.
There is no widescreen gameplay, content filter, new artwork, renderer or GUI
framework in this milestone.
