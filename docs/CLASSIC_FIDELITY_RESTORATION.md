# Classic fidelity restoration — implementation and tests

Branch: `feature/classic-fidelity-restoration`, PR #5. Baseline: `42ae8e2`,
based on merged Milestone 6E (`e35bd48`). The initial F2–F6 code was compiled
with CART and checked with the full 70-case owned-data suite before editing:
**70 passed in 303.929 seconds**. A separate strict-C11 SDL harness verified
all five initial panel entry points and frozen game time, and exposed the
incorrect return to the main menu. F5's native display panel was also an
explicit mismatch in the initial audit.

This pass implements native C11/SDL3 behaviour from the protected DOS source.
No WOLFSRC, commercial archives, save format, original artwork or master
branch is changed. No PR is merged automatically.

## Variable viewport and border

`WL_VIEW.c` ports the original NewViewSize and CP_ChangeView workflow:

- Size `n` is 4 through 19 inclusive; default 15.
- `width = n*16`, `height = n*8` (HEIGHTRATIO 0.50); all heights are even.
- `x = (320-width)/2`, `y = (160-height)/2`; the lower 40 pixels are the HUD.
- The resulting range is 64×32 at (128,64) through 304×152 at (8,4).
- Pixel-angle tables are recalculated for the active width using the existing
  DOS focal length/MINDIST formulas. Ray-angle and wall-height buffers have a
  bounded 304-column capacity; drawing iterates only over active columns.
- Wall posts, sprite clipping/occlusion, weapon scaling, death fizzle and
  death-camera regions use active geometry. The existing fixed-point traversal,
  movement/collision and ray hit rules remain in place. This is a real smaller
  raycast view, not a scaled copy of the former 240×120 image.

The original DrawPlayBorder fills the top 160 rows with palette index 127,
clears the view black, and draws black top/left, index-125 bottom/right, and
index-124 lower-left corner bevels. Palette values are not replaced with RGB
approximations. Menu return redraws the border/view/HUD without advancing
actors, pickups or RNG. View size remains current when loading a save and is
not serialized in the existing format.

Change View previews an empty black view against that border. Left/Down
shrinks, Right/Up enlarges; bounds clamp at 4 and 19. Held arrows repeat every
10 presentation tics without blocking the event loop. Preview does not commit
view geometry; Enter commits and Escape cancels. The three original instruction
lines use the existing Wolfenstein font. Native fullscreen/scaling/window size
settings now live at **Control → Display settings**, retaining Alt+Enter/F11.
Both input presets use the same Classic viewport and menus.

## Function keys and pause/return

`WL_MAIN.c` separates a direct F-key panel's return destination from a panel
opened in the main control menu. Direct panels freeze game simulation and
return to it; main-menu panels return to their parent menu. Resume resets the
simulation clock after fade-in, clearing pending input. Loaded games also use
that resume boundary. Death, victory and intermission retain save restrictions.

| Key | In living gameplay |
| --- | --- |
| F1 | GOODTIMES boss key: black C> prompt; Escape resumes |
| F2 | Save Game; unavailable storage leaves gameplay usable |
| F3 | Load Game; unavailable storage leaves gameplay usable |
| F4 | Sound settings |
| F5 | Original Change View |
| F6 | Control settings; native display is a separate child panel |
| F7 | End Game, with Y/N/Escape confirmation |
| F8 | Quicksave to the selected quick slot; first use opens Save |
| F9 | Confirm quickload; first use opens Load |
| F10 | Quit, with Y/N/Escape confirmation |

F8 writes the existing description without asking to overwrite an established
quick slot. F9 names the slot before loading. The save menu's last selection
is shared by established quick operations, as in DOS. Canceled initial
selection does not establish a quick slot. Invalid/missing slots reopen normal
selection. Load failures retain the live game and present an acknowledgement;
confirmation-time corruption is tested. New/end game clears quick selection.
The original six portable slots and atomic save implementation are retained.

F1 deserves special care: `WOLFSRC/VERSION.H` defines GOODTIMES. In that
configuration `US_ControlPanel(sc_F1)` calls BossKey, not HelpScreens. The SDL
implementation stays in the original framebuffer, draws C> with an existing
font, mutes music temporarily and restores the user's music/effects settings
on Escape. It never invokes a shell or DOS emulator. Other release help layouts
are outside the currently supported GOODTIMES layout. No attempt is made to
render the incompatible retained help-article picture references.

F1–F11 cannot be assigned to gameplay actions. SDL non-repeat key edges enter
the original menu scan-state boundary before simulation. Function-key entry
clears pending action input, so it cannot also fire/move on the entry tic.
Fullscreen shortcuts keep their separate Milestone 6E consumption rules.

## Attract-mode demos

The original title (15 s), credits (10 s) and high scores (10 s) now lead to a
validated embedded demo, then back to the title. T_DEMO0 through T_DEMO3 rotate
between cycles; invalid/missing chunks or unavailable maps are skipped safely.
The existing CA Huffman decoder performs all archive reads; data is never
modified or extracted into distributable fixtures.

`WL_DEMO.c` checks the four-byte header and declared little-endian length,
requires complete three-byte commands, maps in 0–59, and signed movement in
original bounded −100…100 units. The fourth header byte is ignored, as in DOS:
the first two owned streams contain 0xd8 there, not a required zero. It is not
treated as a high length byte. Buttons use original attack/strafe/run/use and
weapon bit positions. Run is already represented in recorded movement values;
those values are not scaled a second time for the run bit.

Playback starts hard difficulty and RNG index zero. It bypasses live bindings,
mouse turning and Modern movement; recorded use edges and attack/weapon states
feed the original game functions. Commands advance in four-tic groups, matching
DEMOTICS=4, and movement, actor transition/think timers, projectile speed, door/
pushwall movement, face/attack clocks and palette decay use that group size.
Normal gameplay continues to use one tic per simulation update. Accumulating
one tic at a time or four at a time produces identical demo state/frame hashes.
A long host stall retains the existing bounded catch-up policy; it does not
make gameplay simulation depend on display frame count.

Playback shows the original SPR_DEMO overlay. Saving and relative mouse capture
are disabled for attract demos. A fresh key or mouse click aborts to the menu;
unfocused clicks are ignored and held clicks are not reused as gameplay fire.
End, death, exit or decoding failure releases demo/game state. Missing/corrupt
streams continue the attract cycle rather than pretending a demo ran.

**Historical results, superseded by [Milestone 6G](MILESTONE6G.md):** all four
recordings now match the documented DOS reference scope and complete their
recorded duration. The following table records the original restoration result.

**Original synchronization limitation:** the owned streams are valid but do not complete
their recorded paths in this port. Current measured playback is:

| Demo | Source map | Declared tics | Tics before death |
| --- | --- | --- | --- |
| 0 | 37 (E4M8) | 2764 | 1048 |
| 1 | 43 (E5M4) | 7596 | 928 |
| 2 | 56 (E6M7) | 4560 | 1532 |
| 3 | 31 (E4M2) | 6624 | 396 |

These are observed native-port results, not DOS reference results. No immunity,
extra ammunition, replacement commands or modified commercial assets are used
to force completion. Synthetic streams reach their exact declared duration;
owned runs are deterministic across input presets and clock batching. An
instrumented DOS-versus-port trace is still required to locate remaining
simulation/visibility/RNG-order differences. Do not label this complete demo
fidelity. Native episode/floor HUD numbering also normalizes global demo map
indices rather than showing DOS's global “floor 38” style number.

## Verification

Final results on 2026-11-02:

- CART build passes. All 43 application C translation units compile with
  strict C11 warnings as errors in Linux sanitizer and MinGW builds.
- Full **78-case regression suite with owned data passes** (331.748 seconds).
  This includes all 240 map/difficulty combinations, six controlled finale
  paths, original graphics/RNG/AI, sound timing and save/load regressions.
- Full **78-case ASan/UBSan suite passes** (844.700 seconds), with
  `UBSAN_OPTIONS=halt_on_error=1` and no sanitizer reports. The final eight
  focused Classic cases also pass separately under sanitizers (43.355 seconds),
  including the explicit F2-save/F3-load and invalid-episode guard checks.
- A separate comparison compiles the baseline branch's controls harness
  against baseline and final source. Complete save payloads in slots 1, 2, 3,
  5 and 6 match byte-for-byte across revisions for both synthetic and owned
  data, covering normal Classic/Modern input, enemy/RNG state and save resume.
- CART SDL3 and desktop-output-directory regression scripts pass.
- The final application and Classic harness cross-link against the official
  SDL3 3.4.18 MinGW SDK. Under Wine/dummy drivers, all six synthetic/owned
  viewport, shortcut and demo cases pass, including full save-payload equality.
  Five-frame owned E1M1 application startup passes under Wine and under Linux
  ASan/UBSan with isolated user-data storage. These are not physical Windows
  keyboard/mouse/display tests.
- A Linux X11/software-renderer shortcut/panel check passes on repeat, and
  private owned-data Change View / resized-view captures were visually
  inspected. The original-font instructions fit the 320×200 UI and teal border
  surrounds the active view and HUD correctly. An earlier desktop attempt
  failed while acknowledging a quickload error; it did not reproduce with
  diagnostics enabled, and its cause is not established. Deterministic dummy
  driver runs pass. This is recorded as a desktop-automation limitation, not
  evidence of exhaustive physical-input testing.

Reproduction commands:

```sh
./build/tools/cart build --profile cart-sdl
WOLF3D_DATA=/absolute/path/to/owned/WL6 python3 -m unittest discover -s tests -v
```

The sanitizer command and SDK configuration are in [BUILD.md](BUILD.md).
The new eight-case `test_classic.py` builds its SDL C harness with strict C11
warnings as errors, optionally ASan/UBSan. Tests cover synthetic/owned data,
all 16 sizes, DOS projection-angle and border-pixel oracles, oversized wall/
sprite clipping, untouched HUD rows, every-size death fizzle, full save-payload
isolation, current view on load, F1–F10 dispatch, direct return/frozen clock,
view acceptance/cancel/clamps, storage absence, quicksave/quickload/corruption,
confirmation cancel/accept, malformed/missing demos, exact synthetic durations,
owned replay diagnostics, render hashes, clock cadence and focus/key/click abort.
Existing attract tests now expect another title cycle or a real demo, and display
tests enter native settings through Control rather than Change View.

Known fidelity differences are tracked in
[CLASSIC_FIDELITY_AUDIT.md](CLASSIC_FIDELITY_AUDIT.md). No physical Windows input,
high-DPI/multi-monitor or physical DOS/VGA parity is claimed. Reused SDL's
static library is not sanitizer-instrumented. Campaign coverage remains all
map/difficulty combinations and controlled finales, not full human playthroughs.

## Milestone 12 continuation

The later completion branch restores quit personality, shared confirmations,
Pause, new-game replacement confirmation, high-score entry/persistence,
completion transitions, jukebox, MLI/BAT, portable debugging, sign-on capability
lights, mouse navigation, editor repeats and disk activity artwork. It retains
these viewport/demo protections and the native save/configuration formats.
See [the complete source/verification matrix](MILESTONE12_DOS_FIDELITY_BACKLOG.md).
The actual historical executable now supplies a private 20-frame pixel oracle
for all quit sayings at both DOS dialog heights and Saving/Loading windows.
Hardware diagnostics, other editions and physical audio/CRT comparison remain
explicitly outside the verified result.

## Milestone 13 continuation

See [the final audit/hardening report](MILESTONE13_FINAL_FIDELITY.md) and
[complete state-transition inventory](DOS_STATE_TRANSITIONS.md). Established
quickload now resumes directly without Get Psyched, as `CP_LoadGame(1)` does.
The private executed-DOS drawing oracle covers 30 frames, including partial
Get Psyched progress and default scores; continuous presentation parity is
still limited to the explicitly recorded evidence.
