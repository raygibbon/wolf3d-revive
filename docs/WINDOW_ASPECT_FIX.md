# Window aspect independence: Control / Display regression fix

**Branch:** `fix/window-aspect-independent-controls`

## User-visible behaviour

The SDL window size is independent of the original DOS canvas and of the
high-resolution internal world render target.

- The classic DOS menu, setup-like screens and startup pages stay at a
  **320x200 indexed framebuffer presented at 4:3**. They are centred with
  black bars in any other window aspect, with no horizontal distortion.
- Enhanced gameplay uses its selected aspect ratio (Auto, 4:3, 16:9, 16:10,
  21:9 or 32:9) in **the same window**, with no automatic window resizing
  when transitioning to/from a menu.
- **Control → Display settings** is now directly accessible from the
  restored DOS Control screen, without visiting Customize controls.
  The existing route through Customize controls is retained.
- Display → Window size now cycles window dimensions independently of any
  4:3 integer menu scale: 4:3, 16:10, 16:9 and ultrawide candidates.
  The list is ordered as a selector, not a promise each size fits every monitor.
  Window-manager restrictions and fullscreen changes remain authoritative.
- Window size and **3D resolution** are separate settings. The existing
  1080p Auto internal ceiling remains intact.

## Implementation

- `src/engine/WL_CONTROLS_MENU.c`: seventh active Control entry, classic
  cursor and unchanged original menu artwork/font styling.
- `src/engine/WL_DISPLAY.c`: aspect-independent window presets, with
  `scale=0` for explicit dimensions; reset selection on reopening.
- `tests/display_tests.c`: direct Control→Display navigation and sequential
  selection across mixed aspect ratios.

## Verification checklist

Run existing strict-C11 and sanitizer test suites on Linux, including the
SDL dummy backend `test_display.py`; then verify manually on 16:9 and
16:10 monitors:

1. Window-size left/right cycles both 16:9 and 16:10 and saves/restores the
   chosen width/height.
2. On entering Control or Sound, the window size does not change; original
   DOS menu content remains centred at corrected 4:3.
3. Resume high-resolution 16:10/16:9 game world: full appropriate aspect,
   no horizontal stretching of DOS art and no window-size jump.
4. Resize window externally, then re-enter Display: width and height reflect
   the actual window; next preset selection remains usable.
5. Verify fullscreen and Restore Window Size, integer-vs-fit scaling,
   mouse capture/focus, display setting save/load and no DOS demo divergence.
6. Test failed and asynchronous window-manager requests without losing
   previously selected settings.

**Validation status:** Initial implementation committed through GitHub.
Compilation, automated suites and physical monitor validation have **not**
been run by the assistant. Review/repair failures before a PR.
