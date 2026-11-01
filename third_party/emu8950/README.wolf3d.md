# emu8950 provenance

Upstream: https://github.com/digital-sound-antiques/emu8950

Pinned commit: `f82a22e80b68820f263084aa8506b12eb1c9b4dd` (source identifies
itself as emu8950 v1.2.3). Downloaded directly from that commit, 2026-11-02.
`LICENSE` is upstream's MIT licence; retain it and all source notices.
The active Wolf3D port remains GPL-2.0-only, with this MIT dependency.

Imported files: `emu8950.c`, `emu8950.h`, `emuadpcm.c`, `emuadpcm.h`, `LICENSE`.
The ADPCM module is needed to link upstream's API, but Wolf selects YM3812
and never uses Y8950 ADPCM, DOS, DMA or Sound Blaster hardware emulation.

Two local safety patches:

- `emuadpcm.c`: zero-initialize the object with `calloc`, so partial allocation
  failure can safely free either buffer during initial chip creation.
- `emu8950.c:lookup_exp_table`: replace a left shift of a potentially negative
  integer with multiplication by two and an explicit result cast. UBSan found
  this with owned music and the synthetic test instrument. The bounded result
  fits in int16_t and the intended sample values are preserved.

`src/engine/ID_OPL*.c` compile each implementation as a separate translation
unit. GCC conversion/shadow/unused-parameter/pedantic warnings are suppressed
only around these imported files. The port itself is checked with strict
warnings as errors. No synthesis code is generated or downloaded at build
or launch. See `docs/MILESTONE6B.md` for selection and fidelity limitations.
