# Milestone 7: native high resolution and widescreen

Implemented on `feature/7b-visual-foundation` for draft PR #10. Original remains
both the initial preference and the authoritative DOS gameplay renderer. There
is no separate Enhanced gameplay mode. No ECWolf source, replacement artwork,
commercial archives or captured commercial images are included.

## Reference review

Reviewed ECWolf's current upstream tree at
[`1bff92d43bc1c1b6e9c4c03f0fd3c7ae9c972f9e`](https://github.com/ECWolfEngine/ECWolf/tree/1bff92d43bc1c1b6e9c4c03f0fd3c7ae9c972f9e):
`sdlvideo.cpp` (`ScaleWithAspect`, logical presentation, `SetVSync`),
`v_video.cpp` (aspect correction), `wl_draw.cpp` (projection/ray traversal),
`wl_menu.cpp` (aspect and video selections), `r_2d/r_main.cpp` and
`g_wolf/wolf_sbar.cpp` (interface coordinates and status artwork). The useful
behaviours are separate world/UI geometry, horizontal expansion and explicit
video preferences. This implementation was written independently in C11 using
this port's existing ray intersections and SDL3 pipeline; ECWolf code was not
copied and its C++/modding framework is not a dependency.

## Renderer and gameplay boundary

`WL_RENDER_TARGET` owns no allocations. Every operation validates its pixels,
capacity, stride and bounded dimensions, including overflow and descriptor
corruption. Failed initialization preserves the previous descriptor. Native
scene rendering accepts 320..4096 by 200..2160 targets and caller-owned per-column
depth storage. The three fixed menu presets are 640×480, 1280×720 and 1920×1080.

`WL_DRAW` shares its existing wall/door/pushwall intersection routine between
Classic and the native pass. Classic calls it with visibility marking enabled;
the additional pass cannot mark `spotvis`. Each native viewport column casts
its own ray. Perspective wall heights and sprite footprints use the native
projection; they are not enlarged Classic pixels. Original 64×64 wall samples,
sprite transparency posts (including opaque palette index zero), texture flips,
door leading-edge offsets, pushwall offsets and palette indices are retained.
Sprites sort stably far-to-near and test each column against wall distance.
Weapons retain their original two-half sprite anchoring and pixel aspect.

The original 320×200 pass still runs at its existing points in simulation. That
is necessary because DOS rendering determines visibility, actor activation and
some pickups. The native pass reads actors, using local copies for original
rotation calculations, and changes no inventory, RNG, commands, clock, AI or
save state. Widescreen-visible actors still follow DOS activation rules.

A presentation-only render serial invalidates cached native images after a
Classic redraw, including save/load at an identical game clock/framebuffer.
It is neither simulated nor serialized. Unchanged presentation is cached.
Targets, RGBA conversion storage, depth columns and SDL textures are reused.
Allocations occur on mode/resolution changes, not on ordinary frames. Invalid
settings and failed texture/allocation, filtering, VSync or fullscreen changes
retain the previous working settings. Automatic desktop changes stage a new
surface before releasing the old one; allocation failure keeps the old surface
and retries at most once per second.

## Aspect, HUD and presentation

`WL_AspectFit` fits with 64-bit products and commits its output only on success.
FOV validation rejects NaN, infinities, zero and unrepresentable results. The
original MinGW `isfinite` macro caused a strict conversion warning; bounded
ordered comparisons reject those values portably without narrowing doubles.

High Resolution fits the selected aspect into its internal canvas, crops that
content rectangle for SDL presentation and letterboxes the physical output.
Auto aspect uses the current drawable dimensions, clamped to 4:3..32:9. Explicit
choices are 4:3, 16:9, 16:10, 21:9 and 32:9. Portrait/narrow windows letterbox.

Vertical framing is fixed. Native focal length is based on corrected scene
height, the original viewport fraction and `(FOCALLENGTH+MINDIST)`. Width adds
rays on either side of the centre. For reference horizontal FOV `f`:

```
wide_fov = 2 * atan(tan(f / 2) * (aspect / (4 / 3)))
```

Tests compare exact central viewport pixels at equal heights across different
widths, independently of original HUD pixels. The original view sizes 4–19 are
retained; Change View still changes the world rectangle. The teal border and
bevel frame the expanded rectangle. The original status bar remains centred in
an independently aspect-correct 4:3 region; its outer areas are black. Health,
face, ammo, keys and weapon graphics retain their original positions/artwork.
Menus always use the original framebuffer through 320×240 logical presentation.

Nearest texture sampling is fixed in the indexed renderer. The optional Linear
filter affects only final SDL texture presentation. Integer scaling uses whole
internal pixels for the active surface, falling back to aspect-preserving Fit
when the drawable is smaller than one surface. Original mode retains its
320×240 integer/fit behaviour. No OpenGL/Vulkan/shaders or alternate GUI is added.

Resizable windows, borderless desktop fullscreen and asynchronous window-size
restoration retain the previous SDL state machine. Pixel sizes come from
`SDL_GetRenderOutputSize`, not window point dimensions. Auto resolution uses the
window's desktop display mode, preserving its ratio while bounding allocation
to 4096×2160. Fixed resolution remains independent of window size and DPI.

## Display menu and configuration

Control → Display has nine original-font rows: Fullscreen, Scaling, Window size,
Rendering, 3D resolution, Aspect ratio, VSync, Filter and Reset defaults. Arrows
and Enter change choices without restarting. Reset returns to windowed 960×720,
Fit, Original, fixed 640×480, Auto aspect, VSync off and Nearest.

The additive schema remains version 1. Existing files without new keys retain
the defaults. Visual keys live in `[display]` alongside existing window options:

```toml
[display]
fullscreen = false
integer_scaling = false
window_width = 960
window_height = 720
renderer = "original"       # or "high_resolution"
render_width = 640          # fixed pairs: 640x480, 1280x720, 1920x1080
render_height = 480
render_auto = false         # true selects bounded desktop resolution
aspect_ratio = "auto"       # "4:3", "16:9", "16:10", "21:9", "32:9"
vsync = false
filter = "nearest"          # or "linear", final presentation only
```

Unknown renderer/aspect/filter strings, invalid resolution pairs and wrong
value types fall back predictably. The existing atomic preference writer,
unsupported-file preservation, two control profiles and game-save format are
unchanged. An unsupported backend VSync request is reported and rejected.

Alt+Enter switches fullscreen; F11 switches scaling. F12 captures an actual SDL
rendered BMP into `SDL_GetPrefPath(NULL,"wolf3d-revive")`, beside `config.toml`:
`~/.local/share/wolf3d-revive/` (or `$XDG_DATA_HOME`) on Linux and
`%APPDATA%\wolf3d-revive\` on Windows. Names include monotonic nanoseconds and
an existence check. Key repeats and key-up are consumed, focus loss is respected,
and F12 is reserved from custom gameplay bindings. Capture errors are reported
without ending gameplay. These locally generated images may contain commercial
art; do not commit/distribute them.

## Audio investigation

Compared `origin/master` with PR #9's branch
`origin/feature/audio-render-stall-buffer`: its current diff contains only
`docs/AUDIO_RENDER_STALL.md`. It describes an 80 ms experiment but makes **no
code change**. PR #9 was not merged. This branch tests the proposed buffer sizes
independently with test-only queue controls.

Master fills 384-frame chunks to 24–32 ms on the main thread. A real ALSA/pulse
output here has 1024 frames at 48 kHz: **21.33 ms per device block**. A 24 ms
queue barely covers one block. Servicing between expensive render batches alone
reduced the user's crackling but did not eliminate it.

The producer now primes before resuming the device and fills to the greater of
80 ms or two device periods plus 8 ms, with a 250 ms bound and silent failure for
unsupported device latency. Its 8 ms chunks can overshoot the target by less
than 8 ms. Synthesis uses the existing sample-based rational IMF clock, never
frame deltas. Rendering services the same main-thread producer between wall,
sprite, UI and conversion batches and before/after drawing. No audio thread,
sound-command reordering, RNG changes or extra simulation ticks were added.

A dummy-device comparison injected a 60 ms blocking stall: the 24 ms policy had
0 queued bytes afterwards; the 80 ms policy retained 3072 bytes (16 ms). Three
1920×1080 frames with deliberately delayed render batches made 810 service calls
with **zero observed empty queues**; measured maximum service gap was about
3.2 ms in the final focused run. The user then listened through the real SDL
output at 1920×1080 and reported **“Sounds normal now.”**

Latency cost: application queue targets are measured in input PCM frames:
24 ms before versus 80 ms here, approximately **56 ms extra queued output** at
this device period. These are application-buffer measurements, not microphone
or hardware end-to-end latency measurements. SDL/OS/device buffering adds delay.
Arbitrarily long OS/driver freezes can still underrun; this bounded main-thread
producer is not a hard real-time guarantee.

## Verification and measurements

Owned WL6 data was read from the local Steam installation. Automated fixtures
are synthetic; only hashes/trace summaries are tracked. No original archives,
reference source modifications or commercial captures are part of this change.

| Check | Result |
| --- | --- |
| CART C11 SDL build | Passed |
| Full normal owned-data regression suite, after death correction | 115 passed, 461.480 s |
| Focused final visual suite (including all 60 maps) | 5 passed, 18.465 s |
| Full ASan/UBSan owned-data suite, initial Milestone 7 baseline | 115 passed, 992.864 s; no sanitizer diagnostics |
| Death correction ASan/UBSan focused visual suite | 5 passed, 72.641 s; no sanitizer diagnostics |
| CART SDL3 and desktop-directory regressions | Passed |
| Strict GCC C11 (`-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`) | Passed |
| Strict MinGW full app, aspect, renderer and demo-trace cross-links, SDL3 3.4.18 | Passed |
| Wine dummy/software renderer, settings/restarts, screenshots, failures and audio stress | Passed |
| Wine widescreen demo hashes | All 5,386 match DOS reference |
| Wine application `--check-level` | Passed |
| Native Linux X11/software fullscreen, window restoration, relative capture, focus and mode changes | Passed |
| Real SDL audio listening, High Resolution 1920×1080 | User confirmed normal playback |

Focused coverage:

- Target boundaries/overflow/padding/corrupted descriptors; aspect/FOV extremes.
- All 16 original view sizes at each fixed resolution, exact HUD composition,
  extra geometry, depth sorting, transparency, opaque zero, clipping and weapons.
- Additional aspect/view combinations at 3440×1440, 3840×1080 and 4096×2160.
- All 60 owned maps rendered natively at 640×480 / 16:9 in four cardinal
  directions, checking unchanged original framebuffer and RNG.
- Original framebuffer hashes: 32 captures, views 4–19 initially and at tic 60,
  captured from the pre-modern renderer and unchanged by these changes.
- All four owned DOS recordings with a native widescreen render after every
  command, including interspersed 1280×720 and 1920×1080 passes. Expected traces
  remain the existing 6G reference (default view 15, silent reference settings).
- Player/actor/visibility/RNG/clock and serialized save payload isolation.
- SDL resize/tiny windows, fullscreen return, Auto desktop, mode switching,
  three full SDL restarts, preference round trips, filter/VSync failures,
  allocation/texture rollback, F12 capture/repeat consumption and audio stalls.
- Native X11/software 1280×720 captures of widescreen gameplay and the Display
  menu inspected locally. Actual 1920×1080 SDL audio listening confirmed by user.

Owned-E1M1 benchmark (`tests/render_benchmark.c`), view 19, 20 native scene passes, GCC `-O2`, while
other regressions ran; **excludes SDL upload, presentation and simulation**:

| Canvas | Mean scene ms | Persistent CPU indexed + RGBA bytes |
| --- | ---: | ---: |
| 640×480 | 1.71 | 1,536,000 |
| 1280×720 | 4.49 | 4,608,000 |
| 1920×1080 | 9.43 | 10,368,000 |
| 3440×1440 | 20.74 | 24,768,000 |
| 3840×1080 | 17.57 | 20,736,000 |
| 4096×2160 | 79.51 | 44,236,800 |

Repeat the scene benchmark with your own data and local SDL SDK:

```sh
gcc -std=c11 -O2 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror \
  -Isrc -I../cart/vendor/SDL/include tests/render_benchmark.c \
  src/engine/*.c src/platform/data_file.c src/platform/save_file.c \
  ../cart/vendor/SDL/build/libSDL3.a -pthread -lm -o build/checks/render-benchmark
build/checks/render-benchmark "$WOLF3D_DATA"
```

These measurements are not frame-rate guarantees. The normal CART profile has
no optimization flag; the strict unoptimized matrix measured roughly 3–17 ms
at 640×480..1920×1080 and 71–104 ms at 4096×2160, depending on aspect/view/scene.
Depth storage adds 16 KiB; SDL texture/driver allocations add further memory
(approximately another four bytes per canvas pixel for a texture). Replacement
can briefly hold two surfaces. No per-frame surface allocation is required.

Measured full-application peak RSS using `os.wait4` on Linux, SDL dummy/software,
1280×720 window, 200 E1M1 presentation frames and `--no-audio`: **13,880 KiB
Original** versus **27,652 KiB High Resolution at 1920×1080**. Elapsed times
were 2.315 s and 8.711 s respectively; these include the main loop's 4 ms waits,
simulation, fades and rendering and are not a renderer-only benchmark.

## Death transition correction

The initial death fallback switched the world content to a centred 4:3 Classic
image, visibly narrowing a widescreen scene. Death now retains the same native
viewport, projection, teal border and independently centred HUD through the
turn toward the killer, red fizzle, red hold and respawn. The weapon disappears
on death as in the original renderer.

A read-only death phase/cursor query exposes the existing presentation progress.
The native renderer replays the original 17-bit polynomial up to that cursor,
then maps the mask across the entire native viewport. It never advances the
game's fizzle cursor, RNG or timer. The SDL upload cache also watches death
phase/cursor changes: a Classic source pixel already red can leave framebuffer
bytes unchanged while native fizzle coverage still needs updating.

Regression coverage checks all three fixed resolutions, all five explicit
aspects plus Auto in a 16:9 window, and every death phase through respawn. The
controlled death takes 290 original ticks in each case. Checks include native
viewport edges outside the former 4:3 world, full red coverage, stable SDL
presentation proportions, cursor-driven uploads with an unchanged Classic
framebuffer, and read-only frame/RNG/clock/cursor behaviour. Native X11 captures
of rotation, fizzle, red hold and respawn were inspected locally. Original
framebuffer hashes remain unchanged. Follow-up checks passed: full normal suite
115/115, focused visual suite 5/5 normally and under ASan/UBSan, strict CART C11,
strict MinGW full application/render harness, Wine owned-data death/Auto-aspect
cases and native X11 capture inspection. The death fix did not require changes
to the original death simulation or saved-game format.

## Verified limits and deferred work

- Physical Windows, Windows audio, real ultrawide panels, multiple-monitor
  migration and a physical DPI scale greater than 1× are not claimed. Wine and
  dummy dimensions cover API behaviour, not every OS/driver.
- VSync may be unsupported or ineffective on a backend; SDL rejection is shown.
  Linear presentation can soften original pixel artwork, by choice only.
- Scripted victory presentation uses the original composited framebuffer at
  higher display scale. Normal world geometry and death presentation are native.
- The original 0.1-degree ray tables and source-art resolution remain visible at
  high pixel counts. No antialiasing, new artwork, increased vertical FOV or
  replacement rendering API is introduced. Very high resolution is CPU costly.
- Wider visibility does not expand DOS AI activation/pickup visibility. Full
  campaign playthroughs and DOS equivalence beyond established 6G traces/view
  settings are not newly claimed.
- No merge, push, PR #8 reopening or PR #9 merge was performed. PR #10 remains
  draft. `master`, `WOLFSRC/` and original archives remain unchanged.
