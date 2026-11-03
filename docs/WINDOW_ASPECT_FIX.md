# Independent window, DOS menu and gameplay aspects

**Branch:** `fix/window-aspect-independent-controls`

The SDL window stays at the user's chosen dimensions. Entering or leaving
menus changes only the rendered presentation.

## Behaviour

- DOS menus, startup pages and static game screens retain the original
  320x200 indexed framebuffer. Presenting that image on a centred 320x240
  logical canvas corrects DOS pixel aspect to 4:3. Unused output is black.
  Artwork, fonts, buttons and the pistol cursor stay together in that image.
- Menu scaling selects Fit or Integer. Tiny windows fall back to Fit rather
  than cropping. Enhanced gameplay always fits its selected aspect into the
  available output, independently of integer menu scaling.
- Gameplay aspect selects Auto, 4:3, 16:9, 16:10, 21:9 or 32:9. Auto uses
  drawable pixels (including DPI scaling), within the existing 4:3–32:9 range.
  Aspect choices do not resize the window or select an internal resolution.
- Display settings opens directly from the classic Control screen. The
  Customize controls route remains available. Escape returns to the Control
  page from which Display was opened. Arrow keys and Enter navigate settings.
- Display labels distinguish Fullscreen, Menu scaling, Window size, 3D
  renderer, Internal 3D size, Gameplay aspect, VSync, Texture filter and Smooth
  rendering. The internal Auto option retains its existing desktop-derived
  1920x1080 ceiling; it does not select the SDL window size.

Window size cycles in either direction, wrapping through these presets:

| Aspect | Window dimensions (SDL window points) |
| --- | --- |
| 4:3 | 640x480, 960x720, 1280x960 |
| 16:10 | 960x600, 1280x800, 1600x1000, 1920x1200 |
| 16:9 | 960x540, 1280x720, 1600x900, 1920x1080 |
| Ultrawide | 1680x720, 2560x1080, 1920x540, 2560x720 |

Arbitrary dimensions set by the window manager remain supported. From an
unlisted size, Right selects the first preset and Left selects the last.
Preferences save explicit width and height separately from rendering settings.
Fullscreen retains the windowed dimensions and restores them on exit; changing
Window size while fullscreen selects the dimensions for the next windowed view.

The existing asynchronous SDL request handling remains authoritative. A
rejected operation rolls back its settings. A request not applied within three
seconds reports an error, releases the pending state and adopts the actual
windowed size. Window-manager limits can therefore reduce a requested preset;
the preset list does not promise every size fits the current monitor. Minimized,
maximized and fullscreen dimensions do not replace the remembered normal size.

## Review and changes

The existing implementation was reviewed against `master`, including
`WL_DISPLAY.c`, `WL_CONTROLS_MENU.c`, `sdl_video.c`, `WL_MAIN.c`, preference
persistence and the display/DOS menu tests. It was extended rather than replaced.

- `WL_CONTROLS_MENU.c`: direct Display entry with original menu artwork,
  font, colours and cursor styling; updated DOS composition oracle.
- `WL_DISPLAY.c`: independent window presets, selection reset on opening and
  clearer labels that fit the original font layout.
- `sdl_video.c`: exclude enhanced world presentation from integer menu scaling.
  Previously this setting could leave enhanced gameplay unnecessarily small.
- `display_tests.c` / `test_display.py`: all 15 presets in both directions,
  wrapping and arbitrary-size recovery; fit/integer geometry and black-border
  readback; explicit aspects and Auto; resize-event and dimension assertions
  across game → menu → Control → Display → Control → menu → game; fullscreen
  restoration and manual resizing. The old 4:3-only test expectation was fixed.
- `test_config.py`: save/restart restoration of every preset and an arbitrary
  size, retaining independent gameplay aspect and menu scaling preferences.

No simulation, demo decoding, HUD rendering or Milestone 9 performance
optimisation code was changed. `WL_MAIN.c` already restores the correct Control
page after Display; no additional transition code was necessary.

## Validation

Checks used GCC 14.2 with strict C11, `-Wall -Wextra -Wpedantic
-Wconversion -Wshadow -Werror`. Linux builds used the local SDL3 3.5.0
development SDK; MinGW used SDL3 3.4.18.

| Check | Result |
| --- | --- |
| Strict Linux application and SDL test build | Passed |
| CART `cart-sdl` build | Passed |
| Strict MinGW application and SDL display test build | Passed |
| Windows display tests under Wine, owned WL6, dummy/software SDL | Passed: presets, navigation, gameplay isolation and aspect transitions |
| Full Python test suite with owned WL6 data | Passed: 126 tests, no skips (626.324 s) |
| Full ASan/UBSan suite with owned WL6 data | Passed: 126 tests across 12 modules, no skips or findings (588.040 s) |
| Native X11/OpenGL presentation and transition checks | Passed, subject to the limits below |

The full suites include original DOS demo traces (all four demos, 5386 commands,
classic and enhanced rendering), DOS menu pixel comparisons, native rendering
hashes, control binding conflicts, deterministic save/load and audio isolation.
Sanitizer test modules run in four independent subprocesses with
`SANITIZE=1`, `CC='gcc -O1'` and `UBSAN_OPTIONS=halt_on_error=1`; the application
binary also uses AddressSanitizer and UndefinedBehaviorSanitizer. Each module's
exit status and sanitizer output are checked.

The normal suite can be reproduced after building the application:

```sh
WOLF3D_BINARY="$PWD/build/checks/window-aspect/wolf3d-revive" \
WOLF3D_DATA="/path/to/owned/WL6" CC='gcc -O1' \
python3 -m unittest discover -s tests -v
```

`tests/display_tests.c` also accepts the `transitions` mode with a data directory
and writable save directory to exercise native SDL window transitions. The
Python runner selects dummy/software SDL for reproducible exact preset sizes.

### Native display and limits

Linux SDL3 X11/OpenGL checks ran on a physical 1920x1200 panel at approximately
60 Hz. For Control, Display, intermission, victory and end-text captures, every
output pixel matched the original indexed framebuffer enlarged with 5x6 texels,
with 160-pixel black borders on each side. Display label widths were checked
against the value column and the captured layout was visually inspected.

Native transition checks passed for 960x720, 960x540, 960x600, 1680x720 and
1001x601, with explicit gameplay aspects and Auto. They observed zero SDL window
resize events during menu/game transitions, retained integer menu scaling,
restored fullscreen/windowed sizes and adopted manual window resizes.
A 2560x720 request was clamped to 1918x720 by the window manager; the timeout
path reported the limit and adopted that size. The same transition checks then
passed at the actual size.

Physical 32:9, high-DPI/mixed-DPI, multi-monitor and native Windows display
validation were unavailable. Exact large sizes and 32:9 are covered by SDL's
software/dummy backend; Wine checks do not establish native Windows display
behaviour. No claim is made that oversized windows fit this panel. Commercial
game data and captures remain external or in ignored `build/checks` paths.
