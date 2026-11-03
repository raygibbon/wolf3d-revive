# Build and run Wolf3D Revive

CART builds original menus and normal-level gameplay: sprites, enemies,
weapons, pickups, HUD, life restarts, episode/difficulty selection, pushwalls,
elevator exits, intermissions and progression. All original bosses,
projectiles and six finale paths are implemented; all 60 maps load/render/
simulate at all four difficulties. Native AdLib music and digitised/AdLib effects
play through SDL3. Six named save slots, pause/resume and animated intermissions
are implemented; see [save format and validation](MILESTONE6C.md). See [audio support and limits](MILESTONE6B.md) and
[milestone 6A](MILESTONE6A.md) for verified scope and playthrough limitations.

## Local Linux build

From the repository root, reuse the existing SDL3 SDK in the sibling CART checkout:

```sh
./build/tools/cart build --profile cart-sdl --verbose
./build/tools/cart run --profile cart-sdl -- \
  --data "$HOME/.steam/steam/steamapps/common/Wolfenstein 3D/base"
```

Press any key during the title/credits/scores to open the menu. Use Up/Down
and Enter/Space; V, B and Q select View Scores, Back to Demo and Quit. Escape
opens quit confirmation in the menu (Y exits, N/Escape cancels). Closing the
window exits directly. New Game opens the episode and difficulty pages; arrows select, Enter/Space
confirm and Escape goes back. Sound opens functional Music/Effects enable
switches and independent volumes: Up/Down select, Enter/Space toggles a switch,
Left/Right changes volume in 5% steps. Settings persist on normal exit. Control opens Classic/Modern presets, mouse
enable, sensitivity/invert, reset and two-key action bindings. On the binding
page, arrows select action/alternative, Enter captures a key, Delete unbinds
and Escape cancels. Conflicts retain the old assignment. Changing presets
retains each profile's custom bindings and mouse options; Reset Defaults resets
only the selected profile. Other
unported main-menu rows stay disabled. A failed
level load reports the reason and returns to the menu after acknowledgement.

In the level: Up/Down move, Left/Right turn, Alt+Left/Right strafe, Shift runs,
Space operates doors, secret pushwalls and elevator switches. Ctrl fires, and
1–4 select owned weapons (knife, pistol, machine gun, chaingun). Keys unlock
their matching doors. Intermissions count the original ratios/time bonus with
original sounds and BJ breathing. A key skips counting; a separate key continues.
Escape during living gameplay pauses and opens the menu. Save Game and Load Game
use six slots: arrows select, Enter confirms, letters/digits/Space edit names,
Backspace deletes and Escape cancels. Y/N confirms replacement, including corrupt
saves. Back to Game resumes the paused simulation. Saving is unavailable during
death, intermissions and victory cinematics. Victory statistics acknowledge with a key; ending articles
use arrows, Enter or Page Up/Down for pages and Escape to close, followed by
scores and return to menu. `--level` starts E1M1 at medium difficulty directly.

Classic is the default, with the original keyboard controls above. To use WASD
and mouse, open Control and select Modern. W/S move, A/D strafe, mouse X turns,
left-click fires and right-click opens/uses; arrows, Ctrl, Space, Shift and 1–4
remain available. Mouse capture starts after gameplay fade-in and releases for
menus and focus loss. Sensitivity 1–40 sets 0.05–2 degrees/count (default 10,
0.5 degrees/count). Gameplay bindings do not change menu navigation. These
settings persist across normal restarts and remain current when loading a
save. See [Milestone 6D](MILESTONE6D.md) for input timing and validation details.

Change View in the main or paused menu changes the actual 3D viewport through
the original sizes 4–19 (64×32 through 304×152; default 240×120). Arrows preview,
Enter accepts and Escape cancels. The original teal border and 40-pixel HUD stay
around the view. This is independent of SDL window or presentation size.

Control → Display settings selects fullscreen, integer scaling and
window sizes from 320×240 through 1600×1200 (1×–5×). Up/Down selects a row;
Left/Right or Enter changes a setting. Reset Defaults restores a 960×720
window with fit scaling. Settings persist across normal restarts, including custom window sizes, and
remain current across loads.
Alt+Enter (including keypad Enter) switches borderless desktop fullscreen;
F11 switches integer scaling. These shortcuts are consumed before gameplay
or menu input; F1–F11 cannot be assigned to gameplay actions. Fullscreen changes
clear pending input, so release and press held controls again afterwards.

The unchanged 320×200 framebuffer fills a 320×240 logical canvas for the
original 4:3 proportions. Fit mode maximizes that canvas with black borders;
integer mode uses whole multiples of the corrected canvas. Below 320×240
drawable pixels, integer mode temporarily uses fit to avoid cropping. Windows
can be resized freely and restore their previous windowed size after fullscreen.
Size presets use window points; presentation uses drawable pixels on high-DPI
systems. Mouse capture releases during pending mode changes and returns only
for focused gameplay. See [Milestone 6E](MILESTONE6E.md) for calculations,
failure recovery and verification limits.

During living gameplay, F2 Save, F3 Load, F4 Sound, F5 Change View and F6
Control return directly to the paused game. F1 is the original GOODTIMES boss
key: a black C> screen, Escape to return. F7 confirms End Game; F8 selects a
quick slot on first use then saves directly; F9 selects a quick slot on first
use then confirms quickload; F10 confirms Quit. These features keep six native
save slots and the existing save format. View/control/display settings remain
current when loading.

After title, credits and scores, attract mode plays available T_DEMO0–3 streams
from the original graphics archive. A key or click aborts. Missing/corrupt demos
are skipped safely. All four tested owned recordings now match the instrumented
DOS reference at view size 15 with DOS sound off, through their exact durations.
See [Milestone 6G](MILESTONE6G.md) for private reference builds, frame tracing,
source identities and verification limits. Run the added regressions with
`WOLF3D_DATA=/path/to/WL6 python3 -m unittest discover -s tests -p test_demo_sync.py -v`.

`--no-audio` skips sound loading/device output. Otherwise music and sounds read
AUDIOHED/AUDIOT/VSWAP directly; nothing is converted or written to disk. Audio
startup/device errors report a reason and allow silent gameplay. The logical
original digitised mode (including its 140-tic boss death delays) is identical
with audio muted or disabled. No PC speaker mode is offered.

An ALSA-only SDL SDK can use `SDL_AUDIO_ALSA_DEFAULT_PLAYBACK_DEVICE=pulse` on
a PulseAudio/PipeWire desktop. The backend tries this endpoint automatically
if ALSA's default fails and no explicit device hint exists. This does not change
ALSA configuration. A full SDL SDK with native PulseAudio/PipeWire support
uses its own default routing.

Saves use `SDL_GetPrefPath(NULL,"wolf3d-revive")`: normally
`~/.local/share/wolf3d-revive/` on Linux (respecting `XDG_DATA_HOME`) and
`%APPDATA%\wolf3d-revive\` on Windows. Files are `slot1.w3s` through
`slot6.w3s`; back up that directory to preserve games. Original DOS saves are
not compatible. Save errors leave the active game and previous slot intact.
Descriptions use the original ASCII font and a 121-pixel / 31-character limit.

The discovered Steam directory above is used read-only. Supply a different absolute path if your installation
is elsewhere; `cart run` changes the working directory to the output folder.
No path is hardcoded into the executable and no game data is staged by CART.

`build/tools/cart` is the ignored current CART 0.2.0 executable produced during
milestone 1. If absent, build the current CART source in `../cart` and use that
executable instead. The installed `cart`/historical tracked binary reported
0.1.0 during milestone 1. No installed tool was overwritten in this milestone.
The local SDL SDK reports development version 3.5.0, revision `08b9c5539`.

To decode/check without a display, or perform a finite graphical smoke test:

```sh
./build/cart-sdl/linux/wolf3d-revive --data /absolute/path/to/WL6 --check-assets
./build/cart-sdl/linux/wolf3d-revive --data /absolute/path/to/WL6 --check-ui
./build/cart-sdl/linux/wolf3d-revive --data /absolute/path/to/WL6 --check-level
./build/cart-sdl/linux/wolf3d-revive --data /absolute/path/to/WL6 --level
./build/cart-sdl/linux/wolf3d-revive --data /absolute/path/to/WL6 --menu
./build/cart-sdl/linux/wolf3d-revive --data /absolute/path/to/WL6 --frames 3
./build/cart-sdl/linux/wolf3d-revive --help
```

`--capture build/checks/title.bmp` additionally saves a local SDL readback
of the initial title, menu (`--menu`) or level (`--level`) at full brightness, before
its fade-in (create the parent directory first). SDL readback contains
the image viewport, excluding letterbox bars. Captures contain game artwork;
keep them private and out of Git. `build/`, `*.bmp` and `*.ppm` are ignored.

## Other SDL SDKs and Windows

The base config uses ignored `vendor/SDL` with shared linkage. Without that SDK,
base `cart doctor`/`cart build` intentionally reports a missing dependency.
The optional `cart-sdl` profile overrides the root to `../cart/vendor/SDL` and
uses its existing static build; nothing is downloaded or installed.

Implemented `[deps.sdl3]` keys are `path`, `include_dir`, `lib_dir`, `lib` and
`linkage` (plus `enabled`). Defaults use `include/SDL3` and `build/`, with
fallbacks `build/Release`, `build/Debug`, `build/lib`. Adjust to your SDK layout.
The local profile supplies pthread; CART supplies SDL3 and libm. Other static
SDKs may require additional platform libraries in `libs`.

The Windows target explicitly selects GCC/MinGW so the warning flags match.
Use a Windows SDL SDK/import library, not the Linux `cart-sdl` profile. Put the
matching SDL3 DLL beside the executable or on PATH. All application C modules pass strict MinGW C11 compilation. A manual cross-link with
the official SDL3 3.4.18 MinGW SDK and a five-frame E1M1 smoke test under Wine
with dummy video/audio passed. Milestone 6C verifies Windows save/overwrite/cancel, UTF-8 save paths and
restart/load under Wine; see [MILESTONE6C.md](MILESTONE6C.md). Milestone 6D
cross-links all 40 C units and passes the Windows controls harness under Wine;
see [MILESTONE6D.md](MILESTONE6D.md). Milestone 6E cross-links all 41 C units
and passes the Windows display geometry/gameplay harness under Wine with
dummy drivers; see [MILESTONE6E.md](MILESTONE6E.md). Classic restoration builds
all 43 C units and passes synthetic/owned viewport, shortcut and demo tests
under Wine; see [CLASSIC_FIDELITY_RESTORATION.md](CLASSIC_FIDELITY_RESTORATION.md).
This is not native Windows verification;
physical Windows playback, device changes and resize behaviour still need
Windows testing. See [MILESTONE6B.md](MILESTONE6B.md). No CMake build system was introduced.

No CART source changes were required in milestones 2–6. The Linux/Windows
link lists now include the math library used by the original table builder.
CART recursively builds the lowercase
`.c` files under `src`, supporting C11, defines/includes and library lists.
It has no general linker-flags key, pkg-config integration, automatic SDK
build/download, DLL staging or rpath setup. `doctor` does not accept profiles.
Sanitizer flags only reach compilation via `cflags`; use the explicit combined
compile/link below for sanitizers. The prior generic output-directory fix and
its tests in CART remain as they were at the start of this milestone.

## Tests

Build the application with CART first, then run the synthetic/unit suite and
existing Git-protection tests:

```sh
python3 -m unittest discover -s tests -v
```

This compiles memory, Huffman, graphics/UI, input, timing, map/VSWAP, raycasting
movement, sprites, RNG, enemy AI, combat, pickups, HUD, death, campaign,
intermission and pushwall tests with GCC C11 and strict warnings as errors. SDL readback
and event-to-menu/game tests use the static SDK at `../cart/vendor/SDL` (override with `SDL_SDK`). The SDL tests use the dummy
video/software renderer, requiring no desktop. Without that static SDK, those
SDL integration tests report skips. Synthetic archives are generated in temporary
directories and removed; no asset fixtures come from commercial files.

For an independent pixel-by-pixel oracle comparison using your owned files:

```sh
WOLF3D_DATA="$HOME/.steam/steam/steamapps/common/Wolfenstein 3D/base" \
  python3 -m unittest discover -s tests -v
```

Optional Linux sanitizer build (normal builds remain CART-driven):

```sh
mkdir -p build/checks
gcc -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror \
  -fsanitize=address,undefined -fno-omit-frame-pointer -g \
  -Isrc -I../cart/vendor/SDL/include \
  src/main.c src/engine/*.c src/platform/*.c \
  ../cart/vendor/SDL/build/libSDL3.a -pthread -lm \
  -o build/checks/wolf3d-revive-sanitize
SANITIZE=1 UBSAN_OPTIONS=halt_on_error=1 WOLF3D_BINARY="$PWD/build/checks/wolf3d-revive-sanitize" \
  WOLF3D_DATA="$HOME/.steam/steam/steamapps/common/Wolfenstein 3D/base" \
  python3 -m unittest discover -s tests -v
```

`SANITIZE=1` also instruments the C unit/video tests. The reused SDL static
archive is not instrumented. The gameplay simulation and palette flashes use
fixed 70 Hz tics. The campaign suite exercises selection, exits, statistics,
secret routing, pushwalls and later-floor restarts. Audio regressions add bounded archive/sample decoding, deterministic IMF and
shared OPL scheduling, all original music loop durations, volume/priority/position,
RNG independence, both boss delay modes and repeated SDL dummy lifecycle. Finale
SDL tests also initialise sound. Save tests cover all six slots, process restart,
portable CRC/field decoding, corrupt inputs, source identity, failed/competing
writes, deterministic actor/door/pushwall/projectile/boss continuation, audio
restoration and SDL menu input. Controls regressions add Classic/Modern input,
diagonal bounds, mouse sensitivity/buttons/capture, focus transitions, binding
conflicts, settings and save/RNG equivalence. See [Milestone 6C](MILESTONE6C.md).
Display regressions add full-output black-bar readback, integer/fit geometry,
tiny windows, drawable dimensions, external/fullscreen state changes, shortcut
consumption, injected SDL failures, capture/focus and display-independent saves.

From CART, regression checks can use the current executable by absolute path:

```sh
CART=/absolute/path/to/wolf3d-revive/build/tools/cart sh tests/test_sdl3.sh
CART=/absolute/path/to/wolf3d-revive/build/tools/cart sh tests/test_desktop_dirs.sh
```

See [milestone 6 results and scope](MILESTONE6.md) for validation and remaining work.

Classic regression coverage and DOS comparison limits are recorded in
[CLASSIC_FIDELITY_RESTORATION.md](CLASSIC_FIDELITY_RESTORATION.md).

Preferences use `SDL_GetPrefPath(NULL,"wolf3d-revive")/config.toml`, beside but
separate from save slots: `$XDG_DATA_HOME/wolf3d-revive/` or normally
`~/.local/share/wolf3d-revive/` on Linux, `%APPDATA%\wolf3d-revive\` on Windows.
See [MILESTONE6F.md](MILESTONE6F.md) for the actual schema/defaults, two preset
profiles, fixed TOML subset, atomic writes and verified limitations. Malformed,
unreadable or future-version files are preserved; fix or rename them to restore
normal-exit preference saving. Check-only commands do not touch configuration.
The configuration tests use Python 3.11+ for the independent standard-library
`tomllib` output check; the native application has no Python dependency.


## Native visual settings (Milestones 7–8)

Control → Display selects Original / High Resolution, fixed or Auto desktop
resolution, Auto / 4:3 / 16:9 / 16:10 / 21:9 / 32:9 aspect, VSync, Integer / Fit
and Nearest / Linear presentation filtering. Smooth Rendering enables
read-only interpolation in High Resolution and automatically follows the active
display's refresh, with a 60 FPS fallback and 240 FPS ceiling. There is no
manual FPS selection. Original remains the default.
Alt+Enter and F11 retain their existing functions; F12 writes a BMP in the
SDL preference directory. No image or game-data files belong in the repository.

The existing CART command builds the native renderer with C11/SDL3 and libm:

```sh
./build/tools/cart build --profile cart-sdl
WOLF3D_DATA="/path/to/owned/WL6" python3 -m unittest discover -s tests -v
SANITIZE=1 UBSAN_OPTIONS=halt_on_error=1 WOLF3D_DATA="/path/to/owned/WL6" \
  python3 -m unittest discover -s tests -v
```

`test_render.py` covers target boundaries, aspect helpers, native geometry,
Original hashes, HUD composition, SDL switching, preference restoration,
screenshots and audio render stalls. `test_demo_sync.py` additionally renders
widescreen during all four DOS-reference recordings. Test harnesses compile
with `-Werror` as well as strict C11 conversion/shadow/pedantic warnings.
See [MILESTONE7_VISUAL.md](MILESTONE7_VISUAL.md) for schema, measurements,
Windows/Wine checks and verification limits. Sanitizer application tests should
set `WOLF3D_BINARY` to a separately built sanitized full executable.

See [MILESTONE8.md](MILESTONE8.md) for fixed-step debt semantics, VSync pacing,
interpolation boundary tests, end-to-end benchmarking commands and measured
performance/hardware limitations. `test_presentation.py` verifies immutable
snapshots and identical simulation saves across all eight requested rates.

## Milestone 12 DOS interaction

Pause displays the original pause picture during living gameplay; a fresh key
or click resumes. Pause in menus toggles music. New Game confirms replacing an
active game after episode selection. Quit uses all nine original sayings.
When mouse input is enabled, relative menu movements over 60 counts navigate,
left-click accepts and right-click cancels; Y/N confirmations use the keyboard.

Final death and episode endings qualify the score by points, then floor reached.
Enter saves the typed name; Escape leaves it blank while keeping the score.
Scores persist separately as `highscores.w3h` in the SDL preference directory.
Invalid/future score files are preserved; session rankings still work.

Hold M during startup, press M at sign-on, or use `--jukebox` for Robert's
Jukebox. M+L+I grants the original inventory while zeroing score and adding ten
minutes; B+A+T displays the Keen Easter egg. `--goobers` permits the original
Backspace+LeftShift+Alt debug unlock, followed by Tab+G/H/I/E/F/C/P/Q/W/X.
See [the fidelity checklist](MILESTONE12_DOS_FIDELITY_BACKLOG.md) for exact
supported scope, intentional native systems and outstanding comparisons.

## Transition inventory (Milestone 13)

The source/evidence audit needs only Python and the checkout, without game data:

```sh
python3 scripts/audit_transitions.py --check
python3 -O scripts/audit_transitions.py --check
```

After reviewing a source/transition change, update
`docs/dos_state_transitions.json`, then regenerate the review document with
`python3 scripts/audit_transitions.py --write`. Missing/stale/invalid inventory
files fail the check and the standard unittest suite. This check validates
traceability and route coverage, not runtime DOS equivalence. Current executed
DOS and release results are in [Milestone 13](MILESTONE13_FINAL_FIDELITY.md).

Menu mouse selection follows the visible pointer. Letterbox borders and the
320×200-to-320×240 aspect correction are included in the hit testing, so the
highlight stays on the pointed row at different window sizes and scaling modes.
Clicks in black borders or disabled rows do not activate another selection.
Keyboard navigation retains control until fresh pointer movement or a click;
gameplay continues to use captured relative mouse movement.
