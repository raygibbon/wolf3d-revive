# Milestone 6 — first implementation pass: normal-level progression

## Current status after Milestone 6A Part 2

All original bosses, required projectiles and all six episode completion
paths are implemented. Owned-data validation now loads/renders/simulates
**all 60 maps at all four difficulties (240/240 combinations)**. Controlled
SDL weapon fights exercise every finale, Mecha Hitler's transformation,
cinematics, original statistics/articles and return through scores to menu.
See [MILESTONE6A.md](MILESTONE6A.md) for source references, test results,
differences and limitations. [Milestone 6B](MILESTONE6B.md) adds native SDL3
music and effects; see that report for audio verification and remaining limits.
Full navigated playthroughs and saves remain outstanding. The remainder records the earlier Milestone 6 pass and
its original 51-map baseline, not the current actor/finale support.

2026-11-02. This is an incremental, working gameplay/progression slice, **not
completion of Milestone 6 or of the original game**. Audio, saving/loading,
boss/ghost/victory actors and several desktop features remain unimplemented.

## Baseline review

The checkout already contained uncommitted Milestone 5 changes and untracked
source/tests/documentation. They were preserved; no reset, commit or push was
performed. Before this pass, the CART Linux build and all 31 existing Python
regressions passed with owned Steam data. The implementation was inspected
alongside `WOLFSRC/`, rather than inferred from the milestone document.

Verified existing functionality includes E1M1 maps/textures/sprites, normal
enemy state machines, weapons/combat/pickups, health/ammo/lives/score/keys/HUD,
doors, the original RNG table, palette shifts, death/life restarts and game
over returning through the scores page. Launch remained restricted to E1M1;
pushwalls, elevators, intermissions, selection, audio and saves were absent.
Milestone 5's stated scope was substantially accurate; normal-enemy state
tables existed for mutants but the map scanner never spawned them.

## What this pass implements

- New Game opens six original episode choices and four original difficulty
  choices. Arrows select, Enter/Space confirms, Escape returns one page. The
  original episode pictures, skill pictures and menu fonts are used. Choices
  persist while navigating the control panel. `--level` retains its E1M1,
  medium-difficulty direct-start behavior.
- Map selection uses `episode * 10 + mapon`. Ordinary levels preserve health,
  ammunition, weapons, score, lives and extra-life thresholds. Keys and level
  counters reset at the next floor. The HUD displays the current floor.
- Normal guard/officer/SS/dog difficulty bands are decoded individually;
  mutant stand/patrol codes and both mutant difficulty bands now work. Mutant
  health is 45/55/55/65; other normal enemy health remains original. Baby
  difficulty quarters incoming damage. Existing original AI/state tables and
  combat algorithms are reused, with no alternate gameplay engine.
- Space operates a tile-21 elevator only when facing east/west, changes its
  switch texture and ends the floor. Raw area tile 107 selects a secret exit.
  Normal exits advance one floor; secret exits select floor ten. Return floors
  are the original `{1,1,7,3,5,3}` zero-based episode table.
- Intermissions show original artwork, time, par, kill/secret/treasure ratios
  and the final bonus. All 48 ordinary-floor par times are ported. Time is
  counted in simulation tics at 70 Hz. Integer percentage truncation, zero
  totals yielding zero percent, 500 points per second under par, 10,000 per
  perfect ratio and the secret-floor 15,000 bonus match the original. Bonuses
  are awarded once, including normal extra-life handling. Ratios for the
  eight ordinary floors are retained in memory. Acknowledgement loads the
  next floor without simulating time spent on the intermission.
- Secret pushwalls reserve destination cells, block actors, count secrets,
  clear their consumed markers, move in four cardinal directions, stop after
  two tiles or one tile when blocked, and expose the player's area in newly
  cleared cells. Only one wall moves at a time. Wall/static/actor occupancy
  prevents a blocked push. The renderer ports the original partial intercept,
  texture selection/flips and 0..63 wall position; moving walls are not doors.
  Runtime marker/area arrays retain immutable cached map planes for restart.
- Death on a later floor restarts that floor, restores its entry score and
  resets health/ammo/keys/weapons according to the original Died flow. It
  retains the decremented lives and existing extra-life threshold.
- Maps requiring unported bosses, ghosts or victory actors fail explicitly.
  A failed initial/next-floor load displays its reason, releases level/page
  resources and returns to the menu on acknowledgement. Encounters are not
  removed or replaced with normal enemies to make a map appear supported.

## Source references and adaptations

`WOLFSRC/` remains unchanged. New/extended engine code is derived from:

| Original source | Runtime implementation |
| --- | --- |
| `WL_MAIN.C:NewGame`, `WL_GAME.C:SetupGameLevel`, `ScanInfoPlane`, `GameLoop`, `Died`, `ElevatorBackTo` | `WL_GAME.c`, `WL_OBJECT.c`, `WL_PLAY.c`: selection, per-level reset, inventory/entry score, progression and restart. |
| `WL_ACT2.C:starthitpoints`, `SpawnStand`, `SpawnPatrol`; `WL_AGENT.C:TakeDamage` | `WL_OBJECT.c`, `WL_COMBAT.c`: difficulty filtering, mutant spawning/health, baby damage. Existing normal-enemy state machines remain in `WL_STATE.c`. |
| `WL_AGENT.C:Cmd_Use`; `WL_ACT1.C:PushWall`, `MovePWalls` | `WL_ACT1.c`, `WL_PLAY.c`: cardinal interaction, elevator state and one-tic wall motion. |
| `WL_DR_A.ASM:vertpushwall`, `horizpushwall`; `WL_DRAW.C:HitVertPWall`, `HitHorizPWall` | `WL_DRAW.c`: moving-wall intersections, projected distance, source columns and texture pages. |
| `WL_INTER.C:LevelCompleted`, `Write` | `WL_INTER.c`: original par times, arithmetic and result artwork. |
| `WL_MENU.C:CP_NewGame`, `DrawNewEpisode`, `DrawNewGame` | `WL_MENU.c`, `WL_MAIN.c`: resumable selection pages integrated with the existing SDL event loop/fades. |

The intermission currently takes the original immediate-final-results path.
Animated counting, BJ breathing, sounds and transition/preload presentation
are still outstanding. It does not claim the full original intermission
presentation. High-score name entry/persistence and episode endings remain
unported.

The earlier fixed one-tic simulation is retained. For pushwalls, the original
`pwallstate > 256` completion check becomes `>= 256`: fixed one-tic steps can
land exactly on 256, whereas the original multi-tic loop normally overshot
it. Without this adaptation a one-tic loop incorrectly moves a third tile.
The state starts at one, crosses tiles at 128 and 256 and retains original
half-tic texture-position quantization. Simulation remains independent of
presentation. Milestone 5's documented DOS variable-tic/RNG sequence
limitations still apply; no DOS recording/pixel-perfect equivalence is claimed.

Checking every Steam map found assumptions that only held for E1M1:

- E1M3 has unused outer-edge floor. Validate enclosure of initially reachable
  floor/doors, rather than require every boundary cell to be a wall. Ray and
  movement bounds remain checked. Pushwalls may occupy a boundary cell, but
  cannot reserve outside the map.
- E2M3 has floor at a door jamb. Original SpawnDoor marks it `0x40`; preserve
  movement through the floor and the original wall-table-zero/jamb rendering.
- E4M2 has a static object inside a wall; the original scanner permits this.
- E5M4 has overlapping patrol destination reservations; original scan order
  decides which pointer owns the cell while both actors remain in the list.
- E4M6 on hard reserves a closed door as an initial patrol destination.
  Original SpawnPatrol reserves that first move unconditionally.

Tests that incorrectly labeled the first three applicable synthetic patterns
as corrupt were corrected and replaced with positive compatibility regressions.
Pool limits, invalid spawns, malformed streams, movement bounds, reachable
perimeter escapes and required sprite validation remain covered.

## Build and verification

```sh
./build/tools/cart build --profile cart-sdl
WOLF3D_DATA="$HOME/.steam/steam/steamapps/common/Wolfenstein 3D/base" \
  python3 -m unittest discover -s tests -v
```

- CART Linux build passes with portable C11 and the existing SDL3 SDK. No
  dependencies were added and no CART source/configuration changes were
  needed for this pass.
- CART SDL3 and desktop output-directory regression scripts pass.
- 37 Python regression cases pass, including strict `-Werror` C executables.
  New `campaign_tests.c` covers episode/difficulty values, normal and mutant
  spawns, baby damage, ratios/bonuses/timing, all secret return routes,
  inventory/door reset across levels, later-floor death restart, four-way
  pushwalls, blocked pushes, one-/two-tile stops and moving-wall ray distances.
  Existing AI/combat, RNG, timing, archives, input, video and Git safety tests
  continue to pass.
- SDL tests exercise episode/skill cancellation and selection, choose E2 on
  hard, operate a pushwall and elevator, acknowledge an intermission, load the
  next floor, and verify that a missing next map reports an error and returns
  to the menu. Synthetic tests use SDL dummy video/software rendering.
- All 60 owned Steam maps were checked at all four difficulties: **51 maps
  load/render at every difficulty (204 combinations); nine are explicitly
  rejected at every difficulty (36 combinations).** This validates decoding,
  spawning, sprite streams and initial rendering, not complete playthroughs.
- Native Linux SDL software-renderer checks with owned Steam files passed
  selection, an E1M1 pushwall, an elevator intermission and E1M2 loading.
  Captures of selection, skill, E2M1, the moving/opened secret, intermission
  and E1M2 were visually inspected in ignored `build/checks/`. The scripted
  check positions the player beside real secret/elevator tiles; it does not
  navigate the full level. Commercial files were read only, never staged.
- ASan/UBSan app and C tests pass the suite with owned Steam files, including
  resource ownership checks (`MM_TotalBytes() == 0`) after shutdown. The
  existing static SDL archive is not sanitizer-instrumented. See `BUILD.md`
  for the combined sanitizer compile/link command.
- All 28 application C translation units pass strict MinGW C11 compile-only
  checks with SDL3 headers. Windows linking/runtime, fullscreen transitions
  and native Windows resource behavior remain unverified.

The unsupported maps in the owned WL6 installation are E1M9, E2M8, E2M9,
E3M9, E3M10, E4M9, E5M9, E6M9 and E6M10. Consequently no episode can yet be
completed. No unsupported map is skipped automatically.

## Outstanding Milestone 6 work

1. Port boss/ghost/projectile/victory actors and their exact states/actions,
   boss keys, death cameras, BJ victory movement and episode endings. Verify
   actual playthroughs of all six episodes, including secrets and boss maps.
2. Implement the original sound manager with SDL3 output, a small compatible
   licensed OPL emulator, AdLib music, digitized effects, original sound
   priorities/timing/positioning and independent music/effects volumes.
   **There is still no audio initialization, playback or audio cleanup test.**
3. Implement versioned, pointer-free saves, multiple slots, save/load menus,
   complete actor/door/pushwall/RNG/inventory restoration, corruption rejection
   and persistence tests. There is no save/resume or quick save/load yet.
4. Add the small portable C11/TOML game configuration foundation and functional
   Classic/Enhanced settings. `cart.toml` remains build configuration only.
   No nonfunctional settings/menu rows were enabled.
5. Add automatic Steam asset discovery and keep `--data` as an override.
   Currently `--data` is required. Missing archives retain existing clear
   errors. Fullscreen/window switching and its tests remain to be added;
   existing SDL windowed 4:3 presentation remains in use.
6. Restore original demo playback where practical, score entry/persistence,
   full intermission animation/audio and initial-level transitions. Compare
   gameplay/render/timing behavior with the DOS original.
7. Link/package/test through CART with a Windows SDL SDK on Windows, and run
   actual desktop/fullscreen/audio lifecycle tests on both supported systems.

No game-specific behavior was added to CART, no Ironclaw files were modified,
no commercial assets were bundled, and no Git history or original copyright
and licensing notices were changed. No commit or push was made.
