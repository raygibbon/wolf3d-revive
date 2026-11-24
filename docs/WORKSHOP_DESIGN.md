# Workshop: first-person grid authoring proposal

Status: PROPOSAL. Simple walk-around building, not CAD, not Ironclaw3D's structural/freeform 3D system.

## Product experience

Enter Workshop from game UI. Walk using normal game movement. Aim at a tile/edge, see a clearly coloured valid/invalid preview, click/place or delete. Choose a simple palette: wall, door, floor/wall appearance, decorations, player start, enemy, item and secret. A wall drag creates a whole straight run; a room rectangle creates perimeter walls with a valid interior. Every object snaps to the game's tile grid. No flying, separate top-down CAD tool, freeform mesh sculpting or complex material node interface in v1.

Create -> edit -> save -> playtest -> return -> continue editing. Hot-loading means changes are applied at a safe game/editor boundary or explicitly rebuilt only where needed; no unsafe live mutation during active simulation frames.

## One shared model and API

The same validated world representation and content IDs drive game simulation, renderer, save/load and Workshop UI. Native C operations, independent of SDL input/rendering, should include concepts such as `world_new`, `world_validate`, `world_set_tile`, `world_place_object`, `world_remove_object`, `world_begin_edit`, `world_commit_edit`, `world_undo`, `world_redo`, `world_save` and `world_load`. These are illustrative **proposed API names**, not implemented symbols or ABI promises.

Avoid global editor-only world state. Define object identity and ownership, mutation atomicity, deterministic operation order, versioned formats, error codes and asset resolution. The editor can be temporarily paused for playtest; the authored world is durable and game-session transient state is separate.

## Constraints

- Respect current grid, wall collision and raycasting constraints; do not introduce multi-storey blocks or unconstrained heights merely for editing.
- Mouse/keyboard first. Controller/Android touch editing can be a separate later design with confirmed need.
- Clear affordances: highlight active tool, readable preview, snap target and invalid placement reason; undo is essential before extensive content authoring.
- Validate occupied tiles, doors, spawn reachability, map edges, duplicates, resource limits and unknown IDs.
- Save/load must produce the same semantic map; version migrations must be explicit and tested.
- Workshop is optional to playing the shipped original game.

## Delivery slices

R4 headless edit and transaction semantics -> R5 in-world selection, placement, line/room tools and objects -> R6 atomic persistence, hot-load and return loop -> R7 full gameplay placement, undo UI and new-player usability -> R8 distribution.

## First proving demo

Using original placeholder art, create a small square room, add a corridor and door, place a start and an enemy, save to disk, restart, play it, then return to Workshop and edit a wall. Record deterministic API test, screenshot capture in Classic and Enhanced, invalid-placement cases and an observed hands-on session. Only then broaden the palette.

## Early live-wall proof of concept (experimental branch)

**Branch:** `experiment/live-workshop-wall-placement`. This is a deliberately small vertical slice to demonstrate first-person live creation before full R2–R6 delivery. It is not a replacement for those milestones or a production map format. No formal acceptance is implied until the build and hands-on tests are completed.

**Player-visible goal:** In an already loaded level, toggle Workshop, aim at an adjacent empty tile, see a clear placement preview, place a wall, observe collision/rendering update immediately, remove the wall, and walk through the cleared space. Remain in the same first-person scene throughout. Do not implement a standalone map editor, overhead CAD, or free-flight mode.

**Minimum implementation contract:**

1. Inspect the live map owner, tile storage, renderer, collision, input and main-loop ordering before choosing integration points. No second editor-only tile array. Use existing C11/SDL3/CART code, avoid new dependencies.
2. Provide a small, headless-testable native C mutation API (names are implementation choices) that validates world bounds, legal wall IDs, occupancy, player tile and mutation timing; returns precise failure reasons and leaves state unchanged on failure. Apply mutations only at a safe update boundary, not during rendering or iteration over active simulation data.
3. Workshop toggle and wall-place/remove bindings must not conflict with existing gameplay controls. Preserve normal first-person movement and camera. Use a short, deterministic view-based targeting range and grid snapping; disallow creating walls inside the player or other occupied/protected cells. Never let removal delete immutable level borders or critical existing structures without explicit safety rules.
4. Draw an unobtrusive target/placement cue using the current renderer. Show valid versus invalid placement distinctly and show the mode. Missing or invalid targets should not crash or alter the world. For the experiment, reuse locally loaded existing wall texture IDs without copying commercial data into Git.
5. Make the tile change visible to both render and collision immediately; invalidate only the caches actually present in the engine. Confirm live wall placement blocks motion, removal restores motion, and exiting Workshop returns to ordinary play without a restart.
6. Add a deterministic headless unit/integration test for valid placement and removal, out-of-bounds and invalid tile IDs, occupied cell rejection, repeated operations, and no mutation on failure. Add an interactive smoke-test checklist and Classic/Enhanced display checks. Guard existing game behaviour and record actual test commands and results.
7. No permanent format or PK3 export is required for this prototype. Existing map/asset loader remains unchanged. Persistence, undo/redo, drag walls, full room tools, resource hot reload and ECWolf PK3 export remain staged R2–R8 work. Document explicit technical debt rather than pretending these features exist.

**Compatibility invariant:** The future R2 map contract must support mapping/import/export for a tested ECWolf-compatible subset; PK3 is a ZIP-based container, not the in-memory world representation. A mod referencing player-owned Wolf3D resources must not have to bundle commercial assets. This experimental wall tool must not hardwire Workshop's canonical data representation to ECWolf PK3 internals.

**Prototype exit gates:** (a) manual live wall-place/remove demonstration in the same running game, (b) expected renderer and collision changes, (c) safe mutation API passes headless tests, (d) Classic and Enhanced smoke tests reported, (e) existing validation and CART build statuses recorded, (f) no commercial game data/new binary assets committed. A gate is PASS only with observed evidence; otherwise PARTIAL, FAIL or NOT RUN. Do not merge the experiment merely because it compiles.
