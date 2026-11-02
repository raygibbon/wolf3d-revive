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
