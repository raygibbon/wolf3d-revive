# Milestone 9: bounded software rendering and SDL scaling

Implementation and local verification completed on 2026-11-03
(Australia/Adelaide). Physical 4K, high-refresh, multi-monitor and audio-device
validation remain hardware acceptance checks; synthetic outputs do not establish
those results. The latest master README simplification is preserved verbatim.

## Rendering policy

Auto fits the active desktop display's aspect into a canvas bounded by the
display size and **1920×1080**. SDL3 presents that indexed software-rendered
image, converted to an RGBA streaming texture, at the actual drawable size.
There is no dynamic resolution policy, GPU raycaster or additional backend.
The existing manual 640×480, 1280×720 and 1920×1080 settings remain available.

| Desktop display | Auto canvas |
| --- | --- |
| 3840×2160 / 2560×1440 / 1920×1080 | 1920×1080 |
| 1280×720 | 1280×720 |
| 1600×1200 | 1440×1080 |
| 3440×1440 | 1920×803 |
| 3840×1080 | 1920×540 |
| 1080×1920 | 607×1080 |

Auto aspect still follows drawable pixels, bounded to 4:3..32:9. Explicit aspect
choices fit into the internal canvas and SDL letterboxes the physical output.
Vertical world framing, centred original HUD artwork and palette-index-127 teal
gutters are retained. Nearest-neighbour filtering remains the default: 1080p
fills a 16:9 4K drawable at exactly 2×. Linear filtering remains optional.
Integer scaling retains its aspect-preserving Fit fallback for small windows.
Menus use the original 320×200 framebuffer with 320×240 logical presentation.

Desktop dimensions and drawable dimensions have separate purposes. Window
resizes change presentation/aspect without reallocating an unchanged canvas.
Fullscreen remains SDL desktop fullscreen. Monitor migration stages a new
surface only if the bounded internal dimensions change; 4K↔1440p needs no
replacement. Allocation/texture failures keep the old surface and retry at most
once per second. Filtering changes reuse the surface. Fixed manual resolutions
remain independent of desktop dimensions and DPI.

## Measured optimisation and pacing review

The full SDL benchmark identified rays/walls and palette conversion as the main
1080p costs, with sprite drawing a smaller repeated cost. Two portable C11 loop
changes were retained:

- Sprite texel row bounds are calculated once per shape. Wall-depth clipping
  finds visible horizontal runs once per source column; texel rectangles use
  contiguous row fills. Original half-sprite anchoring, floor rounding,
  transparency, opaque index zero and far-to-near ordering remain exact.
- Palette conversion builds 256 RGBA words from bytes, preserving byte order
  on either endian architecture. Aligned stores write one word per indexed
  pixel. The VGA six-to-eight-bit expansion and alpha 255 remain exact.

No decoded-sprite cache, per-frame engine allocation, new configuration,
renderer replacement or simulation change was needed. Wall intersections and
column sampling remain unchanged. Existing contiguous sky/floor fills and
reciprocal wall texture scaling from Milestone 8 are retained. SDL texture
uploads still copy the persistent RGBA buffer; replacing them with texture locks
was not justified by these measurements. The native image cache skips ray,
sprite, conversion and upload work for unchanged non-interpolated scenes.

The Milestone 8 scheduler was reviewed and retained. Simulation consumes
rational 70 Hz deadlines in bounded batches without dropping outstanding ticks.
Smooth presentation samples immutable snapshots independently of simulation;
demos retain their original four-tick commands. Fresh render-start deadlines
subtract rendering cost, do not replay missed presentations, and account for
actually blocking VSync without an extra monitor-period delay. Input and audio
service waits remain bounded to 4 ms. Existing tests cover fractional refresh,
60/75/90/120/144/165/180/240 Hz simulation equivalence, stalls, save equivalence,
pause/load and presentation resets. Rendering continues to pump audio between
bounded work batches. No pacing rewrite was necessary.

## Regression evidence

The native oracle was captured from the pre-optimisation branch (`835bbff`,
with master's README-only merge) before changing the renderer. It covers all
60 maps, five canvases (including 1920×803 and 1920×540), all five explicit
aspects, varied camera angles and view sizes 4/19: **1,500 exact indexed hashes**.
`tests/fixtures/native_render_hashes.csv` contains hashes only. Owned asset
identities are checked before comparing either the native or Classic oracle.
No commercial assets or captures are committed.

Additional SDL regressions simulate desktop changes through the production
Auto path. They check actual texture dimensions, creation counts, allocation
and texture rollback, retry throttling, same-size reuse and linear/nearest
selection. A 3840×2160 dummy/software capture is checked against every pixel of
the 1920×1080 indexed renderer after palette expansion and exact 2× nearest
scaling. This also checks HUD, gutters and RGBA colour reproduction end to end.
Portrait, extreme integer dimensions, invalid inputs and unchanged outputs on
failed Auto size calculations have unit coverage.

| Check | Result |
| --- | --- |
| GCC C11 `-O2`, pedantic/conversion/shadow warnings, `-Werror` | Full application and profiling harness passed |
| CART `cart-sdl` build | Passed |
| Full normal Linux suite, owned WL6 | **120 passed**, 469.055 s; no skips |
| Full ASan/UBSan suite, harnesses/application `-O1` | **120 passed**, 943.235 s; no sanitizer findings or skips |
| Final focused rendering suite after diagnostic corrections | **6 passed**, 53.109 s |
| Classic framebuffer oracle | Exact match at all 16 original view sizes |
| Native pixel oracle | **1,500/1,500 exact hashes**, normal and sanitizers |
| DOS reference demos, Classic and interpolated native | **5,386/5,386 commands match in each path**, normal and sanitizers |
| Refresh-rate/save equivalence and stall retention | Passed at all eight existing test rates |
| Save/load, configuration, keyboard/mouse, display, audio and simulation regressions | Passed in full suites |
| MinGW x86_64 full app and renderer harness, strict `-Werror` | Passed, SDL3 3.4.18 SDK |
| Wine owned-data renderer/settings/Auto/fullscreen/failure/audio tests | Passed, including exact synthetic 4K pixel check |
| Native X11/SDL OpenGL renderer, resizing/fullscreen/Auto, all 60 maps | Passed; oversized-window limitation reported explicitly |
| Native/Wine audio render stress | 888 services, zero empty queues; maximum gaps 1.98 / 2.96 ms |
| Final diff and original sources | `git diff --check` passed; `WOLFSRC/` unchanged; README identical to master |

Full logs are `suite.log`, `asan-suite.log`, `render-final.log`,
`native-verified.log`, `wine-final.log` and `cart.log` under ignored
`build/checks/m9/`. Synthetic 4K software-presentation audio observations from
the benchmark are separate from the passing bounded-render stress test; see
the measurements below.

## Reproduction

Use owned WL6 data outside the repository and a local SDL3 SDK:

```sh
export WOLF3D_DATA="$HOME/.steam/steam/steamapps/common/Wolfenstein 3D/base"
export SDL_SDK=../cart/vendor/SDL
mkdir -p build/checks/m9

gcc -std=c11 -O2 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror \
  -Isrc -I"$SDL_SDK/include" src/main.c src/engine/*.c src/platform/*.c \
  "$SDL_SDK/build/libSDL3.a" -pthread -lm -o build/checks/m9/wolf3d-revive
WOLF3D_BINARY="$PWD/build/checks/m9/wolf3d-revive" \
  python3 -m unittest discover -s tests -v

# Instrument the application as well as harnesses. SDL itself is not instrumented.
gcc -std=c11 -O1 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror \
  -fsanitize=address,undefined -fno-omit-frame-pointer -g \
  -Isrc -I"$SDL_SDK/include" src/main.c src/engine/*.c src/platform/*.c \
  "$SDL_SDK/build/libSDL3.a" -pthread -lm -o build/checks/m9/wolf3d-revive-asan
SANITIZE=1 CC="gcc -O1" UBSAN_OPTIONS=halt_on_error=1 \
  WOLF3D_BINARY="$PWD/build/checks/m9/wolf3d-revive-asan" \
  python3 -m unittest discover -s tests -v

gcc -std=c11 -O2 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror \
  -DWOLF3D_BENCHMARK -DWOLF3D_ALLOC_PROFILE \
  -Isrc -I"$SDL_SDK/include" tests/presentation_benchmark.c \
  src/engine/*.c src/platform/*.c "$SDL_SDK/build/libSDL3.a" -pthread -lm \
  -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc \
  -o build/checks/m9/presentation-benchmark
SDL_AUDIODRIVER=dummy build/checks/m9/presentation-benchmark "$WOLF3D_DATA"
M8_VSYNC=1 SDL_AUDIODRIVER=dummy \
  build/checks/m9/presentation-benchmark "$WOLF3D_DATA"
# Optional controls; neither is a player setting.
M9_NO_SMOOTH=1 SDL_AUDIODRIVER=dummy \
  build/checks/m9/presentation-benchmark "$WOLF3D_DATA"
# Diagnostic target, not a high-refresh scanout test.
SDL_AUDIODRIVER=dummy \
  build/checks/m9/presentation-benchmark "$WOLF3D_DATA" 144
```

## Before/after measurements

Host: Intel Core i5-1340P, Linux, GCC 14.2.0 C11 `-O2`, static SDL3
3.5.0 development SDK (`08b9c5539`),
owned E1M1, view 19. Each canvas runs for four seconds after at least 300 ms /
20 frames of warmup. Final harness polls events during warmup and records
actual drawable pixels. Baseline uses the same final harness and build flags,
replacing only `WL_RENDER.c` and `sdl_video.c` with their `835bbff` versions.
Copies were built with `-Isrc/engine -Isrc/platform` in addition to the commands
above. Thus this pair measures the loop changes, with the Auto cap present in
both builds. The cap itself reduces a 4K scene's software pixel count and
indexed+RGBA storage by 75% (41,472,000 to 10,368,000 bytes), independently of
these measured loop savings.

Measurements are serial, with the ASan regression suite active on other cores.
Thermals, OS scheduling and compositor load affect these short runs. FPS is
present-call throughput; it does not measure scanout or motion-to-photon
latency. CPU is process CPU time, including SDL threads; 100% is one logical
CPU. Indexed+RGBA storage is 1,536,000 / 4,608,000 / 10,368,000 bytes for the
three presets, plus SDL texture/depth/snapshot storage. SDL still allocates in
its frame path; intercepted allocation counts are in the raw logs. Engine
surfaces are retained after warmup.

X11 / SDL OpenGL presentation, 1280×720 drawable, Smooth Rendering on,
VSync off, monitor target 59.880 Hz. SDL's accelerated texture presentation
is used by the existing backend; the world renderer remains CPU-only.

| Internal | Version | FPS | Mean ms | p95 ms | p99 ms | CPU % |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| 640x480 | Before | 59.25 | 16.860 | 17.776 | 18.687 | 19.4 |
| 640x480 | After | 59.24 | 16.855 | 17.954 | 19.134 | 18.5 |
| 1280x720 | Before | 59.22 | 16.866 | 18.715 | 19.706 | 41.1 |
| 1280x720 | After | 59.25 | 16.878 | 19.350 | 20.895 | 39.3 |
| 1920x1080 | Before | 55.70 | 17.953 | 24.037 | 25.782 | 81.2 |
| 1920x1080 | After | 58.38 | 17.129 | 19.954 | 23.115 | 69.4 |

At 1080p the observed sprite cost fell from 0.624 to 0.205 ms and conversion
from 3.309 to 2.128 ms. Lower-resolution p95/p99 do not improve uniformly;
monitor pacing and system jitter dominate those cases. Wall-time variation
between runs is not attributed to a wall-ray algorithm change.

Component means below are **before → after**, milliseconds per presentation.
Capture and snapshot sampling each remain below 0.02 ms in this pair.

| Internal | Rays/walls | Sprites | RGBA | Upload | Present |
| --- | ---: | ---: | ---: | ---: | ---: |
| 640x480 | 0.5263 → 0.5962 | 0.4463 → 0.1836 | 0.4571 → 0.3735 | 0.2676 → 0.2935 | 0.2090 → 0.2317 |
| 1280x720 | 1.8558 → 2.0028 | 0.4887 → 0.1895 | 1.3352 → 0.9940 | 0.6960 → 0.7275 | 0.2292 → 0.2360 |
| 1920x1080 | 4.6330 → 4.0886 | 0.6235 → 0.2050 | 3.3089 → 2.1275 | 1.2326 → 1.1570 | 0.2391 → 0.2197 |

For context, Milestone 8's historical X11/VSync-off measurements were:

| Internal | FPS | p95 ms | p99 ms | CPU % |
| --- | ---: | ---: | ---: | ---: |
| 640x480 | 59.19 | 18.231 | 18.817 | 21.0 |
| 1280x720 | 59.22 | 19.264 | 21.166 | 45.7 |
| 1920x1080 | 57.63 | 21.008 | 24.116 | 76.8 |

These are the recorded values in `MILESTONE8.md`, not newly rerun numbers.
The paired baseline above is the closer comparison. At the diagnostic 144 FPS
target, Milestone 8 reported 139.06 / 124.64 / 70.90 FPS; the new values are
shown below. Neither experiment establishes physical high-refresh behaviour.

Additional final X11 controls (same 1280×720 drawable):

| Control | Internal | FPS | Mean ms | p95 ms | p99 ms | CPU % |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| VSync on | 640x480 | 59.94 | 16.682 | 17.588 | 17.902 | 17.1 |
| VSync on | 1280x720 | 59.11 | 16.901 | 19.230 | 20.570 | 43.8 |
| VSync on | 1920x1080 | 56.12 | 17.820 | 22.924 | 24.844 | 78.1 |
| 144 target, VSync off | 640x480 | 139.66 | 7.153 | 8.296 | 8.813 | 39.0 |
| 144 target, VSync off | 1280x720 | 133.82 | 7.468 | 9.995 | 11.592 | 81.6 |
| 144 target, VSync off | 1920x1080 | 78.87 | 12.680 | 20.534 | 22.069 | 99.8 |
| Smooth off, VSync off | 640x480 | 178.43 | 5.599 | 9.043 | 10.151 | 26.7 |
| Smooth off, VSync off | 1280x720 | 116.67 | 8.563 | 16.284 | 18.397 | 52.0 |
| Smooth off, VSync off | 1920x1080 | 60.44 | 16.528 | 24.597 | 28.302 | 75.2 |

The 640×480 VSync control spent 14.153 ms in present and maintained 59.94 FPS,
consistent with no extra monitor-period wait. VSync did not consistently block
at the larger canvases in this compositor; the existing software pacing fallback
remains relevant. Smooth off samples no snapshots and reuses unchanged native
images; its higher present-call count is not a higher gameplay rate. Smooth off
retains the application's existing 4 ms outer wait. No uniform low-latency or
144 FPS guarantee follows from these measurements.

Dummy video / **SDL software presentation**, Smooth on, VSync off, 60 Hz target:
all regular rows use a 1280×720 output; the last pair has an actual
3840×2160 drawable. This synthetic 4K output is CPU-scaled by SDL, and is
**not a measurement of accelerated 4K monitor presentation**.

| Internal → output | Version | FPS | Mean ms | p95 ms | p99 ms | CPU % |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| 640x480 → 1280x720 | Before | 59.34 | 16.835 | 18.177 | 19.151 | 55.6 |
| 640x480 → 1280x720 | After | 59.44 | 16.824 | 18.583 | 19.128 | 53.4 |
| 1280x720 → 1280x720 | Before | 59.43 | 16.810 | 17.620 | 18.554 | 37.5 |
| 1280x720 → 1280x720 | After | 59.49 | 16.809 | 17.725 | 18.136 | 33.7 |
| 1920x1080 → 1280x720 | Before | 55.19 | 18.119 | 19.362 | 19.779 | 99.7 |
| 1920x1080 → 1280x720 | After | 56.24 | 17.782 | 22.284 | 26.845 | 99.4 |
| 1920x1080 → 3840x2160 | Before | 13.40 | 74.612 | 78.031 | 79.960 | 100.1 |
| 1920x1080 → 3840x2160 | After | 13.28 | 75.295 | 80.364 | 90.338 | 100.2 |

Synthetic 4K software presentation dominates at 62.169 → 63.626 ms per present;
optimising sub-millisecond sprite work cannot remove that backend scaling cost.
This fallback is not smooth on this CPU. Three empty dummy-audio input queues
were observed in the final 4K software run (zero in its baseline); its p99 frame
was 90.338 ms. All other final measurements observed zero empty queues.
Queue emptiness is not a physical underrun measurement, but this is evidence
that sufficiently long blocking software presents can exhaust the existing
80 ms audio queue. No audio latency increase or new producer thread was added
to mask this synthetic backend limit.

Every measured game-clock increment exactly matched processed deadlines.
The slow 4K software run finished five ticks behind the immediately due clock;
that debt remained outstanding, rather than being discarded. Typical final
60 Hz runs ended zero/one tick behind; the diagnostic 144-target 1080p run ended
two ticks behind. The existing stall and demo tests independently verify debt
retention and deterministic continuation.

Native X11 requests for 4K windows were clamped to 1918×1127 drawable pixels,
so the harness explicitly excluded those from 4K performance claims. The exact
native capture viewport was 1918×1078; the exact-2× pixel oracle ran under dummy
video instead. A physical accelerated 4K before/after comparison, including the
former native-4K Auto path, remains unperformed.

Final raw logs are `final-before-native60.log`, `final-after-native60.log`,
`final-after-vsync.log`, `final-before-software.log`, `final-after-software.log`,
`final-after-target144.log` and `final-after-no-smooth.log` in ignored
`build/checks/m9/`. Earlier exploratory `bench-*` logs preceded the final
warmup/drawable checks and are not the numbers reported here.

## Remaining hardware validation and limits

The available physical panel is 1920×1200 at SDL-reported 59.880 Hz. A 4K
window/capture and simulated desktop sizes exercise SDL scaling and Auto policy;
they do not constitute testing on a physical 4K panel. No physical 120 Hz or
144 Hz display was available. Diagnostic target overrides measure CPU/present
capacity only. VSync/VRR, compositor behaviour, high DPI, real ultrawide panels,
multiple-monitor migration and live refresh changes still need hardware checks.

Wine checks API behaviour and portability, not physical Windows presentation.
Dummy audio queue observations do not measure speaker latency or hardware
underruns. Existing main-thread audio buffering tolerates bounded render stalls;
a sufficiently long blocking OS/driver call can still drain it. Real-device
listening/loopback and focus/fullscreen transitions remain acceptance checks.

The original Classic visibility pass remains authoritative for gameplay; wider
presentation does not change DOS activation/pickup rules. Original ray-angle
quantisation and source-art resolution remain visible. Auto rejects extreme
aspects that cannot fit the renderer's minimum 320×200 target, retaining the
working surface. Manual native targets up to the existing engine limits remain
available to diagnostic harnesses, but Auto never exceeds 1920×1080. No full
campaign playthrough or performance guarantee at 144 FPS is claimed.
