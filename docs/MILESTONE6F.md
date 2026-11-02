# Milestone 6F — Persistent TOML preferences

Work is on `feature/persistent-toml-settings`, PR #6, starting from `4f75fc7`.
The initial implementation built with CART and passed the existing **78-case
owned-data suite** (289.035 seconds), but had no configuration-specific tests.
Review found truncating/unreported writes, permissive partial parsing, duplicate
keys, binding-key suffix acceptance, order-dependent binding conflicts, wrong
mouse defaults for preset-only files, lost custom window dimensions and an
uninitialized path pointer on failed video startup. These are corrected here.

No WOLFSRC, commercial archives, game/save formats, 320×200 drawing, raycasting,
AI, RNG or 70-Hz simulation rules are changed. Configuration is a separate file
from the six saved games. Master is unchanged and PR #6 is not merged.

## Location and lifecycle

The application calls `SDL_GetPrefPath(NULL, "wolf3d-revive")` after successful
video startup and keeps that allocated UTF-8 path until shutdown. Defaults are:

- Linux: `~/.local/share/wolf3d-revive/config.toml`; `XDG_DATA_HOME` overrides the
  data root, giving `$XDG_DATA_HOME/wolf3d-revive/config.toml`.
- Windows: `%APPDATA%\wolf3d-revive\config.toml`.

The path is never derived from `--data`, the working directory, or a commercial
archive name. Loading/writing uses UTF-8-aware Windows file APIs through the
existing save-file boundary. Preferences and `slot1.w3s`–`slot6.w3s` share the
user-owned directory but have independent content, versions and lifecycles.

Load occurs after the audio/video backends are initialized and before the
UI/game starts. A missing file uses defaults. Normal exit, including closing
the window and finite `--frames` runs, saves current preferences. Asset/UI/
level check-only commands do not load or write preferences. Failed startup or
abnormal exit does not save. A failed preference write reports an error while
allowing the already completed game session to exit normally.

Malformed, unreadable and unsupported-version files leave startup defaults in
place and are **not automatically replaced on exit**. Repair or rename that
file to re-enable persistence. Semantically unsupported individual values use
defaults, with a diagnostic; other valid values still load. No original archive
is rewritten. Atomic replacement does not follow a destination symlink into
another file, including an asset.

## Schema and defaults

The writer emits schema version 1. An omitted version is accepted for migration
from the initial PR. The following is a valid example with all ordinary fields;
the binding keys described below are emitted for both profiles as well:

```toml
schema_version = 1

[controls]
preset = "classic"

[classic_controls]
mouse_enabled = false
invert_mouse_x = false
mouse_sensitivity = 10

[modern_controls]
mouse_enabled = true
invert_mouse_x = false
mouse_sensitivity = 10

[audio]
music_volume = 70
effects_volume = 85
music_enabled = true
effects_enabled = true

[display]
fullscreen = false
integer_scaling = false
window_width = 960
window_height = 720

[classic]
view_size = 15
```

| Field | Accepted values / meaning |
| --- | --- |
| `controls.preset` | `"classic"` or `"modern"` |
| Profile `mouse_enabled`, `invert_mouse_x` | TOML booleans |
| Profile `mouse_sensitivity` | Integer 1–40; default 10 |
| Profile `bind_A_S` | SDL physical/USB scancode; A is action 0–14, S is slot 0/1; zero unbinds |
| Audio volumes | Integers 0–100, independently applied |
| Audio enable states | Booleans, independent of volumes |
| `display.fullscreen` | Borderless desktop fullscreen / windowed |
| `display.integer_scaling` | Integer canvas scaling / fit; tiny-window fallback still applies |
| `display.window_width`, `window_height` | Integers 1–16384, in window points; remembered windowed size |
| `classic.view_size` | Integer 4–19, independent of window dimensions |

All 30 bindings are written in **each** profile table. Action identifiers and
default pairs are:

| A | Action | Classic pair | Modern pair |
| --- | --- | --- | --- |
| 0 | Forward | 82, 96 | 26, 82 |
| 1 | Backward | 81, 90 | 22, 81 |
| 2 | Turn left | 80, 92 | 80, 92 |
| 3 | Turn right | 79, 94 | 79, 94 |
| 4 | Strafe left | 0, 0 | 4, 0 |
| 5 | Strafe right | 0, 0 | 7, 0 |
| 6 | Run | 225, 229 | 225, 229 |
| 7 | Alt strafe | 226, 230 | 226, 230 |
| 8 | Fire | 224, 228 | 224, 228 |
| 9 | Use/open | 44, 0 | 44, 0 |
| 10–13 | Weapons 1–4 | 30/31/32/33, 0 respectively | Same |
| 14 | Menu | 41, 0 | 41, 0 |

For example, `bind_0_0 = 8` assigns E to primary Forward. These are physical
SDL scancodes, not ASCII, DOS scan codes or Windows virtual-key codes. Escape
is reserved for Menu; F1–F11 are reserved globally. Unknown SDL scancodes and
duplicate nonzero assignments, including repeated alternatives, are invalid.
Fixed mouse-button assignments remain left-fire/right-use when mouse is enabled.

Each preset now owns its bindings, mouse enable, sensitivity and inversion.
Changing presets retains both profiles. Reset Defaults explicitly resets only
the selected preset. Loading validates the complete candidate binding table
before installing it, so swaps work independently of file/key order. If any
binding in a profile is invalid or conflicting, **that profile's entire binding
table falls back to its preset defaults**; its valid mouse options and the
other profile remain available. Invalid profiles are never partially unbound.

Legacy `[controls]` mouse/binding fields are accepted as overrides for the
selected profile, after the new profile tables. Legacy `display.window_scale`
(1–5) supplies dimensions when explicit width/height fields are absent. The
writer migrates to the canonical two-profile / explicit-dimensions schema.
Loading a saved game retains the current preferences; they are not serialized
into or restored from the game save.

## Parser policy and safety

This is deliberately a **fixed-schema TOML subset**, not a general TOML engine.
Supported syntax includes bare keys and single-level bare table names, spaces/
tabs, LF/CRLF, comments (including after table headers), single-line basic and
literal strings, basic-string escapes/Unicode escapes, booleans and signed
64-bit integers. Decimal signs, separators and unsigned hexadecimal/octal/binary
forms are checked without integer overflow. The emitted file also parses with
Python's independent `tomllib` implementation.

Dotted/quoted keys and tables, arrays, inline tables, multiline strings, floats
and dates are outside this subset. Such files are rejected transactionally and
preserved, rather than being partially interpreted. Raw tabs inside strings,
escaped NUL and strings longer than 63 decoded UTF-8 bytes are also unsupported.

The parser bounds input at 64 KiB, lines at 1024 bytes, names at 63 bytes,
records at 256 and explicit tables at 64. Embedded NUL, invalid UTF-8, truncated
escapes, invalid separators/leading zeros, integer overflow, duplicate keys
(including unknown keys), repeated tables and scalar/table collisions are
rejected. Unknown tables/keys containing supported scalar syntax are ignored.
Canonical output replaces comments, ordering and ignored fields; it does not
preserve arbitrary editor formatting or future unknown data. A declared future
schema is preserved and never migrated automatically.

Parsing completes before any preference changes are applied. Syntactically
invalid/read-error files cannot apply a valid-looking prefix. Partial valid
files are interpreted from preset-aware defaults, not leftover state from a
previous load. Unsupported typed/range values report a warning. Diagnostics
name the key or line rather than echoing arbitrary file contents.

## Atomic output and platform behaviour

The serializer builds bounded output in memory, checking every formatting
operation. It then uses the existing `SaveFile_WriteAtomic` primitive:

1. Create a unique same-directory temporary file with exclusive creation
   (0600 on POSIX; user-readable/writable on Windows).
2. Check the complete write, `fflush`, file `fsync` / Windows `_commit` and close.
3. Replace with POSIX `rename` or UTF-16 `MoveFileExW` with replacement and
   write-through flags. Remove temporary files on failure.
4. Attempt parent-directory sync on POSIX after replacement.

Failure before replacement retains the previous destination byte-for-byte.
Directory synchronization is best effort where supported; universal power-loss
or remote-filesystem durability is not claimed. Concurrent writers each publish
a complete file; the last successful replacement wins. There is no per-field
multi-instance merge or locking protocol. Save-game serialization is unchanged.
Fault injection exists only in test builds defining `WOLF3D_CONFIG_TEST`.

Display persistence distinguishes observed SDL flags from an accepted pending
request. A quick exit while fullscreen is still being applied saves the accepted
target, not a transient old window flag. Once SDL reports rejection or the
existing request timeout expires, the observed state is saved. Loading preserves
the remembered custom windowed dimensions even while fullscreen. Leaving
fullscreen after restart restores those dimensions. No blocking fullscreen
synchronization or new simulation work is introduced into normal startup.

The GOODTIMES boss key temporarily mutes music. Shutdown restores its saved user
settings before capturing preferences, so closing the window on the C> screen
does not turn that temporary mute into a permanent preference.

## Verification

Verified on 2026-11-02:

- CART Linux C11 build passes; all **44 application C translation units** also
  compile with strict warnings as errors in Linux sanitizer and MinGW builds.
- Final **104-case normal suite with owned WL6 data passes** (329.054 seconds),
  including all existing campaign, gameplay, audio, display and save regressions.
- ASan/UBSan passes the full **101-case run** (763.881 seconds), plus the three
  subsequently added path/concurrency cases (8.725 seconds). Together these
  exercise all 104 final cases under sanitizers. The final **26-case configuration
  suite** also passes separately (12.261 seconds), including the final tiny-
  window and exit-fullscreen size-restoration assertions. No sanitizer reports;
  `UBSAN_OPTIONS=halt_on_error=1` is enabled.
- CART's SDL3 and desktop-output-directory regression scripts pass.
- Native Linux X11/software-renderer tests pass three shutdown/startup cycles
  with fullscreen restored and complete canonical-file equality. Leaving
  fullscreen after restart restores the custom 1000×600 window size.
- MinGW/SDL3 3.4.18 cross-builds pass. Wine/dummy-driver checks pass canonical
  round trips, all six atomic-write fault stages, profile switching/reset,
  fullscreen rejection/timeout and restored window dimensions, UTF-8 Unicode
  directories, SDL preference paths, and synthetic/owned game/save independence.
  The Windows application itself passes three restart cycles preserving every
  seeded setting, and retains a future-version file unchanged. SDL's observed
  Wine path is `C:\users\rgibbon\AppData\Roaming\wolf3d-revive\config.toml`.
- Full save payloads match before/after configuration I/O with synthetic and
  owned data on Linux and under Wine; game time and RNG remain unchanged.

The configuration suite builds strict C11 with warnings as errors and optionally
ASan/UBSan. Its independent TOML oracle uses Python 3.11+ standard `tomllib`:

```sh
./build/tools/cart build --profile cart-sdl
WOLF3D_DATA=/absolute/path/to/owned/WL6 \
  python3 -m unittest discover -s tests -v
# Full sanitizer build/run commands are in BUILD.md.
```

The 26 focused tests cover missing/partial/legacy files, every preference,
both profiles, binding swaps/conflicts/reserved keys, malformed/duplicate/
overflow/UTF-8 input, parser limits, 1000 deterministic corruption samples,
atomic create/partial-write/flush/sync/close/replace failures, concurrent writers,
read-only/unreadable paths, symlink safety, Unicode directories, SDL preference
paths, pending/failed fullscreen, custom/tiny window dimensions, full save-payload
and RNG/time equality, boss-key mute restoration and actual application restarts
against read-only synthetic game data. Commercial tests read the player's owned
WL6 files without copying their content into test fixtures.

Native Linux X11 fullscreens and restarts are tested separately from dummy
video. Windows API checks use MinGW, the official SDL3 3.4.18 SDK and an isolated
Wine prefix. **Physical Windows, high-DPI/multi-monitor transitions and OS/crash/
power-loss durability are not verified.** Reused SDL's static archive is not
sanitizer-instrumented. Existing Classic demo synchronization and other fidelity
limits remain in [CLASSIC_FIDELITY_AUDIT.md](CLASSIC_FIDELITY_AUDIT.md).
