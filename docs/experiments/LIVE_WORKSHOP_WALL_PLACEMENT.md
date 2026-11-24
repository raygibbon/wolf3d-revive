# Live Workshop wall-placement prototype

**Branch:** `experiment/live-workshop-wall-placement`

**Validation base:** `278c5e1`

**Review date:** 24 November 2026 (Australia/Adelaide)

**Status:** **PARTIAL**. The end-to-end application path and headless native API
are implemented and the available automated gates pass. A human session with
legally installed WL6 data and a physical display was **NOT RUN** in this
environment, so the experiment's manual exit gate is still open.

## Purpose and architecture

This experiment proves a small Minecraft-style interaction inside the running
first-person game: toggle Workshop, aim at a nearby wall, place a wall in the
adjacent empty cell, collide with it, remove it, and continue in the same
level. It does not complete roadmap milestones R2, R4, R5, or R6.

Inspection found these integration points before implementation:

- `tilemap[64][64]` is the authoritative runtime wall grid. Classic raycasting
  (`WL_DRAW.c`) and player collision (`WL_AGENT.c`) read it directly.
- Enhanced rendering reads immutable copies made by `WL_PresentCapture()` from
  that same grid. There is no Workshop map or collision copy.
- `actorat`, `staticblock`, `statobjlist`, door tile encodings, pushwall state,
  and the player's fixed-point bounds are the edit-safety inputs.
- SDL input enters device-independent `ID_IN`/`WL_CONTROLS` state. Requests are
  queued by `WL_MAIN.c` and committed at the beginning of a simulation tic in
  `GameStep()`, before door, pushwall, or actor iteration.
- Classic draws the cue into the indexed framebuffer viewport. Enhanced draws
  the crosshair into its render target after the production scene pass and
  composites the compact viewport panel; neither path writes the status bar.

## Implementation

### Native edit API

`WL_WORLD_EDIT.h/.c` defines the SDL-free `wl_edit_world` view and these public
operations:

- `WL_EditWorldValid`
- `WL_EditCanSetWall` / `WL_EditSetWall`
- `WL_EditCanRemoveWall` / `WL_EditRemoveWall`
- `WL_EditTileBlocked`
- `WL_EditResultName`

`WL_EditSetWall` accepts only an empty, interior tile and a wall ID within the
loaded wall-page range. It rejects player overlap, actors, blocking statics,
nonblocking placed objects, and a placement that seals the player's last
cardinal exit. Elevator switch IDs 21/22 are never selected or placed.
`WL_EditRemoveWall` accepts only a normal wall, rejects borders, doors,
pushwalls and elevator switches, and performs a bounded 64x64 reachability check so removal
cannot expose an open map perimeter. Every check completes before the one-byte
tile mutation, so failures are atomic.

`WL_WorkshopLiveWorld()` binds this interface directly to `tilemap`,
`actorat`, `staticblock`, `statobjlist`, `pushable`, `player`, and the current
pushwall state. Renderer and collision therefore observe the mutation without
a rebuild or map reload.

### Mode, targeting, simulation, and presentation

`WL_WORKSHOP.h/.c` owns only transient mode, target, pending-request, selected
wall, and feedback state. Level shutdown clears it. The selected material is
the first normal wall already validated in the loaded map; this prototype has
no material browser.

Selection samples the actual player position and facing along a fixed-point
ray in 1/64-tile increments, bounded to three tiles. The first nonempty tile is
the removal target. The immediately preceding empty tile is the placement
target. A door may provide a placement surface but is never removable. No hit
or a ray leaving the map yields no target.

Workshop commits one queued request at a safe `GameStep()` boundary. While it
is enabled, player movement and looking continue, but level time, enemy/actor
thinking, combat, doors, pushwalls, and face animation pause. Rendering,
palette updates, audio positioning, menus, and first-person navigation remain
active. Exiting resumes the same runtime world and player pose.

The centre crosshair is green for a valid placement and red for invalid/no
placement. A compact two-line panel at the bottom of the 3D viewport identifies
Workshop, the wall ID, controls, coordinates, and the most recent explicit edit
result. It never writes the original status bar at Y=160. Classic backs up the
panel and crosshair pixels and restores them on explicit clear; Enhanced draws
the crosshair natively and composites the same panel into its render target.
Leaving Workshop, opening a menu/pause transition, or changing levels clears the
overlay and its saved pixels.

### Inputs

The suggested F6 key conflicts with the existing Controls panel, F1-F10 are
already panel shortcuts, F11 changes scaling, and F12 captures screenshots.
The inspected nonconflicting binding is:

| Action | Input |
| --- | --- |
| Toggle Workshop | Backtick/grave (`` ` ``) |
| Place | configured Fire action; left mouse in mouse-enabled/Modern controls |
| Remove | configured Use action; right mouse in mouse-enabled/Modern controls |
| Move/look | existing configured controls |
| Menu | Escape / configured Menu action |

Presses are edge-triggered. A held mouse button does not repeat edits.
Workshop consumes Fire/Use presses instead of firing a weapon or operating a
door. Toggle transitions clear mouse buttons to prevent a held edit click from
becoming a weapon shot after exit.

## Files changed

- `src/engine/WL_WORLD_EDIT.h/.c`: validated native mutation boundary.
- `src/engine/WL_WORKSHOP.h/.c`: mode, selection, queue, live binding, and cues.
- `src/engine/WL_MAIN.c`: toggle/edit-request input client and menu-transition
  overlay clear.
- `src/engine/WL_PLAY.c`: safe commit boundary, simulation policy, per-tic UI
  update, viewport overlay drawing, and screen/level transition clears.
- `src/engine/WL_WORKSHOP.h/.c`: bounded viewport panel, crosshair, pixel backup
  and restore, feedback timing, and Enhanced panel compositing.
- `src/engine/WL_RENDER.c`: Enhanced target cue.
- `src/engine/WL_GAME.c`: level-lifetime reset.
- `src/engine/WL_CONTROLS.h/.c`: bounded mouse-button transition clear.
- `tests/workshop_tests.c`, `tests/test_workshop.py`: synthetic unit/integration
  fixture using production edit, collision, raycasting, Classic overlay,
  viewport-size transition, cleanup, and Enhanced composition functions.
- `tools/validation/run.py`: canonical `LIVE-WORKSHOP` focused gate.

No asset, archive, map format, commercial data, screenshot, dependency, or
generated binary is added.

## Validation evidence

Environment: Debian GNU/Linux 13 x86-64; GCC 14; Python 3.13; CART 0.2.0;
local static SDL3. No `WOLF3D_DATA` directory was available.

| Command | Exit | Result |
| --- | ---: | --- |
| `python3 tools/validation/run.py LIVE-WORKSHOP` | 0 | PASS; one compiled headless harness. Final run: `build/validation/LIVE-WORKSHOP/20261124T032215Z/summary.json`. |
| `SANITIZE=1 python3 -m unittest discover -s tests -p test_workshop.py -v` | 0 | PASS; one ASan/UBSan-instrumented harness, no finding. |
| `python3 -m unittest discover -s tests -p test_render.py -v` | 0 | PASS; 9 executed, 3 owned-data skips. Classic/Enhanced geometry, HUD isolation, presentation and visual baselines passed. |
| `python3 -m unittest discover -s tests -p test_classic.py -v` | 0 | PASS; 12 executed, 4 owned-data skips. All 16 DOS viewport sizes and interface paths passed. |
| `python3 -m unittest discover -s tests -p test_controls.py -v` | 0 | PASS; 2 executed, 1 owned-data skip. |
| `python3 tools/validation/run.py R1.3` | 0 | PASS; 5 mandatory jobs. `build/validation/R1.3/20261124T023352Z/summary.json`. |
| `python3 tools/validation/run.py R1.4` | 0 | PASS; 2 mandatory jobs. `build/validation/R1.4/20261124T023352Z/summary.json`. |
| `python3 tools/validation/run.py R1.5` | 0 | PASS; 3 mandatory distribution/inventory jobs against the staged tree. `build/validation/R1.5/20261124T023638Z/summary.json`. |
| `python3 tools/validation/run.py R0.6` | 0 | PASS; 6 mandatory Classic/Enhanced/display/presentation/engine jobs. `build/validation/R0.6/20261124T032343Z/summary.json`. |
| `./build/tools/cart build --profile cart-sdl --verbose` | 0 | PASS; C11 application compiled and linked without a warning in the final focused build. |
| `git diff --check` | 0 | PASS before report creation; rerun at delivery. |

The focused harness covers valid place/remove/place, adjacent placement,
unrelated-tile integrity, repeated operations, every coordinate boundary,
wall-ID bounds, player/actor/static/door/pushwall/border rejection, last-exit
and enclosure protection, invalid state, atomic failures, direct/diagonal/range
and boundary-stable selection, and outside-map selection. Its integration case
places through `WL_EditSetWall`, observes the new tile through production
`TryMove` and `WL_RenderRay`, removes it, and observes traversable collision and
the farther original wall through those same production functions. Overlay
regressions assert that the status bar is never modified, changed pixels stay
inside the 3D viewport at all 16 view sizes, explicit clear/disable/level reset
restore the framebuffer, and the Enhanced target receives the panel and
crosshair only while the overlay is active.

## Interactive smoke procedure

Prerequisite: a legal local directory containing the eight required WL6 data
files. The data remains external and must not be copied into the repository.

```sh
./build/tools/cart build --profile cart-sdl --verbose
build/cart-sdl/linux/wolf3d-revive --data /absolute/path/to/WL6 --level
```

For each presentation mode:

1. In Options -> Controls, select Modern or enable mouse input if mouse edit
   buttons are desired. Keyboard Fire/Use also work.
2. In Options -> Display, test once with high-resolution rendering disabled
   (Classic) and once enabled (Enhanced).
3. Start or remain in a level and press backtick. Confirm the Workshop strip
   and crosshair appear without a reload and movement/look still work.
4. Aim at a wall no more than three tiles away. Confirm the adjacent placement
   coordinate and valid/invalid colour update as position/facing changes.
5. Press Fire/left mouse once. Confirm exactly one wall appears immediately.
6. Walk into it and confirm movement is blocked.
7. Aim at that wall and press Use/right mouse once. Confirm it disappears.
8. Walk through the cleared tile.
9. Press Escape, return from the menu, and confirm Workshop remains coherent.
10. Press backtick to exit. Confirm normal Fire/Use behavior returns and the
    player pose and session edit remain unchanged.
11. Re-enter and confirm the session edit state is still present.

**Human result:** **NOT RUN** — this environment had no legal WL6 data path and
no physical-display operator session. Classic/Enhanced automated renderer
regressions passed, but they are not a substitute for this observation.

## Gate matrix

| Criterion | State | Evidence / limitation |
| --- | --- | --- |
| Toggle in running level; preserve pose; no reload | **PARTIAL** | Production path implemented and state reset is lifecycle-bounded; human observation NOT RUN. |
| Movement/look continue; hostile simulation pauses | **PARTIAL** | `GameStep()` policy implemented and regression-tested indirectly; human behavior NOT RUN. |
| Real grid target and visible valid/invalid preview | **PARTIAL** | Deterministic selector tests and both render paths implemented; human legibility NOT RUN. |
| Place wall in authoritative world | **PASS** | Headless API and live binding tests mutate `tilemap` only. |
| Immediate renderer observation | **PASS** | Production `WL_RenderRay` sees placed wall and sees the farther wall after removal. Physical display remains NOT RUN. |
| Immediate collision block/clear | **PASS** | Production `TryMove` rejects placed wall and accepts removed tile. |
| Remove wall and preserve unrelated state | **PASS** | Unit/integration cases and atomic whole-grid comparisons pass. |
| Boundary, ID, player, actor, static, door, pushwall safety | **PASS** | Focused negative matrix passes. |
| No player trapping or opened perimeter | **PASS** | Last-exit and bounded enclosure rejection tests pass. |
| Fire/Use interception and held-click debounce | **PASS** | Requests use `CT_Pressed`; Workshop branch suppresses combat/use; controls regression passes. |
| Headless native C API | **PASS** | Focused harness requires no SDL window or commercial data. |
| Classic and Enhanced presentation preserved | **PASS** | R0.6 Classic/render/display/presentation mandatory jobs pass; hands-on visual inspection NOT RUN. |
| HUD isolation, cleanup and mode transitions | **PASS** | Headless tests prove the status bar is untouched, overlay pixels stay in the viewport at every view size, and clear/disable/reset restore the framebuffer; Enhanced composition is covered. |
| Existing R1.3/R1.4 behavior preserved | **PASS** | Both required runners pass. |
| CART build | **PASS** | Exit 0, no compiler warning in final focused build. |
| Human same-session demonstration | **NOT RUN** | No legal WL6 path / operator display session available. |
| No commercial/new binary assets | **PASS** | Staged source/document/test diff only; R1.5 inventory/distribution gate passes. |

## Known limitations and handoff

- Edits are runtime-session state. Authored persistence, atomic map saving,
  undo/redo, and save-game policy belong to R2/R4/R6 and are not promised here.
- One existing wall material is selected automatically; no palette exists.
- Placement requires a nearby surface. There is no free-space, drag, room,
  door, actor, item, or free-flight authoring.
- Removing a wall preserves existing area metadata. The enclosure guard protects
  renderer stability, but a future canonical world transaction must own area
  semantics and validation explicitly.
- The preview is intentionally a crosshair and compact viewport panel, not a translucent 3D
  block. Hands-on usability remains an open gate.
- The selector is bounded to 192 fixed-point samples. It allocates nothing per
  frame. Disabled Workshop adds no selector, scan, allocation, or render cue.
  Removal's 4096-cell stack flood is bounded and runs only on a remove request.
- This experiment does not read or write PK3. Future R2 adapters should resolve
  named content IDs into the canonical world; R4 should replace this narrow
  live view with approved transaction ownership; R5 can build richer tools on
  the same native operations; R6 owns durable save/playtest/return behavior.

## Next command

After supplying legal owned data, run the launch command above and execute the
Classic and Enhanced checklist. Record each observation as PASS/FAIL, then
rerun `python3 tools/validation/run.py LIVE-WORKSHOP` before review.
