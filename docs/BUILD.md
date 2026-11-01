# CART build preparation

This milestone builds a console dependency probe. It opens no game window or
audio device and does not load commercial data. All original `WOLFSRC/` files
remain outside the active build until their portable dependency slices exist.

## Existing local setup

Inspected CART at `../cart`, baseline commit
`d73ca1f83cd23a559d4bae8eaa9670bff74fa055`. Its current source is 0.2.0;
the installed `cart` and initial tracked build binary reported 0.1.0.
Use a build from the current source for the configuration below. No installed
tool was overwritten. Validation used `build/tools/cart` in this repository;
that generated binary is ignored. CART's tracked build outputs were restored
to their initially clean state after bootstrapping.

The existing `../cart/vendor/SDL` has headers and `build/libSDL3.a`.
It reports SDL 3.5.0, revision `08b9c5539` (a development checkout, not a pinned
release). It is sufficient for this probe; choose a pinned supported SDL3
release before distributing the eventual port. System `pkg-config sdl3`
was unavailable, so do not assume that dependency is installed system-wide.

From the repository root, reuse that local dependency explicitly:

```sh
./build/tools/cart build --profile cart-sdl --verbose
./build/tools/cart run --profile cart-sdl
# Or run directly:
./build/cart-sdl/linux/wolf3d-redux
```

`build/tools/cart` is a local validation artifact, not a checked-in bootstrap.
For future use, build CART in its own checkout and substitute the resulting
current executable in these commands. The SDL checkout is reused read-only;
no clone, download, symlink or SDK installation is performed.

## Normal SDK configuration

The base `cart.toml` uses an external SDL3 SDK at ignored `vendor/SDL`, with
headers in `include/SDL3` and a shared library in `build/`. CART also tries
`build/Release`, `build/Debug`, and `build/lib` if `lib_dir` is omitted. Change
`path`, `include_dir`, `lib_dir`, `lib`, and `linkage` to your actual SDK layout
using the implemented `[deps.sdl3]` keys. No assets should be put under vendor.

Then run `cart doctor`, `cart build`, and `cart run` with the current CART
binary. Without an SDK at `vendor/SDL`, the base build/doctor intentionally
fails with a missing SDL dependency. CART's doctor does not currently accept
`--profile`; the local profile is validated by `build` instead.

On Windows the initial config explicitly selects GCC/MinGW so its warning
flags are valid. Use a Windows SDL3 SDK/import library, not the sibling Linux
static library profile. Put the matching SDL3 DLL beside the executable or on
PATH; on Linux install the shared library or configure the runtime loader.
The present task checks Windows compilation only, not linking or execution.

## Implemented CART capabilities and limits

Inspected `src/config/config.c`, `src/commands/build.c`, `doctor.c`, `run.c`,
`src/util/fs.c` and the GCC/MSVC toolchain implementations, plus README/tests.

| Requirement | Current implementation |
| --- | --- |
| C11 and warnings/defines | `standard`, `cflags`, `defines` per platform; GCC/Clang and MSVC adapters. Choose warning syntax for the selected compiler. |
| Sources | One recursive `project.src` root, lowercase `.c` only. No explicit source list/exclusions; uppercase original `.C` is not discovered, and GCC treats `.C` as C++. Migrate reviewed modules with lowercase extensions while retaining basenames/history. Do not point src at the entire DOS tree or include utility mains. |
| Includes and linking | `include_dirs`, `lib_dirs`, `libs`; SDL3 adds SDK paths and library name. `graphics.backend = "sdl-renderer"` is implemented; GPU names are only reserved. |
| SDL3 SDK discovery | Checks root, SDL3/SDL.h and library file. No pkg-config integration or dependency download/build. `linkage` does not force a linker mode when both static/shared files have the same name. Use an unambiguous SDK layout. |
| Static dependencies | Linux SDL3 handling adds `m`; it does not resolve a static archive's full transitive dependencies. This local SDK advertises pthread/m; the optional profile supplies pthread. Other SDK builds may need more libraries. |
| Runtime deployment | No automatic shared-library copying, DLL deployment or rpath handling. |
| Sanitizers | `cflags` only reach compilation, not the link command; there is no general `ldflags` key. The initial sanitizer probe is built explicitly below. Add a generic linker-flags facility with tests when integrated sanitizer builds require it; do not invent unsupported TOML keys. |
| Paths/profiles | Profile overlays work for SDL dependency settings, with output at `<out>/<profile>`. Fresh nested output creation was broken and is fixed in this task. |
| Assets | No commercial data is listed under CART assets or copied into output. Future `--data` must account for `cart run` changing working directory to the output folder; prefer absolute data paths. |

The only necessary CART source change is generic parent-directory creation
in `src/util/fs.c:ensure_dir`. Its `tests/test_desktop_dirs.sh` covers a first
profile build on a fresh project, default output, nested custom output, and
rejection of a file blocking the output path. `TESTING.md` lists the test.
Existing raylib/Android behavior is exercised by CART's SDL3 and profile-icon
regressions. No Wolfenstein-specific behavior was added to CART.

## Validation commands

```sh
python3 -m unittest discover -s tests -v
./build/tools/cart build --profile cart-sdl --verbose
./build/tools/cart run --profile cart-sdl
git diff --check
git -C ../cart diff --check
```

From CART, use an absolute path for the freshly built binary:

```sh
CART=/absolute/path/to/wolf3d-redux/build/tools/cart sh tests/test_desktop_dirs.sh
CART=/absolute/path/to/wolf3d-redux/build/tools/cart sh tests/test_sdl3.sh
CART=/absolute/path/to/wolf3d-redux/build/tools/cart sh tests/test_profile_icons.sh
sh tests/test_linux_macro_compile.sh
```

Direct Linux sanitizer verification (CART remains the normal build system):

```sh
mkdir -p build/checks
gcc -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
  -fsanitize=address,undefined -fno-omit-frame-pointer -g \
  -Isrc -I../cart/vendor/SDL/include src/main.c \
  ../cart/vendor/SDL/build/libSDL3.a -pthread -lm \
  -o build/checks/wolf3d-redux-sanitize
./build/checks/wolf3d-redux-sanitize
```

Only the new probe is instrumented; the reused SDL archive is not. This is
not sanitizer coverage of the original engine. SDL itself can be rebuilt with
instrumentation later if an investigation requires it.

## Results from this milestone

- Git protection: four unittest cases passed, including HTTPS/SSH destinations,
  multiple push URLs and URL rewriting. The tests never invoke a push.
- Linux: CART 0.2.0 built and ran the C11 probe against the existing SDL 3.5.0
  static archive. An unchanged repeat build reported up to date. No probe
  compiler warnings; direct ASan/UBSan build and execution passed.
- CART: new desktop output-directory regression, existing SDL3 suite (including
  raylib/Android fixtures), Android profile/icon suite and Linux macro compile
  regression passed. Icon-copy warnings are existing behavior. Rebuilding CART
  reports existing unused-parameter and long string-literal warnings.
- Windows: MinGW compile-only checks passed for `src/main.c` and CART's modified
  `src/util/fs.c`. No Windows SDL import library/runtime was exercised.
- Whitespace checks and representative commercial-data/build ignore checks passed.
  No original `WOLFSRC/` file changed; no commercial assets entered the build.

Pending: portable engine/core, actual original graphics, data-directory loading,
input/gameplay, PCM/OPL, save compatibility and original-map parity. Those are
subsequent milestones, not features of the current executable.
