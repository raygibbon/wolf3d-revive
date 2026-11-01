# Build and run the original title

CART is the primary build system. This milestone ports the original MM/CA/VL
slice needed by `CA_CacheScreen(TITLEPIC)`; it is not a playable game yet.

## Local Linux build

From the repository root, reuse the existing SDL3 SDK in the sibling CART checkout:

```sh
./build/tools/cart build --profile cart-sdl --verbose
./build/tools/cart run --profile cart-sdl -- \
  --data "$HOME/.steam/steam/steamapps/common/Wolfenstein 3D/base"
```

Escape or closing the resizable window exits. The discovered Steam directory
above is used read-only. Supply a different absolute path if your installation
is elsewhere; `cart run` changes the working directory to the output folder.
No path is hardcoded into the executable and no game data is staged by CART.

`build/tools/cart` is the ignored current CART 0.2.0 executable produced during
milestone 1. If absent, build the current CART source in `../cart` and use that
executable instead. The installed `cart`/historical tracked binary reported
0.1.0 during milestone 1. No installed tool was overwritten in this milestone.
The local SDL SDK reports development version 3.5.0, revision `08b9c5539`.

To decode/check without a display, or perform a finite graphical smoke test:

```sh
./build/cart-sdl/linux/wolf3d-redux --data /absolute/path/to/WL6 --check-assets
./build/cart-sdl/linux/wolf3d-redux --data /absolute/path/to/WL6 --frames 3
./build/cart-sdl/linux/wolf3d-redux --help
```

`--capture build/checks/title.bmp` additionally saves a local SDL readback
before presentation (create the parent directory first). SDL readback contains
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

CART remains unchanged in milestone 2. It recursively builds the lowercase
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

This compiles C memory/Huffman/framebuffer/cache tests with GCC C11 and strict
warnings as errors. It also compiles an SDL readback test using the static SDK
at `../cart/vendor/SDL` (override with `SDL_SDK`). The video test uses SDL's dummy
video/software renderer, requiring no desktop. Without that static SDK, that
one test reports a skip. Synthetic archives are generated in temporary
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
  src/main.c src/engine/ID_MM.c src/engine/ID_CA.c src/engine/ID_VL.c \
  src/platform/sdl_video.c ../cart/vendor/SDL/build/libSDL3.a -pthread -lm \
  -o build/checks/wolf3d-redux-sanitize
SANITIZE=1 WOLF3D_BINARY="$PWD/build/checks/wolf3d-redux-sanitize" \
  WOLF3D_DATA="$HOME/.steam/steam/steamapps/common/Wolfenstein 3D/base" \
  python3 -m unittest discover -s tests -v
```

`SANITIZE=1` also instruments the C unit/video tests. The reused SDL static
archive is not instrumented. Neither these tests nor the window implement
simulation or exercise unported DOS modules.

From CART, regression checks can use the current executable by absolute path:

```sh
CART=/absolute/path/to/wolf3d-redux/build/tools/cart sh tests/test_sdl3.sh
CART=/absolute/path/to/wolf3d-redux/build/tools/cart sh tests/test_desktop_dirs.sh
```

See [milestone 2 results and scope](MILESTONE2.md) for validation and remaining work.
