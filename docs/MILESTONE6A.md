# Milestone 6A — original campaign actors and completion

2026-11-02. All original bosses, required projectiles and the six episode
completion paths are implemented. **All 60 owned WL6 maps load, render and
simulate at all four difficulties: 240/240 combinations.** Controlled SDL
weapon fights exercise all six finales, their statistics, both original
ending pages, scores and return to the menu.

This is not a claim of six fully navigated episode playthroughs, DOS demo
compatibility or pixel-perfect equivalence. The controlled tests reposition
the player and refresh health/ammunition; their exact limits are below.
Audio playback, save/load and Enhanced Mode remain outside this milestone.

## Baseline and source provenance

Part 1 added Hans, Gretel and four ghosts: 54 maps / 216 combinations and 42
Python regressions passed. Part 2 began from a clean checkout and reproduced
that build and test baseline before editing. Milestones 5, 6, the Part 1
report, PORTING.md, BUILD.md, the active engine and original source were
reviewed. Existing rendering, combat, movement, doors, pushwalls, selection,
intermissions, inventory and RNG were extended.

All gameplay references are id Software's original `WOLFSRC/`, which remains
unchanged. New engine/tests retain GPL-2.0-only notices and source attribution;
see [LICENSING.md](LICENSING.md). No third-party port or dependency was added.
Commercial assets are read from the owned Steam directory, never staged or
bundled. No CART or Ironclaw files, commits or pushes were made. Upstream's
push URL remains `DISABLED`.

## Episode finale status

Display floor numbers are one-based; absolute map indices are zero-based.
Each row is covered by the owned SDL weapon/completion regression.

| Episode finale | Absolute map | Boss encounters | Completion | Verified result |
| --- | --- | --- | --- | --- |
| E1M9 | 8 | Hans Grosse | Gold key, exit tile, BJ run/jump/yell | Statistics, both ending pages, scores, menu |
| E2M9 | 18 | Dr. Schabbs | Needle fight, death camera/replay | Statistics, both ending pages, scores, menu |
| E3M9 | 28 | Five Fake Hitlers, Mecha Hitler, Hitler | Flames, mecha corpse and inherited Hitler actor, death camera/replay | Statistics, both ending pages, scores, menu |
| E4M9 | 38 | Giftmacher | Rockets, death camera/replay | Statistics, both ending pages, scores, menu |
| E5M9 | 48 | Gretel Grosse | Gold key, exit tile, BJ run/jump/yell | Statistics, both ending pages, scores, menu |
| E6M9 | 58 | General Fettgesicht | Rocket plus gun burst, death camera/replay | Statistics, both ending pages, scores, menu |

The E3 SDL fight verifies weapon-driven Mecha-to-Hitler completion in the
original map containing all five Fake Hitlers. Fake Hitler's health,
specialized thinker, eight-flame burst and flame collision are also covered
separately by synthetic tests; the SDL test does not require killing every
Fake Hitler before the final boss, nor does the original victory condition.
No encounter or victory marker is removed, replaced or skipped for map
validation. Loading a finale does not automatically complete it.

## Ported functions and rules

### Actors and combat

`WL_ACT2.C` supplies the complete non-SPEAR state tables for Hans, Gretel,
Schabbs, Giftmacher, Fettgesicht, Fake Hitler, Mecha Hitler, Hitler, the four
ghosts, needles, flames, rockets, smoke, explosions and BJ. Tests compare all
46 Part 1 definitions and all 139 Part 2 definitions directly with DOS source,
including shapes, rotation, tics, thinkers, actions and successors.

Original spawn codes are 214 (Hans), 197 (Gretel), 196 (Schabbs), 215 (Gift),
179 (Fettgesicht), 160 (Fake Hitler), 178 (Mecha Hitler), and 224–227
(Blinky/Clyde/Pinky/Inky). Initial facing, ambush/shootable flags and the
four-difficulty health tables are retained. Hitler after transformation has
500/700/800/900 health. Ghost spawning intentionally does not assign health
or shootability, despite the unused ghost hit-point table entries.

Ported/extended routines include `SpawnBoss`, `SpawnGretel`, `SpawnGhosts`,
`SpawnSchabbs`, `SpawnGift`, `SpawnFat`, `SpawnFakeHitler`, `SpawnHitler`,
`FirstSighting`, `SightPlayer`, `T_Chase`, `T_Schabb`, `T_Gift`, `T_Fat`,
`T_Fake`, `SelectRunDir`, `DamageActor`, `KillActor`, `A_HitlerMorph`,
`A_MechaSound` and `A_Slurpie`. Shared algorithms remain from `WL_STATE.C` and
`WL_AGENT.C`. Specialized dodge/flee/attack logic is not replaced by guard AI.

Hans retains his shot-distance accuracy advantage; Gretel uses the original
ordinary shot calculation. Boss burst lengths, damage, score and drops
remain original. Fake Hitler awards 2,000 points, other bosses 5,000, and
Hans/Gretel drop the gold key. Mecha's death leaves its corpse and spawns the
original Hitler state at its physical position, inheriting distance,
direction and flags and consuming the original initial-state RNG draw.
Mecha and Hitler each award their original points/kill count; no extra
`killtotal` is invented for the transformation.

### Projectiles and actor ownership

`T_SchabbThrow`, `T_GiftThrow`, `T_FakeFire`, `T_Projectile`,
`ProjectileTryMove` and `A_Smoke` retain original targeting, speed,
original float angle quantization, signed-magnitude fixed-point movement,
wall/static/door collision, strict
player-hit square, damage RNG and state transitions. Needles/rockets move at
`0x2000`; flames at `0x1200` on state actions, rather than every tic. Rockets
emit smoke and explode on walls. Player hits remove the projectile, as in
DOS; no invented explosion splash damage is added. Rocket rotation uses its
angle rather than its `nodir` direction.

`WL_PLAY.C:GetNewActor/RemoveObj` is adapted to the existing bounded array
and original linked iteration order. Runtime objects append to the live
list for simulation and drawing; removed projectile/effect slots are reusable.
Patrol spawning releases its birth-cell mark before reserving its first move. Array `objcount` is a
high-water mark, not a live-object count. Pool exhaustion reports an error
through the existing game-error/menu path, replacing the original fatal
`Quit`, without corrupting the list or silently losing a required morph.
The killer's position is snapshotted before its projectile slot can be reused.

### Victory, cameras and progression

`WL_AGENT.C:VictoryTile/VictorySpin` and `WL_ACT2.C:SpawnBJVictory`,
`T_BJRun`, `T_BJJump`, `T_BJYell`, `T_BJDone`, `A_StartDeathCam` and
`CheckPosition` are ported into resumed, bounded steps. Exit-plane code 99
is recognized as a movement trigger. BJ runs the original six path choices,
jumps, yells, waits in the final frame and sets `ex_victorious`.

Death cameras preserve the initial 100-tic hold, grey fizzle, original message,
300-tic acknowledgement timeout, original angle/position search, death-state
replay and final 100-tic hold. Grey and viewport replay fizzles use the
original 70- and 20-frame candidate budgets and LFSR respectively. Their
bounded full traversals take 144 and 41 simulation tics. The unacknowledged
initial camera setup therefore takes 585 tics. The blinking original
`SPR_DEATHCAM` overlay uses a replay counter following the original
post-fizzle clock reset. Cinematic input cannot move/use/fire, and a death
camera does not run BJ's spin or consume face-look RNG.

`WL_INTER.C:Victory` draws the original victory artwork, eight-floor average
ratios, summed time, 99:99 clamp and medium/hard verification code. The HUD
retains the final score. `WL_TEXT.C:EndText/ShowArticle/PageLayout` renders the
actual six external end articles with original artwork, colors, word wrapping,
picture margins, borders and page numbering. Arrows, Enter and Page Up/Down
navigate; Escape closes the article, followed by scores and menu.

`GameStep` distinguishes ordinary/secret exits, boss/BJ victory and player
death/restart. `NextMap` never maps a finale to another episode or the secret
floor; finale victory is handled separately. Existing secret-floor return
routes `{1,1,7,3,5,3}`, ordinary inventory carry, key reset, bonuses and
same-floor death restart remain tested. All 60 original per-map ceiling
colors are now selected from `WL_DRAW.C:vgaCeiling`.

### Sound and RNG

The existing synchronous sound-event hook now includes original new boss
sight/death/attack sounds, mecha footsteps, slurpie, projectile launch/impact
and BJ yell. Original non-positional boss sounds use NULL actor. No SDL audio
backend is initialized. Hook consumers must not retain actor pointers or
mutate simulation.

Registered WL6 death-scream selection still consumes its original RNG even
without a sink, including the rare extra draw on **absolute map index 9,
E1M10**, rather than every episode's secret floor. Original Schabbs/Hitler
immediate death-sound calls and their later state actions remain. The four
sound-dependent boss `die2` delays use the original **digitized-audio-off
five tics**, consistent with the current audio-free profile; Milestone 6B
must restore the original 140-tic selection when digitized audio is enabled.

## Files modified

- New `src/engine/WL_ACT2.c/.h`: remaining boss/projectile states and routines,
  fleeing, morph, effects and sprite requirements.
- New `src/engine/WL_VICTORY.c`: BJ, death-camera timing/placement, fizzle and
  replay lifecycle.
- New `src/engine/WL_TEXT.c`: bounded original ending-article layout/navigation.
- `WL_OBJECT.c`, `WL_STATE.c`, `WL_ACTOR.h`, `WL_DEF.h`: spawning, actor
  allocation/removal, enemy integration, sound events and required state data.
- `WL_AGENT.c`, `WL_COMBAT.c`, `WL_DRAW.c`: victory tiles, stable killer data,
  rocket orientation, camera overlay and original map ceiling colors.
- `WL_PLAY.c`, `WL_GAME.c`, `WL_INTER.c`, `WL_MAIN.c`: cinematic/progression
  flow, player control/RNG rules, original statistics and frontend return.
- New `tests/finale_tests.c`, `tests/finale_sdl_tests.c`; updated
  `campaign_tests.c`, `combat_tests.c`, `game_fixtures.py`, `test_engine.py`.
- README, BUILD.md, PORTING.md, MILESTONE6.md and this report: verified scope.

## Validation and playthrough coverage

Commands are in [BUILD.md](BUILD.md):

```sh
./build/tools/cart build --profile cart-sdl
WOLF3D_DATA="$HOME/.steam/steam/steamapps/common/Wolfenstein 3D/base" \
  python3 -m unittest discover -s tests -v
```

Verified results on the final code:

| Check | Result |
| --- | --- |
| CART `cart-sdl` Linux build | Pass |
| Standard owned-data Python/C/SDL suite | 45/45 pass |
| ASan/UBSan application and C tests, same owned-data suite | 45/45 pass; no sanitizer diagnostics |
| Owned map/difficulty sweep | 240/240 load, render and run 140 actor tics safely |
| Native Linux SDL software-renderer finale harness | All six medium-difficulty controlled weapon/completion paths pass |
| MinGW C11 strict compile-only checks | All 31 application modules pass |
| Source-table comparisons | All 185 special/projectile/victory definitions match the original static tables |
| Shutdown ownership checks and Git whitespace checks | Pass |

The regression suite has 45 Python cases, including strict `-Werror` C tests.
The same suite passes with the combined ASan/UBSan application and C-test
procedure. The final sanitizer executable was built with the documented flags
as `build/checks/wolf3d-revive-part2-final-asan` and selected through
`WOLF3D_BINARY`. The reused SDL static archive itself is not instrumented.

Coverage includes:

- All 240 owned map/difficulty combinations: spawn, required sprite decoding,
  initial render, 140 actor tics, periodic rendering and resource cleanup.
- All boss spawns/health/facing, source-table equality, specialized attacks,
  flame emission cadence, projectile-wall/player/door/static collision,
  original damage RNG and rocket effects/removal.
- Morph position/flags/distance/direction/health/RNG and preserved mecha
  corpse; 1,000 effect-slot reuse cycles, exhaustion, killer-slot reuse, stable
  sprite order after reuse and released patrol birth-cell reservations.
- All four death-camera bosses, exact hold/fizzle budgets, frozen camera and
  input/RNG behavior, BJ animation/yell, all six articles and frontend return.
- Average-ratio truncation, total-time arithmetic/clamp and difficulty code,
  death/restart on a boss floor, secret routes, missing boss/projectile/BJ/
  overlay sprites, malformed article headers/end markers and existing corrupt
  maps/archives, limits, combat, input, rendering and Git safeguards.
- MinGW C11 compile-only checking for all 31 application modules. Windows
  linking/runtime remain unverified without a Windows SDL SDK/runtime.

**Controlled gameplay coverage:** `finale_sdl_tests.c` loads every owned finale
without removing actors. It opens nearby doors through the existing door API,
positions the player, aims and delivers actual SDL Ctrl input through the
existing chaingun/combat simulation at the original medium difficulty.
Health and ammunition are refreshed; spawn/health tests and the map sweep
cover all four difficulties.
It verifies boss death, the original Mecha-to-Hitler transition, collected gold
keys and exit tiles where applicable, complete cinematics, statistics, both
article pages, scores and menu. Initial floor ratios are zero in these tests
because preceding floors were not played. It does not claim those zero
statistics represent a full episode run.

Native Linux desktop software-renderer checks use the same harness. Captures
of all six victory statistics and all twelve original ending pages were
visually inspected. Additional boss, morph and replay captures remain under
ignored `build/checks/milestone6a-part2/`; no commercial files are in Git.

**No complete navigated floor or episode playthrough was performed.** Tests
establish controlled combat/completion paths and bounded simulation, not
unassisted player navigation through six campaigns.

## Differences and remaining work

- The established fixed one-tic 70 Hz loop remains. DOS variable-tic batching
  can change probabilistic AI/RNG ordering. No DOS recording, bit-perfect,
  demo-perfect or pixel-perfect equivalence is claimed. Existing ray/scaler
  adaptations and original baseline caveats remain.
- Runtime pool exhaustion becomes a recoverable explicit game error. Killer
  snapshots, bounded camera searches, sprite/map checks and clipped fizzle
  writes replace unsafe DOS pointer/border behavior.
- The Part 1 ghost closed-door wait adaptation remains: a negative movement
  distance waits through the existing door API instead of snapping through a
  closed door.
- Internal statistics/article boundaries draw on the next frame instead of
  reproducing every original palette fade. Existing intermissions still use
  their immediate-final-results path; count-up/BJ breathing and initial-level
  preload/fizzle presentation are not expanded here.
- Article layout targets the six registered WL6 end articles. They use page,
  color and graphic commands. Bounded location/bar/comment commands are also
  supported; timed-picture commands are rejected and are not present in any
  of these six articles. This is not a generalized modding/text engine.
- Qualifying high-score name entry/persistence, audio/music, saves, demos,
  Windows runtime verification and full navigated playthroughs remain. Audio
  is Milestone 6B; save/load and Enhanced Mode remain later work.
