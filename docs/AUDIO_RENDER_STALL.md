# Audio buffer experiment for higher-resolution frame stalls

The current `master` remains a 320x200 game renderer. High-resolution
rendering is not part of this change. In `sdl_audio.c`, SDL queues audio
asynchronously but the main thread replenishes it once per rendered frame.
The previous 24 ms queue target could underrun when a frame took longer
than that. This branch increases the bounded target to 80 ms, still filling
in 384-frame (8 ms) synthesis chunks.

This is a pragmatic **experiment**, not a verified fix for a reported
high-resolution audio bug. It may reduce crackle during intermittent slow
frames; it adds approximately 56 ms of extra buffered output compared with
the previous target. Long freezes can still underrun. Audio is not generated
from render frame timestamps; game simulation, 70 Hz logic and RNG are not
changed.

Before merging, build and run the audio suite, full game regression,
ASan/UBSan, and manual listening on Linux and Windows with both fast and
slow rendering loads. Check music continuity, latency of gunshots and
menu sounds, focus/unfocus, mute, pause, and startup/shutdown. Compare
24 ms and 80 ms behaviour in an actual high-resolution rendering build,
if available. Revert or tune if latency becomes unacceptable. Do not
modify `WOLFSRC` or merge without verification.
