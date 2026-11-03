# Milestone 10: Restore the original DOS menus and presentation

**Status:** Rework specification for Codex; implementation not yet complete.

## Goal

Restore the original Wolfenstein 3D DOS interface and startup presentation
from the user's legally owned WL6 archives. Modern options remain available,
but they must use the classic menu language. **Do not change game HUD,
simulation, original 70 Hz timing, demo sync, or enhanced-world rendering.**

## User-provided visual references

- DOS sound-effects menu: red panel, original pistol selector, red/yellow
  toggle graphics, gray pixel type, three sections.
- DOS Control menu: centered original Control heading, mouse/joystick controls,
  gray active text, dark red disabled entries, pistol cursor, help legend.
- Original startup/intro reference: PC-13 cyan screen, Wolfenstein logo and
  original imagery where data and licensing permit it.

These reference screenshots exist in the conversation only. They are **not**
redistributable assets to commit to Git. Extract needed graphics from original
owned archives at runtime using existing asset loaders. Do not generate or
copy substitute logos or commercial artwork.

## Non-negotiable presentation rule

The menu/startup framebuffer remains exactly **320x200 indexed pixels**.
DOS VGA non-square-pixel correction presents it as **4:3** (320x240 logical
presentation). Scale **uniformly** with aspect-preserving pillarbox/letterbox
and appropriate black borders. On a 1920x1200 (16:10) drawable this means a
1600x1200 menu with 160-pixel side borders. Do **not** stretch the source
image horizontally to fill a 16:10, 16:9 or ultrawide display. Do not modify
or reposition individual graphical elements based on monitor width.

Enhanced world rendering may still fill the user's selected 16:10 or other
supported aspect, while menus and static intros remain 4:3, including when
entered from the game. Classic world gameplay remains unchanged.

Use the existing SDL3 `SDL_SetRenderLogicalPresentation` path where possible;
avoid a new renderer or unnecessary texture, shader or scale setting.

## Rework areas

1. `src/engine/WL_MENU.c`: restore source-authentic panel composition,
   gun cursor/selection feedback, inactive items, stripes and menu artwork.
   Main-menu navigation and save/pause behaviour must stay functional.
2. `src/engine/WL_CONTROLS_MENU.c`: reproduce the original DOS Control
   hierarchy/visual composition where feasible. Keep existing Modern/Classic
   profiles, sensitivity, invert, remapping and Display settings accessible
   in clearly styled subordinate menus; don't silently remove functions.
3. `src/engine/WL_MENU.c` sound page: use classic sound effects/menu
   controls and graphics, but don't offer unsupported PC speaker hardware
   or fake sound modes. Explain modern audio options in DOS styling.
4. `src/engine/WL_MAIN.c`: review startup/attract sequence against the
   original DOS sequence and preserve authentic title/credits/high-scores
   screens. Any PC-13 / sign-on screen must come from verified original
   archives and be shown only when available. Do not mistakenly claim the
   separate historical DOS SETUP.EXE is a mandatory startup screen.
5. `src/platform/sdl_video.c`: verify all menu, title, credits, save,
   control, sound and intro states use fixed 320x240 logical display with
   black bars. The widescreen high-resolution renderer is used only for
   gameplay/world presentation.
6. `tests/`: add focused image/hash/reference checks and 4:3 aspect checks
   on 16:10, 16:9 and ultrawide synthetic window sizes; exercise UI state
   transitions, menu navigation, scaling and return to enhanced gameplay.

## Architecture and protection

- Use original graphics **already in WL6 game archives** where available,
  and the existing `VWB_DrawPic`, `CA_CacheScreen`, original fonts and
  palette/fade code. Inspect original `WOLFSRC/WL_MENU.C` for coordinates,
  structure and intent **without modifying WOLFSRC**.
- Keep enhancements behind presentation logic. Never break save format,
  settings, key mappings, accessibility or input handling.
- Use separate screen compositions for original menus versus new options,
  rather than squeezing many modern rows into a tiny original panel.
- Preserve original red panel, gun selector, red toggle/yellow indicator,
  inactive colour treatment, title plaque and bottom instruction legend.
- Do not add third-party icon packs, new licensing dependencies or a UI
  framework.

## Acceptance

- Compare reference DOS screenshots against an in-engine 320x200 source
  framebuffer with known owned assets. Prefer exact comparisons for
  original screens and targeted visual review for new option pages.
- On 1920x1200 output, confirm 4:3 menu content is 1600x1200 centered with
  160px black bars on each side; no horizontal distortion.
- Confirm the same 4:3 rule on 1920x1080 and 3440x1440 and during
  fullscreen/window switches. Pixel output and window sizes must be
  distinguished in high-DPI scenarios.
- Confirm enhanced gameplay returns to full configured aspect after menus.
- Verify Classic framebuffer hashes, menu selection, control bindings,
  save/load, audio and all four DOS demo traces remain unchanged.
- Run C11 strict warnings, automated suite and sanitizer checks with
  owned data; if unavailable, disclose blockers.
- No commercial data files, screenshots or audio embedded in Git.

## Deliverable

Implement in small commits on this feature branch, update this document
with what was actually changed and tested, and open a PR to `master`
without merging it automatically.
