# Milestone 6B — native SDL3 audio

2026-11-02. Original WL6 music and effects now play directly from the player's
archives through a shared YM3812 synthesiser and SDL3. There is no converter,
PCM asset cache, extracted soundtrack or external playback process. The normal
application selects AdLib effects plus digitised voices, with independent music
and effects volume/mute controls in the existing Sound menu.

This implements the core playback path and campaign integration. It does **not**
complete every presentation detail: the existing immediate-results intermission
still skips animated counting and its counting/percentage sound sequence.
Detailed listening comparisons against DOS and native Windows hardware playback
also remain outstanding. These limits are not covered by a claim of complete
original audio fidelity.

## Baseline and provenance

The working tree was clean. Before porting, the CART build and all 45 existing
regressions passed with owned Steam files. MILESTONE6A.md, MILESTONE6.md,
BUILD.md, LICENSING.md, sound identifiers/hooks, resource handling, SDL lifecycle
and original DOS routines were reviewed. `WOLFSRC/` remains unchanged. No CART
or Ironclaw source was changed, commercial assets added, commit made or push
performed. Upstream pushing remains disabled.

Primary implementation references are the archived `ID_SD.C`, `ID_SD.H`,
`ID_SD_A.ASM`, `AUDIOWL6.H`, `WL_MAIN.C:InitDigiMap`, `WL_GAME.C:SetSoundLoc`,
`WL_PLAY.C:songs`, and original gameplay/menu sound call sites.

## Synthesiser selection and licence

[emu8950](https://github.com/digital-sound-antiques/emu8950) was selected at
commit `f82a22e80b68820f263084aa8506b12eb1c9b4dd`; the source labels itself
v1.2.3. It is a small C implementation with an explicit YM3812 mode, nine
channels, waveform selection, envelopes, feedback and rhythm/percussion.
Its [MIT licence](../third_party/emu8950/LICENSE) permits inclusion in this
GPL-2.0-only engine while preserving its notices. It compiles as C11.

[Nuked-OPL3](https://github.com/nukeykt/Nuked-OPL3) was also reviewed. It is
C, self-contained, and LGPL-2.1-or-later according to its source notices, but
models YMF262/OPL3. emu8950 provides the explicit YM3812 target and per-channel
output gains needed here. This selection does not assert greater hardware
accuracy than Nuked. C++ DBOPL and large audio frameworks were not imported.

Upstream files, exact provenance and two safety patches are recorded in
[README.wolf3d.md](../third_party/emu8950/README.wolf3d.md). The patches fix
partial-allocation cleanup and a negative signed left shift found by UBSan.
The unused Y8950 ADPCM module is linked because upstream references its API;
the selected chip type is YM3812 and no Y8950 sample playback is used.

## Resources and ownership

`ID_SD.c` uses the existing case-aware `CA_ReadDataFile` and stable MM ownership
handles. AUDIOHED's little-endian offsets bound the original 288 WL6 chunks:
87 PC sounds, 87 AdLib sounds, 87 unused sampled-sound slots, then 27 music
chunks. Actual effects use the original 87 `soundnames` indices and the AdLib
common header's priority. All effects validate their 32-bit length, 16-bit
priority, 16-byte instrument and block/data boundaries. PC chunks are validated
but PC speaker playback is not exposed or implemented.

WL6 music uses the original length-prefixed `MusicGroup`, followed by four-byte
register/value/delay records. Length must be nonzero, divisible by four and
inside its archive chunk. This is deliberately a WL6 loader, not a generic
IMF type-0/type-1 detector. Empty unused digitised slots are allowed; malformed
referenced resources disable audio with an error rather than unchecked reads.

VSWAP's complete offset/length directory is checked for file bounds, ordering
and sparse entries. The last page's original metadata supplies start-page and
byte length for each of 46 digitised samples. Samples are copied across bounded
4096-byte pages into ordinary memory. No dependence on the graphics page
cache survives a level restart. Mapping is the registered, non-SPEAR table,
including DEATHSCREAM2/3 sharing sample 13. No resource filesystem work occurs
while generating PCM. Archives are opened read-only; no game directory write
permission is needed. Existing CA startup still requires the original archive
set and rejects missing/empty files before the optional audio startup stage.

## Scheduling and mixing

The portable sound manager ports `SD_PlaySound`, AL instrument programming,
`SDL_ALSoundService`/DOFX, IMF sequencing, music off/start, digitised interruption
and priority handling. `SD_Render` advances a rational 700 Hz sample clock;
every fifth service tick processes 140 Hz effects **before** music, as in the
original ISR. At 44.1/48 kHz, register timing is independent of frame rate and
render request size. There is no change to the separate 70 Hz simulation.

The sequencer preserves a subtle DOS rule: immediately after the final event
is written, its pointer and time counters reset; that final event's delay is
not waited out. Looping preserves live chip envelopes, instruments and rhythm
state. It does not reset the synthesiser or append an invented release tail.
Music transitions key off music channels and rhythm, matching `SD_MusicOff`.
Re-enabling music restarts the selected track and its instrument setup.

A single chip receives both music and effects. Effects program operators zero
and three/channel zero, clear the instrument first, set connection to zero as
in Wolf's MUSE-compatible driver, and use the original block/key-on pattern.
IMF register writes can overwrite those registers. There are no independent
full-chip music/effect instances. Channel zero is assigned the effects gain;
remaining melodic and percussion outputs receive the music gain. Owned tracks
use channel zero for their initial reset, rather than melodic notes. This is
an output-level convenience, not separate synthesis or filtered IMF writes.

The chip runs at its native approximately 49,716 Hz rate; bounded linear
interpolation produces the requested output rate without an allocating
resampler in the render path. Digitised samples are unsigned eight-bit PCM,
held at the original SB DAC rate: `1000000 / 142` Hz (about 7042.25 Hz), from
`256 - floor(1000000/7000)`. Their phase accumulator preserves that fractional
rate. The mixer combines FM and voices with saturation to stereo signed
16-bit PCM. Analogue AdLib/SB filters and exact DAC electrical behaviour are
not reproduced; this is not a cycle- or analogue-exact hardware claim.

There is one AdLib effect and one digitised voice, with independent original
priorities. Lower priority cannot interrupt; equal or higher can. Neither
lane invents modern polyphony. The original 15-by-30 positioning tables,
focal camera offset, rotated coordinates, distance limits and non-positional
sounds are retained. Only digitised sounds use the original SB Pro position
attenuation. Original attenuation nibbles map to linear software gain; the
analogue mixer chip's exact gain curve is not modelled. Positions are copied, and moving the listener updates the active
sample's balance; removed/reused actors are never dereferenced by playback.

## SDL3, settings and integration

`sdl_audio.c` opens an SDL3 default playback stream, supplies 48 kHz stereo
PCM with `SDL_PutAudioStreamData`, and lets SDL convert to the physical device.
The main thread fills a bounded 24–32 ms queue in 384-frame blocks. SDL owns
asynchronous consumption. There is no application audio callback, mutex,
filesystem access, dynamic allocation or mutable actor access in synthesis.
Severe main-thread stalls can underrun this small queue; no underrun changes
the simulation, RNG or music's sample-domain tempo.

Audio starts after video startup and stops before video's `SDL_Quit`.
Repeated startup/shutdown and failed opening release all resource handles.
A device/queue error reports a reason and permits silent play. SDL manages
its default-device routing. On Linux, if an ALSA-only SDL build cannot open
its unconfigured default and no user device hint exists, the backend also
tries ALSA's standard `pulse` endpoint without changing system configuration.
Other SDL drivers keep their normal default behaviour.

The main Sound menu uses the existing fonts, panel renderer and input queue:
Up/Down select; Enter/Space or Left/Right toggle enable switches; Left/Right
change the corresponding volume in five-percent steps. Settings are session
only. `--no-audio` skips resource synthesis/device startup. No nonfunctional
PC speaker/device mode rows, new UI framework or configuration registry exist.

Title/attract, control panel, scores, all 60 level selections, intermissions,
victory statistics and ending articles select/retain their original songs.
The original actor hooks now produce voices, shots, death screams, footsteps,
projectile effects and BJ's yell. Missing native events were restored for
player gun/knife attacks, all pickups and extra lives, wall collisions,
opening/closing/locked doors, pushwalls, elevator/invalid use, player death,
and menu movement/select/cancel. Existing actor RNG calls were preserved.
The original active non-SPEAR `TakeDamage` and `CheckWeaponChange` do not
emit standalone player-injury/weapon-selection sounds; unused identifiers
were not repurposed to invent them.

The application's logical original mode is AdLib plus digitised sounds,
configured before loading levels even with `--no-audio`, a muted bus, corrupt
audio contents or failed SDL opening. Schabbs/Gift/Fat/Hitler die2 states
therefore use the original **140 tics**. The portable core's explicitly
non-digitised mode uses **five tics**, retained for existing silent fixtures.
Mode is never derived from device availability or volume. This intentionally
restores the original mode-dependent delay documented as absent in 6A.

## Verification

CART and regression commands are in [BUILD.md](BUILD.md). Automated coverage
includes synthetic, invented instruments/music/samples and owned files read
in place. It checks:

- Archive offsets, malformed lengths/instruments/IMF, truncated files and
  VSWAP sample metadata; cleanup after failed loading and read-only directories.
- Exact 700/140 Hz service, zero-delay batches, stateful loop restart, expected
  full first-loop duration of each of the 27 owned tracks, 44.1/48 kHz timing,
  split-buffer PCM equality, audible nonzero FM and all 87 sound identifiers.
- A deliberate channel-zero collision proving effect-before-IMF write order
  on the same chip; distinct and equal-priority interruption on both lanes;
  shared scream mapping and exact digitised sample duration/channel gains.
- Original positional tables/orientation, both mute/volume controls, settings
  input, repeated SDL dummy lifecycle and intentional device-opening failure.
- Identical player/actor/game state and RNG after 210 gameplay tics with PCM
  generation enabled versus no sound manager, at the same logical device mode.
- Both original boss delays with muted/absent hardware, all 240 map/difficulty
  combinations, and all six controlled SDL weapon/finale/article flows with
  actual archive loading, synthesis and dummy-device output enabled.

The final CART Linux build and all **51** Python regression cases pass with
owned data (222 seconds). The full **ASan/UBSan** application and instrumented
C/SDL suite also passes all **51** cases (609 seconds), with
`UBSAN_OPTIONS=halt_on_error=1` and no sanitizer reports. CART's SDL3 and desktop
output-directory regression scripts pass. The Sound panel was rendered using
owned fonts and visually inspected at both volumes set to 100%; text fits.
All **35** application C translation units pass strict MinGW C11 compilation;
the Windows application also cross-links against the official SDL3 3.4.18
MinGW SDK. A separate ignored Wine prefix runs that Windows executable through
E1M1 startup and five frames with SDL dummy video/audio. The Windows synthetic
audio executable also passes under Wine (timing, shared-chip writes, PCM,
priorities, positioning, settings and repeated dummy-device lifecycle). These
are Wine API checks, **not native Windows or Windows physical-audio verification**.
The SDK and test binaries remain in ignored `build/checks/`.

These campaign tests are controlled encounters, not fully navigated episodes.
The reused Linux SDL static library itself is not sanitizer-instrumented.

### Listening and physical output

A 12-second test played original title music, pistol, guard voice, ammunition
pickup and BJ voice through SDL3, ALSA's pulse plugin, PipeWire and the physical
Intel PCH analogue stereo sink at 48 kHz. A new `s16le 2ch 48000Hz` sink input
was observed. The check was replayed on request. The user confirmed: **“I heard
it; it sounded normal.”** No audio recording or converted audio file was made.

This establishes a short human listening check and physical Linux output,
not an exhaustive timbre/percussion/latency or DOS A/B comparison. Such a
comparison, longer device-loss/stall testing and fully navigated campaign
listening remain future verification.

## Remaining work

- Restore animated intermission counting and its ENDBONUS1/2, PERCENT100 and
  NOBONUS sequencing alongside its presentation. The existing immediate-final
  results path does not pretend to play that missing animation's sounds.
- Compare representative melodic/rhythm tracks and simultaneous AL effects
  against DOS on the same output route. Tune interpolation/levels only from
  that evidence; the short listening check is not a hardware-fidelity oracle.
- Verify packaging, device changes and physical playback on native Windows.
  PC speaker mode is optional and remains unimplemented/unexposed.
