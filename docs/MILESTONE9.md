# Milestone 9: predictable pixel-art scaling

## Direction

Use the same simple separation that suits pixel-art games: choose an internal
rendering resolution, then let SDL3 fit the resulting texture to the physical
display. Keep **Auto** software rendering bounded to 1920x1080. Native 4K/5K
output remains available through desktop fullscreen and SDL presentation;
it does **not** mean drawing the raycaster at 4K.

This is a fixed ceiling, **not dynamic resolution scaling**. No runtime
quality switching, GPU raycaster, new renderer backend, frame-rate selector or
gameplay changes are required.

The existing manually selectable 640x480, 1280x720 and 1920x1080 choices stay.
Original DOS 320x200 rendering, indexed palette, HUD, gameplay 70 Hz timing,
high-refresh interpolation and save/demo compatibility remain untouched.

## Auto internal size

For the active display, fit its aspect ratio into a canvas whose dimensions
are bounded by both the display and 1920x1080. Examples:

| Display | Auto internal renderer |
| --- | --- |
| 3840x2160 (4K) | 1920x1080 |
| 2560x1440 | 1920x1080 |
| 1920x1080 | 1920x1080 |
| 1280x720 | 1280x720 |
| 1600x1200 | 1440x1080 |
| 3840x1080 (32:9) | 1920x540 |
| 3440x1440 | approximately 1920x803 |

The final image is presented by SDL3 at the actual window/display size,
respecting the existing aspect, integer scaling, nearest/linear settings.
For 4K 16:9, a 1080p texture scales exactly 2x. Wider displays keep their
aspect; they are not forcibly cropped to 16:9.

## Verification needed

- Run strict C11 build, existing aspect, display, renderer and demo tests.
- Inspect 4K/1440p/ultrawide outputs under real hardware where available.
- Compare frame times and CPU load before/after at equal input, with and
  without Smooth Rendering/VSync.
- Confirm original Classic hashes and all 5,386 demo commands still match.
- Confirm Auto resolution changes and allocation rollbacks work across
  monitor changes and fullscreen transitions.

No compilation, automated test suite, or physical 4K verification was
performed as part of the initial GitHub-only change.
