# Milestone 7A: resolution-independent rendering foundation

This branch is deliberately **foundation only**. It adds a bounded, caller-owned,
stride-aware indexed render-target type in WL_RENDER_TARGET.c/.h. The original
320x200 game framebuffer, projection, raycaster, wall/sprite scaler, menu and
save formats are untouched. The new target is not yet connected to the gameplay
renderer and **does not provide native high-resolution raycasting yet**.

## Intended integration

- Keep the DOS 320x200 viewport, HUD and menus as a reference and fallback.
- Allocate a bounded offscreen 3D surface at a user-selected resolution; never
  allocate from untrusted dimensions without checking overflow and limits.
- Make ray projection, wall columns, sprites, depth/occlusion and weapon placement
  use render-target dimensions, not global W3R_SCREEN_WIDTH / VIEWWIDTH.
- Composite original indexed UI artwork into the correctly aspect-scaled output.
- Present with SDL3 nearest-neighbour by default, without stretching.
- In a later step, widen the horizontal field of view **without scaling up the
  original camera's centre**. Maintain projection and sprite perspective.
- Add selectable Original / High Resolution via Display and persist in 6F TOML.
- Do not introduce separate Classic/Enhanced gameplay modes.
- Compare untouched original rendering and 6G demo traces byte-for-byte.

## Tests needed before merging

Unit-test target validation (zero dimension, null pointers, undersized capacity,
stride/padding, integer overflow, clipped writes). Confirm original gameplay
capture hashes and 6G DOS traces are unchanged. Compile Linux strict C11,
sanitizers, MinGW and run the complete owned-data regression suite.

Do not advertise this branch as implemented widescreen or higher-resolution
rendering until the raycaster and SDL presenter are actually connected and
validated.

## ECWolf feature comparison and adoption limits

Reference implementation: ECWolfEngine/ECWolf at `src/sdlvideo.cpp`,
`src/v_video.cpp`, `src/config.cpp`, and `src/wl_menu.cpp`.
ECWolf's SDL presentation handles multiple aspect ratios (including 16:9,
16:10 and ultrawide) and exposes display synchronization options. Use these
as *design references*, not implementation to copy. Verify ECWolf's source
licence before considering any source reuse. This milestone introduces
independent C11 code only.

Prioritized additions to Revive after the bounded render-target foundation:

1. Native aspect-correct 4:3 fallback; selectable wider 16:9, 16:10, 21:9
   and 32:9 rendering through additional rays and corrected projection.
   Do not stretch an existing 4:3 framebuffer.
2. True high-resolution 3D target with unchanged original WL6 textures/sprites.
3. Correct 320x200 original HUD/menu artwork composition on widescreen.
4. Optional VSync control and performance checks after SDL presentation works.
5. Optional screenshot shortcut and user-data export; no original assets
   bundled, and screenshot output is opt-in/user-generated.

All new presentation options belong in the existing Display menu and 6F TOML.
Keep Original/Nearest as the default. No separate Enhanced game mode, changes
to the 70 Hz simulation, mod loading, or replacement artwork. Test 6G
demo-reference traces and original framebuffer hashes before merge.
