# Build and run Wolf3D Revive

CART builds original menus and normal-level gameplay: sprites, enemies,
weapons, pickups, HUD, life restarts, episode/difficulty selection, pushwalls,
elevator exits, intermissions and progression. All original bosses,
projectiles and six finale paths are implemented; all 60 maps load/render/
simulate at all four difficulties. Audio and save/load remain unported. See
[milestone 6A](MILESTONE6A.md) for verified scope and playthrough limitations.

## Local Linux build

From the repository root, reuse the existing SDL3 SDK in the sibling CART checkout:

```sh
./build/tools/cart build --profile cart-sdl --verbose
./build/tools/cart run --profile cart-sdl -- \
  --data "$HOME/.steam/steam/steamapps/common/Wolfenstein 3D/base"
```

Press any key during the title/credits/scores to open the menu. Use Up/Down
and Enter/Space; V, B and Q select View Scores, Back to Demo and Quit. Escape
opens quit confirmation in the menu (Y exits, N/Escape cancels). Closing the
window exits directly. New Game opens the episode and difficulty pages; arrows select, Enter/Space
confirm and Escape goes back. Unported main-menu rows stay disabled. A failed
level load reports the reason and returns to the menu after acknowledgement.

In the level: Up/Down move, Left/Right turn, Alt+Left/Right strafe, Shift runs,
Space operates doors, secret pushwalls and elevator switches. Ctrl fires, and
1–4 select owned weapons (knife, pistol, machine gun, chaingun). Keys unlock
their matching doors. Intermissions show the final original ratios/time bonus;
press a key to continue. Escape ends the game and returns to the menu; it does
not save or resume. Victory statistics acknowledge with a key; ending articles
use arrows, Enter or Page Up/Down for pages and Escape to close, followed by
scores and return to menu. `--level` starts E1M1 at medium difficulty directly.

The discovered Steam directory above is used read-only. Supply a different absolute path if your installation
is elsewhere; `cart run` changes the working directory to the output folder.
No path is hardcoded into the executable and no game data is staged by CART.

`build/tools/cart` is the ignored current CART 0.2.0 executable produced during
milestone 1. If absent, build the current CART source in `../cart` and use that
executable instead. The installed `cart`/historical tracked binary reported
0.1.0 during milestone 1. No installed tool was overwritten in this milestone.
The local SDL SDK reports development version 3.5.0, revision `08b9c5539`.

To decode/check without a display, or perform a finite graphical smoke test:

```sh
./build/cart-sdl/linux/wolf3d-revive --data /absolute/path/to/WL6 --check-assets
./build/cart-sdl/linux/wolf3d-revive --data /absolute/path/to/WL6 --check-ui
./build/cart-sdl/linux/wolf3d-revive --data /absolute/path/to/WL6 --check-level
./build/cart-sdl/linux/wolf3d-revive --data /absolute/path/to/WL6 --level
./build/cart-sdl/linux/wolf3d-revive --data /absolute/path/to/WL6 --menu
./build/cart-sdl/linux/wolf3d-revive --data /absolute/path/to/WL6 --frames 3
./build/cart-sdl/linux/wolf3d-revive --help
```

`--capture build/checks/title.bmp` additionally saves a local SDL readback
of the initial title, menu (`--menu`) or level (`--level`) at full brightness, before
its fade-in (create the parent directory first). SDL readback contains
the image viewport, excluding letterbox bars. Captures contain game artwork;
keep them private and out of Git. `build/`, `*.bmp` and `*.ppm` are ignored.

## Other SDL SDKs and Windows

The base config uses ignored `vendor/SDL` with shared linkage. Without that SDK,
base `cart doctor`/`cart build` intentionally reports a missing dependency.
The optional `cart-sdl` profile overrides the root to `../cart/vendor/SDL` and
uses its existing static build; nothing is downloaded or installed.

Implemented `[deps.sdl3]` keys are `path`, `include_dir`, `lib_dir`, `lib` and
`linkage` (plus `enabled`). Defaults use `include/SDL3` and `build/`, with
fallbacks `build/Release`, `build/Debug`, `build/lib`. Adjust to your SDK layout.
The local profile supplies pthread; CART supplies SDL3 and libm. Other static
SDKs may require additional platform libraries in `libs`.

The Windows target explicitly selects GCC/MinGW so the warning flags match.
Use a Windows SDL SDK/import library, not the Linux `cart-sdl` profile. Put the
matching SDL3 DLL beside the executable or on PATH. Every new C module passes
MinGW compile-only checking; Windows linking, runtime and native resize behavior
remain unverified. No CMake build system was introduced.

No CART source changes were required in milestones 2–6. The Linux/Windows
link lists now include the math library used by the original table builder.
CART recursively builds the lowercase
`.c` files under `src`, supporting C11, defines/includes and library lists.
It has no general linker-flags key, pkg-config integration, automatic SDK
build/download, DLL staging or rpath setup. `doctor` does not accept profiles.
Sanitizer flags only reach compilation via `cflags`; use the explicit combined
compile/link below for sanitizers. The prior generic output-directory fix and
its tests in CART remain as they were at the start of this milestone.

## Tests

Build the application with CART first, then run the synthetic/unit suite and
existing Git-protection tests:

```sh
python3 -m unittest discover -s tests -v
```

This compiles memory, Huffman, graphics/UI, input, timing, map/VSWAP, raycasting
movement, sprites, RNG, enemy AI, combat, pickups, HUD, death, campaign,
intermission and pushwall tests with GCC C11 and strict warnings as errors. SDL readback
and event-to-menu/game tests use the static SDK at `../cart/vendor/SDL` (override with `SDL_SDK`). The SDL tests use the dummy
video/software renderer, requiring no desktop. Without that static SDK, those
six tests report skips. Synthetic archives are generated in temporary
directories and removed; no asset fixtures come from commercial files.

For an independent pixel-by-pixel oracle comparison using your owned files:

```sh
WOLF3D_DATA="$HOME/.steam/steam/steamapps/common/Wolfenstein 3D/base" \
  python3 -m unittest discover -s tests -v
```

Optional Linux sanitizer build (normal builds remain CART-driven):

```sh
mkdir -p build/checks
gcc -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror \
  -fsanitize=address,undefined -fno-omit-frame-pointer -g \
  -Isrc -I../cart/vendor/SDL/include \
  src/main.c src/engine/*.c src/platform/*.c \
  ../cart/vendor/SDL/build/libSDL3.a -pthread -lm \
  -o build/checks/wolf3d-revive-sanitize
SANITIZE=1 WOLF3D_BINARY="$PWD/build/checks/wolf3d-revive-sanitize" \
  WOLF3D_DATA="$HOME/.steam/steam/steamapps/common/Wolfenstein 3D/base" \
  python3 -m unittest discover -s tests -v
```

`SANITIZE=1` also instruments the C unit/video tests. The reused SDL static
archive is not instrumented. The gameplay simulation and palette flashes use
fixed 70 Hz tics. The campaign suite exercises selection, exits, statistics,
secret routing, pushwalls and later-floor restarts. Audio and save/load are
not implemented or exercised.

From CART, regression checks can use the current executable by absolute path:

```sh
CART=/absolute/path/to/wolf3d-revive/build/tools/cart sh tests/test_sdl3.sh
CART=/absolute/path/to/wolf3d-revive/build/tools/cart sh tests/test_desktop_dirs.sh
```

See [milestone 6 results and scope](MILESTONE6.md) for validation and remaining work.
