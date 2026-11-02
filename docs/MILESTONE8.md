# Milestone 8: decoupled frame pacing (in progress)

This feature branch begins the independent presentation-frame timing work,
while keeping the DOS simulation and Classic renderer as the authoritative
baseline. Initial code includes a stand-alone presentation clock interpolation
helper and tests. It is **not integrated into the game loop yet**, and does not
unlock higher FPS on its own.

## Planned migration

1. Audit all simulation-dependent draws: the Classic visibility pass marks
   spotvis and can affect actor activation/pickups; do not run it for every
   presentation frame. Preserve original GameStep/DemoLoop tick semantics.
2. Introduce an explicit update boundary: run each authoritative 70 Hz tick
   exactly once, then capture immutable previous/current presentation state.
3. Add a renderer-only interpolation factor for player camera, actors, doors,
   and pushwalls. New/destroyed actors, teleport/death/pause/load transitions
   require snap/reset rules. Interpolated state must never write to the game.
4. Render extra presentation frames without advancing AI, RNG, combat, doors,
   saves, or the original 70 Hz clock. Respect monitor VSync and frame pacing.
5. Keep original Classic timing/output as default and pixel-identical, with
   optional smooth presentation in the existing Display menu and TOML.
6. Benchmark actual end-to-end frame times at 60/120/144/240 Hz and measure
   audio underruns/latency at 1080p and ultrawide. Avoid unbounded catch-up.
7. Run strict C11, sanitizer, owned-data demo trace, frame hash, Linux and
   Windows/Wine tests before merging.

## HUD colour polish

Modern widescreen gameplay fills the full scene area teal (original palette
index 127), then composites the untouched original 4:3 HUD in the middle.
This replaces the black strips alongside the bottom HUD in modern mode.
Classic 320x200 framebuffer, original menu composition and game logic remain
unchanged. Inspect at 16:9 and ultrawide to confirm the intended appearance,
and verify the palette and original framebuffer are unchanged.

No code was borrowed from ECWolf. No original game archives are bundled.


## Automatic refresh-rate pacing (PR #11 update)

The Display menu now exposes Smooth Rendering on/off but no manual FPS limit.
The platform layer queries the **current display mode** for the SDL window,
falls back to the desktop display mode, rounds refresh to a nominal integer
rate, and caps it at 240 FPS. If no valid display refresh can be determined,
the fallback is 60 FPS. The existing rational presentation deadline scheduler
uses that rate when Smooth Rendering is enabled. It is re-evaluated each frame
to follow display changes. This is an upper target, not a guaranteed FPS.

`presentation_fps` is no longer a persisted TOML setting. Older files
carrying that key are handled as unsupported/ignored by the existing parser;
`smooth_rendering` remains persisted. The original 70 Hz simulation,
Classic mode and fixed-step debt rules remain independent of the new policy.

When VSync is enabled, SDL's present call may block and act as a tighter
limit than the software deadline. Actual VRR/VSync interactions, display
migrations, and performance under load require hardware tests. Monitor
refresh values above 240 Hz are deliberately capped to limit CPU use.

The menu and config regression sources have been updated, but **no builds,
test suite, physical monitor checks or audio listening tests were executed
by the assistant for this update**. Keep this PR draft until the full
regression suite and frame pacing measurements pass.
