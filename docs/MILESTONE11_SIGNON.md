# Milestone 11: DOS Sign-on screen (initial implementation)

## Missing original screen

The Wolfenstein 3D DOS startup sequence includes a red sign-on/hardware
summary with logos, title, conventional/EMS/XMS memory bars, and a
"Press a key" prompt. Original source: `WOLFSRC/WL_MAIN.C`,
`SignonScreen()` and `FinishSignon()`.

The source describes `introscn` as a **320x200 bitmap linked into the
DOS executable**, not a VGA graphics archive chunk. The graphics are not
bundled as a new redistributable asset with this branch.

## Implemented on this branch

- Optional original-art loader for **`SIGNON.RAW` in the supplied `--data`
  directory**: exactly 64,000 bytes of 320x200 row-major, indexed palette
  values matching the original Wolfenstein VGA palette.
- Missing file: silently skip sign-on and preserve the existing optional
  PC-13 / title sequence; invalid length: clean error, no partial display.
- Draw onto the existing indexed framebuffer, with no independent SDL
  window resizing, no custom shader or alternate menu renderer.
- `UI_SIGNON` waits for a key and then returns to the existing intro
  presentation, preserving rating/title order and game simulation.
- The presentation inherits the existing **4:3 DOS aspect-corrected
  canvas**. At 16:10, 16:9 and ultrawide window dimensions, keep the
  complete original artwork **centred and un-stretched**.
- `--menu` and `--level` skip the historical intro.

## Still needed

1. Verify extraction from legitimately owned original DOS sign-on data
   (`introscn` or its object module). A screenshot alone is not a reliable
   palette-index source. Verify the file is *row-major* and uses the
   correct 256-color palette; don't commit commercial binaries or art.
2. Add reference-pixel tests for known owned `SIGNON.RAW`; verify missing
   and truncated input; check key dismissal and sound/palette transitions.
3. Compare original timing: DOS `FinishSignon()` waits for a key, while
   the original startup may print/alter the bottom prompt. Preserve the
   actual original bitmap and avoid fabricated detection values.
4. Run full strict C11 compiler/test suite, sanitizers, SDL3 4:3
   letterbox presentation checks and user testing at 16:10/16:9.
5. Check side-border colour, startup fade and palette restoration on actual
   hardware. Avoid treating the sign-on image as a regular WL6 chunk.

**Status:** Initial code committed. Compilation and runtime tests have
not been executed. This is not a finished or merged milestone.

## Asset requirements

Place an authentic extracted `SIGNON.RAW` in the user's own WL6 data
directory if available. The repository remains code-only and does not
redistribute any artwork. If the user has only `WL6` files, they may not
contain `introscn`, since DOS linked it into the executable.

This document intentionally does not prescribe a fabricated copy of
the detected 1992 RAM/sound hardware configuration.
