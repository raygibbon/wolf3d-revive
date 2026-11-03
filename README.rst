Wolf3D Revive
=============

**A faithful, modern C11 port of the original Wolfenstein 3D for Windows and
Linux, built with SDL3 and CART.**

Wolf3D Revive aims to preserve the original game's look, timing and behaviour,
while making it comfortable to play on modern systems. It is a preservation-
focused source port, **not** a remake or a new game engine.

What works
----------

* The original six-episode campaign systems: enemies, bosses, weapons, pickups,
  doors, secrets, intermissions and endings.
* Original-style AdLib music, sound effects and digitised audio read from game
  archives, without converting the soundtrack.
* Classic keyboard controls or optional modern WASD and mouse controls.
* Six named save slots, configurable key bindings and persistent settings.
* Original 320x200 presentation with correct 4:3 display proportions, view
  sizes, HUD and palette.
* Optional high-resolution software rendering, expanded widescreen views,
  fullscreen, display scaling, VSync and screenshots.
* Optional **Smooth Rendering**: interpolated presentation at the display's
  refresh rate while keeping the original **70 Hz game simulation**.

The original renderer remains the default. High-resolution rendering is an
optional presentation mode, not a different game. The active main branch
supports automatic high-resolution render targets up to 4096x2160; further
4K performance work is being developed separately.

Getting started
---------------

You must provide your own legally obtained **Wolfenstein 3D WL6 game data**.
The repository does not distribute commercial game assets. A Steam installation
is one possible data source.

The project uses CART and SDL3. Once your local CART/SDL3 environment is set up,
a typical Linux build and launch is::

    ./build/tools/cart build --profile cart-sdl --verbose
    ./build/tools/cart run --profile cart-sdl -- --data "/absolute/path/to/WL6"

The ``cart-sdl`` profile expects the SDL3 SDK in the sibling CART checkout.
There is no automatic game-data download. For Windows builds, alternative SDK
layouts and troubleshooting, see `Build instructions <docs/BUILD.md>`_.

Controls
--------

Classic controls are the default: arrow keys move/turn, Ctrl fires, Space
operates doors, Shift runs, and number keys select weapons. Select **Modern**
in the Control menu for WASD movement and mouse aiming.

Useful shortcuts:

* **Alt+Enter:** toggle fullscreen.
* **F11:** toggle integer display scaling.
* **F12:** save a BMP screenshot.
* **Escape:** open the in-game menu or pause.

Controls, audio, display choices and view settings are configurable in-game.
The configuration and save files live in the platform's SDL user-data directory.

Fidelity and project scope
--------------------------

The project retains the original DOS source in ``WOLFSRC/`` without modifying
it. The maintained port lives in ``src/``.

All 60 original maps have been exercised in automated load/render/simulation
coverage at four difficulties. The four attract-mode demos match an instrumented
DOS reference under documented test settings. These results **do not** claim
that full human-played episode runs or every audio/device combination have
been verified.

The goal is to preserve gameplay and DOS compatibility while improving the
platform and presentation layers. Modding frameworks, replacement art and
major gameplay redesigns are outside the current scope.

Documentation
-------------

* `Build, run and controls <docs/BUILD.md>`_
* `Milestone 8: frame pacing and benchmarks <docs/MILESTONE8.md>`_
* `Milestone 7: widescreen and high-resolution rendering <docs/MILESTONE7_VISUAL.md>`_
* `DOS demo synchronization <docs/MILESTONE6G.md>`_
* `Classic fidelity audit <docs/CLASSIC_FIDELITY_AUDIT.md>`_
* `Configuration details <docs/MILESTONE6F.md>`_
* `Porting notes and progress <docs/PORTING.md>`_
* `Licensing and data ownership <docs/LICENSING.md>`_
* `Historical source-release README <docs/HISTORICAL_README.rst>`_

License and attribution
-----------------------

The active port's new C engine code is licensed under **GPL-2.0-only**; see
`COPYING <COPYING>`_ and `docs/LICENSING.md <docs/LICENSING.md>`_ for
provenance, third-party notices and the distinction between engine source
and copyrighted game data. Original game assets and trademarks remain their
respective owners' property.

This project is an independent preservation effort, not an official id Software
release.
