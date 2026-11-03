# Milestone 10: classic DOS menus and startup presentation

Implemented and locally verified on 2026-11-03 (Australia/Adelaide).
The original menu artwork is loaded from owned WL6 archives at runtime.
No commercial graphics, captures or replacement artwork are committed.
The root README, original sources, save/configuration formats, control bindings,
70 Hz simulation, demo command timing and Milestone 9 world renderer are retained.

## Original reference and verified assets

Reviewed the unchanged original implementations:

- `WOLFSRC/WL_MENU.C`: `DrawCtlScreen`, `DrawSoundMenu`, `DrawMenu`,
  `DrawMenuGun`, `DrawMouseSens`, panel outlines and cursor timing.
- `WOLFSRC/WL_MENU.H`: Control and Sound coordinates and panel dimensions.
- `WOLFSRC/WL_INTER.C`: `PG13` composition and seven-second input wait.
- `WOLFSRC/WL_MAIN.C`: startup/sign-on and title/credits/scores/demo sequence.

The owned Steam WL6 archive successfully decoded every control-panel picture
(slot 10 through 42), the title, credits, high-score heading and rating graphic.
The existing WL6/GOODTIMES picture mapping and main-menu edition layout remain
in use; no artwork or menu rows from other editions are fabricated.

| Archive artwork | Size in source pixels |
| --- | --- |
| `C_CONTROLPIC` | 144×48 |
| `C_CUSTOMIZEPIC` | 152×48 |
| `C_CURSOR1PIC`, `C_CURSOR2PIC` | 24×16 each |
| `C_NOTSELECTEDPIC`, `C_SELECTEDPIC` | 24×8 each |
| Sound Effects, Digitized Sound, Music headings | 136×16 each |
| `C_MOUSELBACKPIC` instruction legend | 104×16 |
| `PG13PIC` (slot 88, artwork labelled **PC-13**) | 88×64 |
| Title and credits | 320×200 each |

All palette indices and VGA palette expansion remain original. Validation uses
independent source-composition pixel oracles and locally inspected captures
from the owned archives. No independent DOS-emulator screenshot comparison is
claimed.

## Control menu and modern options

The first Control page follows the original source composition:

- Red background, stripes at y=10, original heading at (80,0), instruction
  legend at (112,184), dark-red panel at (16,65), size 284×84.
- Font 1 at x=80, y=70+13×row, original normal/highlight/disabled palette colours.
- Pistol at x=24, y=68+13×row; original toggle graphics at x=56, y=73+13×row.
- Original six labels: Mouse Enabled, Joystick Enabled, Use joystick port 2,
  Gravis GamePad Enabled, Mouse Sensitivity and Customize controls.

Unsupported joystick rows remain dark red and navigation skips them. Mouse
Sensitivity is disabled while the current profile's mouse is off. Its separate
DOS-styled slider retains Revive's existing 1..40 sensitivity range.

Customize controls opens a subordinate page using the original Customize
heading, fonts, pistol and instruction legend. Classic/Modern preset selection,
per-profile mouse settings, inversion, defaults, key bindings and Display
settings remain available there. Binding capture, conflict detection, reserved
keys and alternate bindings retain their existing behaviour. Display retains
all existing functionality and now also uses a pistol selector and the original
legend. Returning from Display restores the Customize page rather than losing
the user's place at the root Control menu.

The initial branch's cursor fix was validated, then extended to the restored
root/subordinate pages. Control and Sound share the existing tic-driven pistol
animation, including both archived frames, original greater-than-8 /
greater-than-70 tick blink thresholds and bounded coarse-clock catch-up. Moving
selection restores the first pistol frame. Held up/down navigation on the
Control, Customize and Sound pages repeats at the original 20-tick interval;
binding capture continues to consume physical keys without navigating. Menu move/select/back feedback uses
the existing original sounds without advancing gameplay or gameplay RNG.

## Sound menu

The original three headings and panel proportions are retained:

| Section | Panel (x,y,width,height) | Heading draw position |
| --- | --- | --- |
| Sound Effects | (40,17,250,45) | (100,0) |
| Digitized Sound | (40,82,250,45) | (100,65) |
| Music | (40,147,250,32) | (100,130) |

Buffered picture x positions retain the original eight-pixel alignment.
The original fonts, red/yellow toggle art, pistol and bottom legend are used.
The five selectable rows are Effects None, AdLib effects, Adjust volumes,
Music None and AdLib music. Volume adjustment opens a separate DOS-styled page
with the same existing 0..100%, five-point adjustment steps.

Revive has one effects enable/volume pair covering synthesis and digitized
samples. Consequently, the digitized section reports its actual playback
status as **With sound effects**, with a toggle-status graphic and explanatory
text. It is a status panel, not a second device selector or newly persisted
mode. No PC speaker, Disney Sound Source or nonfunctional physical sound-device
options are offered. The existing `SD_DigitizedMode` behaviour is preserved.
Music/effects enables, volumes and both control profiles use the unchanged
configuration writer and schema.

## Startup and attract sequence

Normal application startup now shows the verified PC-13 artwork using the
original `PG13` composition: palette index 0x82 fills the 320×200 framebuffer,
and the 88×64 archived picture is drawn at (216,110). It fades in, waits seven
seconds in 70 Hz time, then fades to the title; a key skips the wait to the
title. The rating appears once at startup, not on every attract-loop cycle.
`--menu` and `--level` continue to enter their requested screens directly.

An absent picture or nonmatching dimensions leaves the existing title startup
in place. A present but malformed graphic reports a loading error. There is no
substitute/generated commercial artwork. `CA_GrChunkPresent` provides a
non-mutating presence check so optional sparse graphics do not become mandatory.

The original `SignonScreen` uses `introscn`, an image linked into the DOS
executable, rather than a WL6 graphics-archive picture. It is outside this
archive-backed restoration. No separate SETUP.EXE screen, device-detection
screen or executable-linked image is reconstructed. The existing title,
credits, high scores and four recorded demos retain their order and timings;
the rating runs as UI time without consuming simulation ticks or demo commands.

## Fixed 4:3 presentation

The existing SDL3 logical-presentation mechanism is retained. A shared
read-only predicate now excludes intermission and victory/end-text pages from
world presentation even while they remain inside `UI_GAME`; previously those
static pages inherited the enhanced world's logical canvas. Every menu, dialog, rating, title, credits and static interface
screen uses the **320×200 indexed framebuffer**, presented as **320×240 logical
pixels** with centred aspect-preserving letterboxing. Artwork coordinates
never depend on monitor width.

On the native 1920×1200 panel, SDL reported a **1600×1200** menu rectangle at
**(160,0)**. Fullscreen X11/SDL OpenGL readbacks compared every physical output pixel for
Control, intermission, victory and end-text screens: original source pixels
occupy exactly 5×6 physical pixels each and both 160-pixel side borders are
opaque black. The actual full-output Control capture was inspected. Readback redraws before
capture because an accelerated backend's post-present back buffer is undefined.

Synthetic tests cover 1920×1200, 1920×1080, 3440×1440 and 3840×1080 drawables,
black side borders, logical 320×240 dimensions, fullscreen/windowed changes,
menus entered during enhanced gameplay, and return to the configured 16:9 world.
The existing display suite also covers integer/Fit scaling, tiny
windows, filtering, display-setting failures and gameplay isolation. Enhanced
world presentation remains restricted to game/demo states; UI screens use fixed 4:3 presentation, including while a game is paused behind them.
No SDL texture, backend, shader or world-rendering policy was added.

## Regression results

| Check | Result |
| --- | --- |
| Strict GCC C11 `-O2`, pedantic/conversion/shadow warnings, `-Werror` | Full application and native diagnostics passed |
| CART `cart-sdl` build | Passed |
| Complete Linux suite, owned WL6 | **123 passed**, 461.671 s; no skips |
| Final ASan/UBSan verification, all 12 modules and instrumented app | **123 passed**, 358.230 s with four parallel processes; every process exited 0, no findings or skips |
| Final static-page/menu ASan/UBSan regressions after presentation correction | **3 passed**, 25.877 s |
| Final menu/input/sound regressions, including held repeat | **10 passed** normally (38.663 s) and **10 passed** under ASan/UBSan (75.430 s) |
| Focused display/menu regressions | **3 passed**, 15.167 s |
| Independent indexed-pixel Control, adapted Sound and rating composition | Exact reference-buffer comparisons passed |
| Original Classic framebuffer oracle | Exact reference hashes retained |
| Milestone 9 native renderer oracle | All **1,500** hashes retained |
| All four original DOS recordings | **5,386/5,386 commands match**, Classic and interpolated native paths, normal and sanitizers |
| Native 1920×1200 fullscreen Control/intermission/victory/end text | Every output pixel matches source DOS pixels at 5×6 physical pixels each; both 160-pixel borders black |
| MinGW x86_64 strict C11 full app and new menu harness | Passed, SDL3 3.4.18 SDK |
| Wine owned-data menu/persistence/rating/static-page/presentation tests | Passed |
| Final diff | Clean; README, WOLFSRC, simulation, frame clock, native world renderer and save/configuration implementations unchanged |

The full normal suite was rerun after the static-page correction. The final
held-repeat adjustment was followed by the ten focused menu/input/sound tests
in both normal and sanitizer builds. No world-renderer, simulation or save-format
changes were needed.

An earlier serial sanitizer log reported 123 tests and `OK`, but its command
wrapper returned termination status 143 when collected. For unambiguous final
results, all twelve test modules were rerun on the final presentation code in
four parallel child processes, recording each actual exit status. Every module
exited 0; `sanitizers-final/summary.json` records all 123 tests and no findings.
The subsequent held-repeat change is covered by the final ten-test sanitizer
run above.

The new `tests/dos_menu_tests.c` independently decodes archived planar pictures
and original font glyphs into reference buffers. It compares all 64,000 indexed
pixels for the source-authentic Control composition, both pistol frames and
mouse-enabled/disabled states; the adapted Sound composition; and PC-13 artwork.
It also exercises disabled-row navigation, sensitivity/preset/inversion changes,
volume/enables and configuration round trips, missing or wrong-size optional
rating art, fine/coarse cursor timing, held-arrow repeat, input dismissal, paused-game save-payload
isolation and the 4:3-to-widescreen round trip. Synthetic fixtures contain solid
colours and generated glyph patterns, not copies of original artwork.

Raw logs and all captures stay in ignored `build/checks/m10/`. Relevant logs:
`final-suite.log`, `sanitizers-final.log`, `sanitizers-final/*.json`,
`menu-input-final.log`, `menu-input-final-asan.log`, `static-pages-asan.log`,
`display.log`, `native-pages.log`, `wine-input-final.log` and `cart-final.log`. Capture inspection
includes main menu, Control on/off, Customize, Sound, Volume, title, credits,
PC-13 and a full-output native 1920×1200 Control menu. Commercial captures must
not be added to commits or PR attachments.

## Reproduction

```sh
export WOLF3D_DATA="$HOME/.steam/steam/steamapps/common/Wolfenstein 3D/base"
export SDL_SDK=../cart/vendor/SDL
mkdir -p build/checks/m10

gcc -std=c11 -O2 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror \
  -Isrc -I"$SDL_SDK/include" src/main.c src/engine/*.c src/platform/*.c \
  "$SDL_SDK/build/libSDL3.a" -pthread -lm -o build/checks/m10/wolf3d-revive
WOLF3D_BINARY="$PWD/build/checks/m10/wolf3d-revive" \
  python3 -m unittest discover -s tests -v

# Instrument the app as well as the harnesses; the SDL SDK is not instrumented.
gcc -std=c11 -O1 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror \
  -fsanitize=address,undefined -fno-omit-frame-pointer -g \
  -Isrc -I"$SDL_SDK/include" src/main.c src/engine/*.c src/platform/*.c \
  "$SDL_SDK/build/libSDL3.a" -pthread -lm -o build/checks/m10/wolf3d-revive-asan
SANITIZE=1 CC="gcc -O1" UBSAN_OPTIONS=halt_on_error=1 \
  WOLF3D_BINARY="$PWD/build/checks/m10/wolf3d-revive-asan" \
  python3 -m unittest discover -s tests -v
```

Physical Windows, ultrawide panels, high-DPI scaling and other fullscreen
compositors remain hardware checks. Wine and synthetic drawable dimensions
verify API behaviour rather than those physical environments. No new physical
audio-device measurements or complete campaign playthrough are claimed.
