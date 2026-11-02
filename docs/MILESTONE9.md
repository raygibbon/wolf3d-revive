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

## Architecture comparison: Wolf3D iOS versus Revive

The original [Wolf3D iOS source](https://github.com/id-Software/Wolf3D-iOS)
uses CPU world traversal and GPU wall quads (see
`wolf3d/code/wolf/wolf_opengl.c`, `R_Draw_Wall`). This is a different
architecture from Revive's **CPU software raycaster and SDL3 texture upload**.
The iPhone's `iphoneFrame` also couples gameplay with rendering, unlike
Revive's independent fixed 70 Hz simulation and presentation snapshots.

**Decision:** do not replace Revive's renderer with the iOS geometry renderer,
add GLES/Vulkan as a dependency or copy any iOS/ECWolf source. Milestone 9 is
a targeted **software-render-resolution policy**: Auto keeps the internal
image at or under 1920x1080 and SDL3 fits the resulting texture to the actual
window or monitor. The existing manual 640x480/1280x720/1920x1080 choices,
widescreen framing, classic DOS mode and interpolation remain available.

This follows the same separate-internal-resolution-and-display principle
used by pixel-art games, not the exact rendering technology of Stardew Valley.

## Performance acceptance before merge

For a 3840x2160 16:9 display, verify Auto selects 1920x1080 without
changing fullscreen/window output size. Check nearest-neighbour 2x scaling
against linear, compare the 1080p output against the former native-4K Auto
path, and measure frame times with VSync and smooth presentation on/off.

Also check 2560x1440, 1600x1200, 3440x1440 and 3840x1080 to ensure correct
aspect handling and no unexpected cropping. Test menu↔game transitions,
fullscreen toggles, monitor migration, allocation-failure rollback and
high-DPI output sizes. Review any ultra-wide source sampling where the
1920x1080 ceiling implies a shorter 1920x540 render target.

The 1080p cap **reduces work but does not guarantee 144 FPS**: Milestone 8
reported about 71 actual FPS at 1080p for a diagnostic 144 FPS target on the
measured CPU. Do not introduce adaptive resolution or GPU drawing just to
claim a higher FPS number; first verify consistent pacing and artwork quality.

No compilation, benchmark or physical monitor testing was run for this
documentation update.
