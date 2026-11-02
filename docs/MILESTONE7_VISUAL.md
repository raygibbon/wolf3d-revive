# Milestone 7: ECWolf-inspired visual improvements

This branch starts the **non-invasive geometry foundation** for later display
work. ECWolf's SDL video code was studied for aspect-ratio support; no code was
copied. `WL_ASPECT` provides overflow-safe integer aspect-fitting and an
aspect-based horizontal FOV helper. It does **not** enable actual native high-res
raycasting or widescreen yet. The existing 320x200/4:3 Classic rendering path
is unchanged.

## Integration order (future work)

1. Tests and build wiring for aspect helpers; compare behavior at 4:3, 16:9,
   16:10, 21:9 and 32:9. Handle extreme inputs and resize.
2. Resolution-independent **3D** render target, with independent projection,
   ray counts and sprite z-buffer. Avoid simply stretching 320x200 pixels.
3. Maintain centre framing at 4:3 and expand horizontal visibility for
   widescreen. Test walls, sprites, doors and weapon proportions.
4. Compose original UI/HUD in correct aspect-preserved geometry. Preserve
   Classic 320x200 indexed framebuffer and pixel hashes.
5. SDL3 presentation: dynamic resolution/high DPI, nearest filtering default,
   optional VSync, safe fullscreen changes and optional screenshots.
6. Add menu options and versioned TOML persistence, conservative defaults.
7. Audio underrun investigation: measure under rendering stalls and compare
   dedicated audio PR #9 separately, not implicitly merge it.

## Guardrails and acceptance

No changes to WOLFSRC, commercial archives, gameplay simulation, 70 Hz timing,
RNG, AI, saves, demo frames, or separate Enhanced gameplay mode. Do not add
custom-map/mod support. Run unit tests, strict C11, ASan/UBSan and Windows
cross-builds, then verify owned-data 6G DOS traces and Classic frame hashes.
Record actual measured results rather than claiming unexecuted tests passed.
