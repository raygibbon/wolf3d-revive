# Milestone 5: E1M1 sprites, enemies and combat

E1M1 now has original static objects, guards and dogs, directional sprites,
original enemy state machines, shooting, damage, pickups, weapon animation,
live status graphics, palette flashes and death/life restarts. New Game starts
E1M1 at the original medium difficulty. Escape ends the session and returns to
the menu. This is a playable combat slice, not a complete campaign port.

## Build, run and controls

From `~/Sync/git/wolf3d-revive`, using the current CART executable and the
existing SDL3 SDK in the sibling CART checkout:

```sh
./build/tools/cart build --profile cart-sdl
./build/tools/cart run --profile cart-sdl -- \
  --data "$HOME/.steam/steam/steamapps/common/Wolfenstein 3D/base"
```

Press a key at the title, then Enter on New Game. To start directly:

```sh
./build/tools/cart run --profile cart-sdl -- \
  --data "$HOME/.steam/steam/steamapps/common/Wolfenstein 3D/base" --level
```

| Keys | Action |
| --- | --- |
| Up / Down | Forward / backward |
| Left / Right | Turn |
| Alt + Left / Right | Strafe |
| Shift | Run |
| Ctrl (either side) | Attack; release between pistol/knife attacks |
| 1 / 2 / 3 / 4 | Knife / pistol / machine gun / chaingun, when available |
| Space | Operate adjacent door; matching keys unlock locked doors |
| Escape | End this game and return to the menu |

Automatic weapons repeat while Ctrl is held. Guns require ammunition; the
knife remains available when ammunition runs out. Items are collected using
the original projected getting-distance test, subject to original health,
ammunition and gib eligibility limits. Escape does not save or pause a game.

Existing `--check-assets`, `--check-ui`, `--check-level`, `--menu`, `--frames N`,
`--capture FILE.bmp` and `--help` options remain supported. `--check-level`
validates maps, wall pages, available sprite streams, normal-enemy animation
frames, weapon frames, static sprites, the initial HUD and scene without SDL.
Malformed data disables New Game, or reports an error for a direct level/check
request. The owned Steam directory is read only; no assets are staged.

## Changes and original-source references

| Phase / files | Ported functionality |
| --- | --- |
| A: `WL_ACT1.c`, `WL_AGENT.c`, `tests/game_tests.c` | Closing-door checks now use the complete inclusive player collision square, including corners outside the door's center row/column. They also protect shootable actors' physical bounds and reserved destination tiles. Closing reopens immediately when occupied. Both orientations, all corners, just-outside positions and entry during closing are tested. Static and live-actor collision augment the original player square and axis-slide order. |
| B: `WL_DRAW.c`, `WL_SCALE.c`, `WL_SPRITES.h` | Original `TransformActor`, `TransformTile`, `CalcRotate`, `DrawScaleds`, `ScaleShape` and `SimpleScaleShape` calculations from `WL_DRAW.C`/`WL_SCALE.C`, with sprite indices from `WL_DEF.H`. Signed-magnitude rotation, actor/tile forward biases, perspective height, eight facing frames/two pain directions, ray visibility, original visible-list limit and stable back-to-front height sorting remain. Transparent post masks preserve opaque palette index zero. Original scaler directory quantization and separately anchored left/right halves are retained, including asymmetric knife frames. Wall heights include original wide-post reuse and clip sprites per column. |
| B/C: `WL_OBJECT.c`, `WL_GAME.c`, `WL_ACTOR.h`, `WL_DEF.h` | Original `statinfo`, `SpawnStatic`, `PlaceItemType`, `ScanInfoPlane`, `SpawnNewObj`, stand/patrol spawning and original medium difficulty filtering. Bounded pools hold 150 actors and 400 statics. Integer-tagged DOS `actorat` entries become separate wall/static arrays and genuine actor pointers. Actor destinations, diagonal side checks, door waiting, area numbers and ambush neighbour priority follow the original. The small area graph is rebuilt from door states instead of maintaining pointer-dependent connection counters; opening connects on its first advancing tic, closing disconnects only at fully closed. |
| C: `WL_STATE.c`, `ID_US_A.c`, `ID_US_1.h` | Original guard/dog state tables and the common officer/SS/mutant tables from `WL_ACT2.C`; `DoActor` action/think/transition ordering from `WL_PLAY.C`; `TryWalk`, `MoveObj`, `SelectDodgeDir`, `SelectChaseDir`, `SelectPathDir`, `CheckLine`, `CheckSight`, `SightPlayer`, `FirstSighting`, `T_Stand`, `T_Path`, `T_Chase`, `T_DogChase`, `T_Shoot`, `T_Bite`, `DamageActor` and `KillActor`. Original speeds, reaction delays, ambush/hearing rules, attack probabilities, shot-distance rules, pain/death frames and drops remain. The 256-entry `ID_US_A.ASM` RNG preserves increment-before-read and byte wrapping. Audio calls are omitted, while guard death-sound selection still consumes its original RNG draw. |
| D/E: `WL_COMBAT.c`, `WL_PALETTE.c`, `WL_PLAY.c` | `WL_AGENT.C` knife/gun target selection, damage, surprise multiplier, original `attackinfo`/`T_Attack` timing, semi-automatic and held automatic fire, ammunition, weapon selection, health/ammo/treasure/key pickups, extra lives and drops. `LatchNumber`/status coordinates, weapon/key artwork, health-dependent faces, look animation and chaingun pickup face use original pictures. `WL_PLAY.C` red/white DAC interpolation, intensity/duration and red-over-white priority are retained. HUD drawing caches unchanged status. |
| E: `WL_PLAY.c`, `WL_MAIN.c` | `WL_GAME.C:Died` attacker-facing rotation at two degrees/tic, weapon removal, `ID_VH.C:FizzleFade` 17-bit LFSR red wipe, input/wait and life decrement, followed by E1M1 restart with pistol/8 rounds/100 health/no keys. Score restores to this level's entry value, zero; the extra-life threshold persists. After lives reach -1, the existing scores screen is shown and a key returns to the menu. The death flow is incremental and keeps the SDL event loop responsive. |
| Validation: `ID_PM.c/.h`, tests | Page-presence queries distinguish legal sparse directory entries from missing required frames. Startup validates sprite streams beyond the initial room, common enemy animation frames and all four weapons. New synthetic archives cover object spawning and full sprite-index ranges without commercial bytes. Malformed streams, missing offscreen frames, overfull pools and invalid spawns are rejected. |
| Integration/docs: `src/main.c`, `cart.toml`, `docs/BUILD.md`, `docs/PORTING.md` | Updated help, diagnostics, current build scope and commands. CART configuration/backend/dependencies are unchanged; only its introductory comment changes. |

`WOLFSRC/` is untouched. This work does not import Wolf4SDL, wolf3d-lib or
wolf3d-portable code. Platform SDL3 video/input/timing remain isolated in
`src/platform/`; the engine adds no SDL dependency. No CART source changes,
new dependencies, commits or pushes were made.

## Rendering and timing review

The original quadrant stepping, half-angle tangent table, focal point,
signed-magnitude `FixedByFrac`, wall/door texture offsets, projection numerator,
scaler quantization and wide-post optimization were reviewed against
`WL_DR_A.ASM`, `WL_DRAW.C`, `WL_SCALE.C` and `WL_MAIN.C`. Existing exact ray and
wall-source-run regression checks still pass. Sprite scaling emits the original
source-pixel runs instead of executable x86 scaler code. Its clipping checks
each destination column instead of VGA mask/edge shortcuts; partial occlusion
at irregular wall edges may differ from those original shortcuts. No
pixel-perfect comparison against a running DOS executable is claimed.

Simulation uses one-tic steps at 70 Hz, independently of SDL presentations.
Each simulation tic refreshes visibility and projected targets, collects items
and updates the framebuffer; unchanged presentations still avoid texture
uploads through the existing video path. Batched ticks and individual ticks
produce the same tested movement, actor states and inventory for identical
input. Fast make/break edges survive until a tic consumes them. Pauses discard
elapsed time above MAXTICS (10) rather than simulating an unbounded backlog.

DOS used variable-size tic batches. Its per-frame RNG calls and probabilistic
AI can therefore produce a different sequence from this fixed-tic loop. Normal play seeds the original RNG from the low clock byte at game start
and life restart, using monotonic tics in place of the DOS BIOS clock. Asset
validation explicitly seeds zero, and tests supply deterministic start tics. The
original state machine and random table are used, not substitute enemy AI.
The fizzle retains the original `64000/70` candidate pixels per tic and full
LFSR traversal; it is not shortened to an arbitrary 70 presented frames. Its
writes are bounded to the actual viewport, fixing original border overdraw.

## Verification

```sh
WOLF3D_DATA="$HOME/.steam/steam/steamapps/common/Wolfenstein 3D/base" \
  python3 -m unittest discover -s tests -v
```

- CART `cart-sdl` Linux build passes with GCC/C11 and strict warnings.
- 31 Python regression cases pass, including strict `-Werror` C executables.
  `tests/combat_tests.c` covers projection, transparency, opaque zero,
  asymmetric anchors, sorting/occlusion, medium spawns, blocking objects,
  pickups/caps, RNG wrapping, reaction/ambush/hearing, area connectivity,
  chase/shoot/death transitions, patrol arrows, dog bites, collisions, drops,
  knife/pistol/automatic timing, ammo, selection, HUD, palette flashes,
  per-tic/batched determinism, life restarts, game-over/menu and 1,000 mutated
  sprite streams with output canaries.
- SDL event regression covers both Ctrl aliases, held pistol behaviour, quick
  taps, focus loss, movement and game/menu transitions. Existing palette,
  nearest-neighbour/aspect readback, repeated startup/shutdown, fonts, fades,
  malformed archives, memory ownership and upstream-push protection pass.
- ASan/UBSan instrumented app and C tests pass the same suite with owned Steam
  data. The reused SDL static archive itself is not instrumented.
- All 28 C modules pass strict MinGW C11 compile-only checks. Windows linking
  and runtime remain unverified without a Windows SDL SDK.
- CART SDL3, Android icon/profile, Android ads generation, desktop directory
  and Linux macro regression scripts pass. No Android device run is claimed.
- Owned Steam E1M1 validates 436 sprite pages, 20 medium-difficulty enemies,
  one corpse, 121 static objects and 23 treasure objects. An aimed pistol
  combat regression kills an original guard through real input/tics and
  verifies ammunition consumption and scoring. This test positions the
  player in a valid visible location; it is not a complete level playthrough.
- Native SDL captures of the start room and a guard/table scene were inspected
  locally: original textures, directional actor, decor, weapon, occlusion and
  HUD appear correctly. Captures remain in ignored `build/checks/`; none are
  embedded here or added to Git. The commercial files were only read in place.

The sanitizer compile/link command is in [BUILD.md](BUILD.md). Automated
fixtures contain synthetic graphics/map/sprite bytes only.

## Remaining work

- Audio/music, elevator/level completion, secret pushwalls, campaign
  progression, save/load, demos and episode/difficulty selection remain
  unavailable. E1M1 is the only launched level; no completion is simulated.
- The post-game scores display uses the existing table. Original qualifying
  high-score name entry/persistence is not ported. There is no invented
  game-over success screen.
- Audio-dependent face/sound timing remains to be integrated with the
  original sound manager. Other levels/bosses are not
  claimed to work merely because common enemy state tables are present.
- Life restart redraws the initial scene directly; the original initial
  level fizzle-in/preload presentation is not yet ported. Death rotation,
  red fizzle and wait/life reset are implemented.
- No complete E1M1 playthrough or DOS screenshot/recording equivalence test
  has been performed. Testing establishes this combat slice and its stated
  rules, not complete Wolfenstein 3D compatibility.
