# Classic DOS fidelity restoration

Classic targets the original DOS rules; this branch does **not** claim complete
DOS equivalence. The initial restoration was `feature/classic-fidelity-restoration`,
PR #5. Current completion work is on `feature/12-dos-fidelity-completion`;
see the [Milestone 12 checklist and verification matrix](MILESTONE12_DOS_FIDELITY_BACKLOG.md)
and [initial implementation and verification](CLASSIC_FIDELITY_RESTORATION.md).

## Restored in this pass

- Variable-size 3D viewport: all original sizes 4–19, default 15, using
  `WL_MAIN.C` NewViewSize/CalcProjection formulas. Rays, wall/sprite scalers,
  weapon placement, combat view thresholds and death effects use active sizes.
- Original teal play border and bevel (`WL_GAME.C` DrawPlayBorder), including
  safe restoration around the viewport, HUD, menus and save/load.
- DOS Change View preview, arrows, bounds, Enter acceptance and Escape cancel
  (`WL_MENU.C` CP_ChangeView). Native SDL display controls are separate, under
  Control → Display settings.
- F2 Save, F3 Load, F4 Sound, F5 Change View and F6 Control now return directly
  to paused gameplay. The initial branch compiled and passed 70 regressions,
  but its F-key panels returned to the main menu and F5 opened SDL settings.
- F1 implements the **GOODTIMES boss key**, matching `VERSION.H` and
  `US_ControlPanel`: a native black “C>” screen, paused music/gameplay, Escape
  to return. The non-GOODTIMES HelpScreens path is a different release variant.
- F7 End Game confirmation, F8 quicksave/initial slot selection, F9 quickload
  confirmation/initial slot selection and F10 Quit confirmation, following
  `CP_CheckQuick`. Function keys are reserved from gameplay bindings.
- Original embedded `T_DEMO0`–`T_DEMO3` command playback in the attract cycle,
  with bounded archive decoding, fixed RNG seed, signed movement, original
  buttons, four-tic updates and key/click abort. Missing/corrupt/unsupported
  demos are skipped; the attract loop continues instead of fabricating input.

## Remaining fidelity differences

1. **Demo synchronization is verified for the Milestone 6G reference scope.**
   All four recordings match every traced frame through their declared durations
   at view size 15 with DOS sound off. Other releases/view sizes and enabled-DOS
   audio timing need equivalent comparisons. See [MILESTONE6G.md](MILESTONE6G.md)
   for the original executable reference, proven corrections and remaining limits.
2. The GOODTIMES boss screen uses the existing game font in the 320×200 SDL
   framebuffer, rather than switching hardware to BIOS text mode. Help for
   other release layouts is not implemented. The retained owned help article
   contains picture references that do not fit this GOODTIMES picture layout;
   it is not silently remapped or drawn out of bounds.
3. Milestone 12 restores all nine quit sayings with the original C selection
   precedence, confirmation blinking/release/sounds, Pause, new-game refusal,
   high-score qualification/editing/persistence and Saving/Loading pictures.
   End Game now follows final death into qualification and attract mode.
   `WL_GAME.C GameLoop` has **no separate Game Over card** in this edition.
   The 20 executed-DOS dialog frames match pixel-for-pixel. Disk-flip timing
   follows native operation phases, not DOS serialization chunks; UI entropy
   intentionally stays separate from gameplay RNG.
4. Six portable save slots and format version 1 remain; original DOS ten-slot
   files are not introduced. Settings, including view size, now persist separately
   in [Milestone 6F TOML preferences](MILESTONE6F.md).
5. Full VGA/CRT pixel equivalence is not established. Milestone 6G corrects
   actor projection to DOS `centerx=width/2-1` and verifies traced visibility at
   the default size. Safe clipping of death-fizzle edge pixels remains.
6. Milestone 12 adds MLI/BAT, Robert's Jukebox, supported portable debug keys,
   device-capability sign-on lights, DOS mouse navigation and text editing.
   Physical ISA/gameport/EMS/XMS diagnostics and native joystick/gamepad input
   remain unsupported. Non-GOODTIMES/SPEAR/localized menus need matching data
   and separate verification. Full physical DOS audio/CRT parity remains open.
   Classic/Modern are input presets; Enhanced is a presentation/renderer mode,
   sharing the original simulation. No separate gameplay mode was introduced.

## Protection and verification

WOLFSRC, master and commercial game files are unchanged. All generated data,
private captures and binaries remain outside tracked source assets. Tests cover
all 16 view sizes, border/HUD/sprite/death clipping, save/RNG isolation, function
keys, pause/return, quick-operation errors, demo headers/cadence/input isolation,
missing/corrupt streams and owned data. Full regression, sanitizer and available
Windows results and hardware limitations are recorded in
[CLASSIC_FIDELITY_RESTORATION.md](CLASSIC_FIDELITY_RESTORATION.md).

Milestone 12 additionally covers editor repeats, atomic high-score files,
original-DOS dialog pixels under DOSBox, native 1920×1200 aspect readback and
strict Linux/MinGW builds. Current full-suite results and precise remaining
limits are recorded in the linked milestone document.

Final Milestone 12 validation passed 146 normal and 146 ASan/UBSan tests without
skips or findings, plus the final completion-timing follow-ups. WOLFSRC remains
unchanged; see the milestone matrix for executed-DOS, native display and Wine
results and the physical hardware/edition limitations.

## Milestone 13 audit and release hardening

The [current transition inventory](DOS_STATE_TRANSITIONS.md) accounts for all
native UI states and original exit values, with explicit verification gaps.
[Milestone 13](MILESTONE13_FINAL_FIDELITY.md) records the established-quickload
correction, 30 executed-DOS drawing frames, expanded 21:9/32:9 loading checks
and current release validation. The earlier counts above are historical.
Entry palette fading versus DOS `fizzlein`, safe death-fizzle clipping,
enabled-sound face/RNG behavior and physical audio/display comparison remain
explicit differences or open evidence gaps.
