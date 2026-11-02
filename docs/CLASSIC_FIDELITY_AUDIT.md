# Classic DOS fidelity restoration (work in progress)

Goal: Classic follows the original 1992 DOS Wolfenstein 3D behaviour; Enhanced may add conveniences without changing Classic.

## Implemented on this branch

- Restore in-game F2 Save, F3 Load, F4 Sound, F5 view/display and F6 Controls entry points. Original DOS dispatch is in `WOLFSRC/WL_MENU.C` (`US_ControlPanel`). F5 currently opens Revive's SDL display panel, **not** the original DOS view-size panel; therefore this is not yet fully faithful.
- Preserve current save eligibility and SDL keyboard input boundary. No gameplay timing or data formats changed.

## Significant remaining gaps: do not claim complete Classic fidelity

1. **Original variable 3D view size and teal border.** `src/engine/WL_DEF.h` fixes `VIEWWIDTH=240`, `VIEWHEIGHT=120`, `VIEWX=40`, `VIEWY=20`. Original DOS `CP_ChangeView` uses `viewwidth`/16 and `NewViewSize` (see `WOLFSRC/WL_MENU.C`). Restoring this requires dynamically sized ray columns, projection/pixel-angle tables, wall/sprite/HUD drawing, death animation, and careful on-screen clipping. Do not merely scale the 240x120 image.
2. **Original F1 Help and F7-F10 quick control panel actions.** DOS implements these in `WOLFSRC/WL_MENU.C`; they require proper pause/confirmation and game-state integration.
3. **Original attract-mode demos.** Current `src/engine/WL_MAIN.c` rotates title, credits and highscores, whereas the DOS `WOLFSRC/WL_GAME.C` includes demo recording/playback. Gameplay/demo tick sequencing and assets need parity tests.
4. **Exact original view-size UI.** In Revive `changeview` opens the modern display settings screen. Classic should instead present the DOS change-view workflow, with OS window/scaling controls separated.
5. Audit high-score entry/persistence, help content, cheats/debug flags, keyboard edge cases, menus and effects against the corresponding `WOLFSRC` paths.

## Test plan

- Build with CART and run the full owned-WL6 regression suite, sanitizer suite and MinGW cross-build.
- Add tests pressing F2-F6 during gameplay and verifying the resulting UI screen, including unavailable save directories, faded transitions, pause/resume and load.
- For dynamic viewport work, compare every original view size with DOS source formulas, projections, sprite clipping, status-bar boundary, save/load continuation, death effects, and menu restoration.
- Never alter `WOLFSRC/`, redistribute commercial assets, or claim real hardware parity from automated tests alone.
