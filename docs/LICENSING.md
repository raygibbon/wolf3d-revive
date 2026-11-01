# Source licensing and provenance

The original release's limited-use notice remains untouched in
`WOLFSRC/README/LICENSE.DOC` and the historical `README.rst`. Preserve its
copyright notices and the original snapshot in Git history.

The historical GPL relicensing basis is John Carmack's account in id Software's
[official iDoom source-release README](https://github.com/id-Software/DOOM-IOS2/blob/master/readme_iDoom.txt).
It states that the original Wolfenstein source was initially noncommercial
and later released under the GPL. This project uses that later grant as its
basis for the active engine port; new C source in `src/` is GPL-2.0-only.
`COPYING` contains the GNU GPL version 2. This does not replace the historical
notice or purport to relicense archived DOS binaries, tools or artwork.

Milestone 6B imports Mitsutaka Okazaki's
[emu8950](https://github.com/digital-sound-antiques/emu8950) directly from
commit `f82a22e80b68820f263084aa8506b12eb1c9b4dd`, under its
[MIT licence](../third_party/emu8950/LICENSE). Preserve that licence and its
copyright notices when distributing source or binaries. The active port stays
GPL-2.0-only; these imported files retain their MIT terms. Exact provenance
and two local safety patches are listed in
[README.wolf3d.md](../third_party/emu8950/README.wolf3d.md).
No code was copied from a third-party Wolfenstein port. Nuked-OPL3 was reviewed
but not imported. SDL3 uses the zlib licence; retain its notices when distributing
it. See [MILESTONE6B.md](MILESTONE6B.md) for dependency selection and limitations.

Commercial `.WL6` files, SIGNON artwork, music, sounds, trademarks and other
game content are separate from the engine source license. Supply your own
Steam data outside this repository; never stage or distribute it. Milestone 2
reads the user's local Steam data in place. No commercial archive was copied
into the source tree or downloaded. Local rendered captures remain ignored.

`src/engine/gamepal.inc` reproduces the original 768-byte, six-bit VGA palette
from the already tracked `WOLFSRC/OBJ/GAMEPAL.OBJ` LEDATA record; it is not
extracted from Steam artwork. Tests independently parse that original OMF
record and check the generated constants. `GFXV_WL6.h` is the original generated
graphics-index header, copied with its DOS EOF marker removed. The original
objects and source files remain untouched. No SIGNON artwork or third-party
port implementation is incorporated in the active title path.
