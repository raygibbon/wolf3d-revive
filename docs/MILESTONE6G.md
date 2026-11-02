# Milestone 6G: original DOS demo synchronization

The original owned retail demo recordings load but diverge during replay. Do not
change AI, movement, RNG, demo commands or assets merely to make a recording last
longer. Find the **first** differing frame against an instrumented DOS run.

## Initial branch instrumentation

`Demo_Progress()` exposes the current source map, validated total command count,
consumed command count and last button bitfield. It returns false when no demo
is active, and has no effects on input consumption or simulation state.

## Next work

1. Add a deterministic per-demo trace with command index, four-tic clock,
   player X/Y/angle, health/ammo, random index, actor count and compact stable
   actor-state digest. Do not include machine-dependent pointers.
2. Create equivalent DOS-side traces **outside WOLFSRC** using a temporary build
   patch/instrumented reference; keep the tracked historical tree untouched.
3. Compare commands and initial map/actor layout, then locate the first divergent
   tick. Verify RNG consumption, actor ordering, Think vs movement ordering,
   visibility/activity, recorded button latch semantics and attack sequencing.
4. Add synthetic and owned-data regression assertions, reproduce all four
   owned demo durations, and only alter gameplay after proving equivalence to
   the DOS reference.
5. Run CART, full owned-data tests, ASan/UBSan, MinGW and available Wine checks.
   Do not report full demo fidelity until the original recordings complete.

This branch intentionally adds diagnostics first. It does **not** claim to fix
the original demo synchronization problem yet.
