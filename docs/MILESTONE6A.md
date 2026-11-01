# Milestone 6A — first pass: Hans, Gretel and ghosts

2026-11-02. **The original campaign is still incomplete.** This pass ports two
bosses and four ghosts, improves sound-event/RNG fidelity and guards finale
routing. It does not implement projectile bosses, BJ victory actors, death
cameras or episode endings. No complete episode playthrough is claimed.

## Baseline and source review

The working tree was clean. `docs/MILESTONE5.md`, `docs/MILESTONE6.md`, the
active engine and the corresponding original actor, movement, map, combat and
progression code were reviewed before editing. The initial CART build passed;
all 37 Python regressions passed with owned Steam data. The owned-map sweep
confirmed 204 successful and 36 explicitly rejected map/difficulty combinations.
Existing movement, doors, sprite rendering, weapons, pickups, RNG, pools and
70 Hz state execution were reused.

`WOLFSRC/` is unchanged. Code is ported from id Software's original sources,
with the existing GPL-2.0-only engine licensing and provenance described in
[LICENSING.md](LICENSING.md). No third-party port, assets or dependencies were
imported. No CART or Ironclaw changes, commits or pushes were made. Upstream
continues to have the disabled push URL.

## Implemented behavior and original references

- `WL_ACT2.C` Hans and Gretel state tables: all 19 states each, including
  stand, six chase states, eight shooting states and four death states.
  Original shapes, non-directional rotation, thinkers/actions, successors and
  durations are retained. The burst waits 30 tics, fires six times at ten-tic
  intervals and returns to chase after 100 tics. The corpse is reached after
  45 death-animation tics. Neither boss has a pain state.
- `SpawnBoss` / `SpawnGretel`: codes 214 / 197, south / north initial facing,
  patrol speed 512, shootable/ambush flags and health 850/950/1050/1200 for
  baby/easy/medium/hard. `WL_STATE.C:SightPlayer` retains the one-tic boss
  reaction delay. `FirstSighting` selects the original chase state and speed
  1536. Hearing does not bypass ambush sight checks.
- `T_Chase`, `T_Shoot`, `DamageActor`, `KillActor`: the existing original
  algorithms now select the boss states. Hans retains his distance-times-2/3
  accuracy advantage; Gretel uses the original ordinary shot calculation.
  Surprise damage, difficulty damage scaling and player movement effects
  reuse the existing combat rules. Each kill awards 5,000 points, increments
  the kill count, removes shootability and drops the gold key at the physical
  death position. Death does not itself complete these episodes.
- `SpawnGhosts`, ghost states and `T_Ghosts`: codes 224/225/226/227 select
  Blinky/Clyde/Pinky/Inky, each with its own two ten-tic animation states.
  Ghosts start facing east at speed 1500 with the ambush flag. Original
  spawning does **not** assign shootability or health, despite the unused
  ghost entries in `starthitpoints`; these are not made killable here.
  They contribute to `killtotal`, as in DOS. Shared direction selection,
  occupancy, walls and movement remain; `WL_STATE.C:MoveObj` now restores
  their two-points-per-tic player contact damage and movement rollback.
- `A_DeathScream`, `FirstSighting`, `T_Shoot`, `T_Bite`: minimal synchronous
  sound hooks carry the original `AUDIOWL6.H` sound identifiers and actor
  position, or NULL for original non-positional boss sounds. No audio system
  is initialized. The optional hook must not mutate simulation or retain an
  actor pointer. With no hook installed the original RNG draws still occur.
- Registered/non-SPEAR `A_DeathScream` now includes the rare death sound on
  **absolute map index 9 (E1M10)**. It consumes a draw for every death scream
  there, including bosses; eligible normal actors return early on a zero
  draw. Guards otherwise consume their sound-selection draw. Other episodes'
  tenth floors do not acquire that extra draw. This follows ID_CA's global
  `mapon`, rather than confusing it with episode-local `gamestate.mapon`.
- `NextMap` rejects floor nine (local index 8), invalid selections and a
  non-exit play state with sentinel 60. It no longer interprets an ordinary
  finale exit as entry to the secret floor. Ordinary exits through floor
  eight and all six existing secret return routes remain intact. Finale
  presentation remains explicitly unavailable.

## Files changed

- `src/engine/WL_STATE.c`: original state tables, AI/combat integration, ghost
  movement/contact and required sprite validation.
- `src/engine/WL_OBJECT.c`: bounded special spawning and sound-event handling.
- `src/engine/WL_ACTOR.h`, new `WL_SOUND.h`: thinker and sound-hook declarations
  and original sound indices, without audio assets or playback code.
- `src/engine/WL_GAME.c`: guarded final-floor routing.
- `tests/boss_tests.c`, `tests/boss_sdl_tests.c`, `tests/game_fixtures.py`,
  `tests/test_engine.py`, `tests/campaign_tests.c`: synthetic encounters,
  source-table comparison, state/combat/RNG/limits/SDL and owned-map coverage.
- `README.rst`, `docs/BUILD.md`, `docs/MILESTONE6.md` and this document: current
  scope, validation and remaining work.

## Supported maps and validation scope

All 60 original maps are checked at every difficulty, **240 combinations**.
**54 maps load/render at every difficulty (216 combinations); all six finale
maps remain explicitly rejected (24 combinations).** Newly supported maps:

| Display name | Absolute zero-based index | Newly supported encounters |
| --- | --- | --- |
| E2M8 | 17 | Two Hans actors |
| E3M10 | 29 | All four ghosts |
| E6M10 | 59 | Three Hans actors and Blinky |

Still rejected: E1M9 (BJ victory tiles), E2M9 (Schabbs), E3M9 (Mecha Hitler /
Fake Hitler), E4M9 (Giftmacher), E5M9 (BJ victory tiles / Gretel), E6M9
(Fettgesicht). Gretel's actor implementation is validated with synthetic maps;
her real finale is not reported as supported merely because she can spawn.
No encounter is removed or replaced to make an original map load.

The automated owned sweep also advances the three newly supported maps for
140 actor tics per difficulty and renders periodically. This tests bounded
runtime behavior; it is not player navigation or a playthrough.

## Verification

Commands follow [BUILD.md](BUILD.md): CART build, the Python suite with
`WOLF3D_DATA`, and the combined ASan/UBSan application build followed by the
same suite with `SANITIZE=1`. The SDL static archive is not instrumented.

Actual results:

- CART `cart-sdl` Linux build passes.
- All **42 Python regression cases pass**, including strict `-Werror` C
  executables and synthetic/owned SDL encounter checks.
- All **42 cases also pass with ASan/UBSan** on the application and C tests,
  with the same owned data. No sanitizer diagnostics were reported.
- The owned-data sweep evaluates all 240 combinations: 216 load/render
  successes and 24 expected explicit finale rejections. Each newly supported
  map/difficulty also passes 140 actor tics with periodic rendering.
- Native desktop SDL software-renderer E2M8/E3M10 encounter checks pass.
  Four local captures were visually inspected: Hans standing/firing,
  original weapon/HUD, ghost standing/chasing, wall clipping and damage
  palette flashes. The native check uses the same controlled encounter
  setup described below; it is not a human playthrough.
- `git diff --check` passes. No `WOLFSRC/` changes or commercial files appear
  in the Git changes. No complete level or episode playthrough was performed.

Regression coverage includes all six new spawn codes at all difficulties,
original ghost spawn RNG, boss sight/reaction/speed, six-shot burst timing,
Hans/Gretel accuracy differences, death animation and sound, points and gold
key collection, ghost state cycles/contact/collision/door waits, sound RNG
with and without a sink, missing offscreen special sprites, 150-actor pool
success and 151-actor rejection, invalid boundary spawns and final-map
routing. A separate oracle compares every field of all 46 new state
definitions directly against `WOLFSRC/WL_ACT2.C`. Existing malformed archive,
combat, sprite limits, input, progression, cleanup and Git-protection tests
remain in the suite. Shutdown checks require `MM_TotalBytes() == 0`.

The SDL encounter regression uses synthetic data and owned E2M8/E3M10. It
positions the player for controlled encounters, opens nearby E2M8 doors via
the existing door API, delivers real SDL Ctrl events, verifies weapon damage,
ammunition and boss return fire, and simulates/render-checks ghost pursuit.
It refreshes player health during the encounter. It is not an unassisted
fight, a navigated level or a full episode playthrough. Commercial files are
read in place; captures stay under ignored `build/checks/milestone6a/`.

## Differences and unfinished work

The existing fixed one-tic 70 Hz loop, DOS variable-tic/RNG sequence caveats
and rendering differences remain as described in milestones 5 and 6. No
bit-perfect, pixel-perfect or demo-perfect DOS equivalence is claimed.

One deliberate safety adaptation is added to `T_Ghosts`: the original thinker
has no negative-distance door wait, although `TryWalk` can reserve a closed
door. This port waits through the existing door API, as `T_Chase` does,
rather than subtracting a negative distance and snapping through the door.
Door waits are regression tested. Other ghost chase and contact rules remain
original.

Remaining Milestone 6A work:

1. Schabbs, Fake/Mecha/real Hitler, Giftmacher and Fettgesicht, including all
   projectile states, reusable bounded runtime actor allocation, collision,
   damage, effects, morphing, timing and sound hooks.
2. BJ victory tile/movement/animation, death cameras, episode statistics and
   original narrative/victory screens. Hans/Gretel finale maps remain blocked
   until this works. Do not substitute an elevator intermission for victory.
3. Complete all 240 map/difficulty combinations successfully, then validate
   combat and actual navigated episode playthroughs, secret routes and final
   sequences. The current 216 successes are load/render validation only.
4. Broader original gameplay sound hooks where calls are still absent. Audio,
   saves and Enhanced Mode remain reserved for later milestones.

Windows linking/runtime and DOS equivalence were not tested in this pass.
