# Milestone 3: original menus, input and attract screens

The application now runs the original WL6 title -> credits -> high-score
sequence, then opens the main menu. Any supported key during attract mode
opens the menu early. Original fonts, graphics, colors, layout, text alignment,
gun selection indicator and palette interpolation come from `WOLFSRC/`.
No modern UI toolkit, replacement engine or reference-port code is used.

## Run and controls

From the repository root:

```sh
./build/tools/cart build --profile cart-sdl
./build/tools/cart run --profile cart-sdl -- \
  --data "$HOME/.steam/steam/steamapps/common/Wolfenstein 3D/base"
```

Add `--menu` to start directly in the control panel. Up/Down navigate, wrapping
and skipping unavailable rows. Enter/Space selects. V, B and Q select View
Scores, Back to Demo and Quit, respectively. Escape in the main menu opens
quit confirmation: Y exits, N/Escape cancels. Any key on View Scores returns
to the menu. Closing the SDL window exits directly.

New Game, Sound, Control, Load Game, Save Game and Change View are disabled,
drawn in the original `DEACTIVE` color and skipped by navigation/shortcuts.
The first enabled selection is View Scores. No action pretends to start a game,
load/save data or configure unported audio/input. The original mouse legend
is retained as artwork; mouse navigation is not implemented in this slice.

The selected GOODTIMES layout in the original VERSION.H has nine actual rows,
without Read This. Its row count is bounded by the actual array, avoiding the
historical release's hardcoded ten-row/conditional-array mismatch.

## Ported modules and provenance

| File | Source and behavior |
| --- | --- |
| `src/engine/ID_VH.c/.h` | `ID_VH.C/H`: buffered planar picture blits, original eight-pixel VWB picture alignment, bars/lines and transparent proportional fonts. Font headers and every glyph offset/extent are decoded explicitly and checked before drawing. Low-level `VL_MemToScreen` continues to support arbitrary pixel coordinates. |
| `src/engine/ID_US_1.c/.h` | `ID_US_1.C`: `US_Print`, newline/window positioning, proportional measurement. String literals are no longer modified in place. |
| `src/engine/WL_MENU.c/.h` | `WL_MENU.C/H`: `ClearMScreen`, `DrawStripes`, beveled `DrawWindow`, `DrawMainMenu`, `SetupControlPanel`, tic-driven `HandleMenu` and original quit `Message` geometry. Coordinates, text, palette indices and gun pictures retain original ownership. |
| `src/engine/WL_INTER.c` | `WL_INTER.C:DrawHighScores`, default seven-entry table from `ID_US_1.C`, original picture headings, episode/level labels and fixed-width digit glyphs 129 onward. These are the authored defaults, not invented player scores. |
| `src/engine/ID_IN.c/.h` | `ID_IN.C/H`: original set-1 key namespace, make/break state, LastScan/LastASCII and clear operations. Press edges survive a quick down/up between frames, while OS auto-repeat is ignored. |
| `src/engine/WL_MAIN.c/.h` | `WL_MAIN.C:DemoLoop` plus control-panel flow, converted from blocking loops to explicit states/deadlines. Main-menu scores, return-to-attract and original Y/N quit behavior are wired to real implemented actions. |
| `src/engine/ID_TIME.h` | Original `TickBase=70` semantics expressed as an overflow-safe nanoseconds-to-tics conversion, independent of presentation count. |
| `src/engine/ID_VL.c/.h` | Incremental `VL_FadeIn`, `VL_FadeOut`, `VL_FadeStep`: original six-bit `orig + delta*i/steps` interpolation, including C integer truncation toward zero and exact final palette. |
| `src/platform/sdl_input.c/.h` | SDL physical scancodes map to set-1 codes. Both edges are forwarded; modifier/keypad aliases remain held until all matching physical keys release. Focus loss clears physical and logical key state. |
| `src/platform/data_file.c/.h` | Portable directory enumeration for ASCII case-insensitive WL6 matching on Linux/Windows. Exact spelling wins; otherwise ambiguous duplicate spellings fail instead of selecting an arbitrary file. |
| `src/platform/sdl_video.c/.h` | Monotonic SDL clock, sleeping, key/focus/quit event forwarding, safe partial-init cleanup, repeated startup/shutdown and cached framebuffer/palette texture uploads. |
| `src/main.c` | Startup/lifecycle and CLI, now without SDL-specific headers or calls. `--check-ui` validates/draws the complete UI slice without opening a window; `--check-assets` retains the title-only check. |

`ID_CA.c/.h` now delegates filename resolution to the platform file helper and
shares graphics error reporting with font/picture validation. Existing Huffman,
offset, decompression and allocation bounds remain in force. `cart.toml` keeps
the same C11/SDL_Renderer configuration; its scope comment is updated. CART
itself required no changes. The prior CART working-tree changes were preserved.

## Timing and transitions

The original hold times are 15 seconds for TITLEPIC, 10 for CREDITSPIC and 10
for DrawHighScores. The 30-tic attract fades and 10-tic menu fades have their
own deadlines; hold time starts after fade-in, as in the original blocking
sequence. Black attract fades and dark-red menu fades use the original target
colors. Completed transitions catch up to scheduled times after a delayed
frame rather than resetting the clock to the latest presentation.

Original demo playback is not implemented or simulated. At the demo handoff,
the program fades into the menu. Back to Demo restarts the supported sequence.
This is an explicit milestone boundary; the title/credits/scores order and
hold durations are preserved. The menu gun blinks with the original >8/>70-tic
thresholds; held arrows repeat on a 20-tic deadline. Shortcuts and navigation
skip disabled rows. The original blocking half-step gun-slide animation is
not ported; selection moves directly between rows. Audio feedback is also
absent until audio dependencies are ported.

Quit confirmation uses the first original English quit phrase; random phrase
selection awaits the original RNG. N/Escape cancels without resetting the
selection, and Y runs the original dark-red menu fade before closing. Saved
configuration/high-score import, high-score entry, help, gameplay-related
submenus, PG-13/signon startup screens, audio and demo playback remain outside
this milestone. No gameplay simulation or frame-rate-dependent movement was
introduced. Windows linking/runtime, fullscreen and gamepad remain future work.

## Resource and initialization hardening

The SDL initializer owns cleanup at every failure stage and preserves the
failure message across teardown. Startup can be retried after a failed renderer
creation, and shutdown is idempotent. SDL-specific code stays under platform/.
The frame loop sleeps between event polls; it does not wait on tics in a spin
loop. Tests inject virtual ticks directly into the original UI flow.

Unchanged pixels and palette reuse the existing texture contents. Palette-only
changes still recolor the complete indexed image. Exact snapshots are compared
so direct framebuffer writes by original-style engine code remain supported;
resizing never requires re-decoding assets or a fresh texture upload. Renderer
presentation remains separate from the 70 Hz UI/timing updates.

## Validation

Commands are in [BUILD.md](BUILD.md). Results:

- CART GCC/C11 build with strict warnings passed; no new warnings. Repeat build
  reports up to date.
- Twenty unittest cases passed with the owned Steam path enabled, including all
  prior Git-protection, memory, Huffman, framebuffer and malformed-archive tests.
- The same suite passed with ASan/UBSan instrumentation of the application and
  C tests, including clean UI teardown, repeated SDL lifecycles and renderer-init
  failure cleanup. The pre-existing SDL static archive is not instrumented.
- Synthetic font/picture archives test transparency, metrics, immutable multiline
  text, malformed heights/offsets/truncation, menu drawing, selection wrapping,
  disabled actions, shortcuts, held repeats, fades and exact/coarse attract clocks.
- SDL dummy/software tests inject key-down/up and repeat/focus events. A complete
  SDL-event-to-menu test opens the menu from the title, navigates held arrows,
  views scores, cancels quit and confirms a clean exit. Texture-upload counts
  verify unchanged frames/resizes do not re-upload; palette changes do.
- Steam title pixel/palette oracle comparison remains passing. Real menu/fonts,
  credits and high-score screens loaded in place and were visually inspected
  via SDL readbacks. Captures remain under ignored build output.
- All active C sources passed MinGW C11 compile-only checks with warnings as
  errors. Native Windows execution is not claimed.
- Existing CART SDL3 suite (including raylib/Android fixtures), desktop output
  directory, Android profile/icon and Linux macro regressions passed. Expected
  existing icon-copy warnings remain unchanged.
- Whitespace, Git safeguards, original-source preservation and asset-ignore
  checks passed. No game assets, binaries, captures or generated fixtures were
  added to Git. No commits or pushes were performed.

Milestone 4 can now add bounded GAMEMAPS/MAPHEAD and VSWAP loading, deterministic
fixed-point/RNG helpers, original ray traversal and first-level movement behind
this working original UI. No Ironclaw 3D work was started.
