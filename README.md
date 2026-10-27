# Audbol Plugin Suite

A suite of VST3 plug-ins built on the Steinberg VST3 SDK and VSTGUI, with a
shared modern dark UI and DSP library.

| Plug-in | Status |
| --- | --- |
| **Audbol EQ**: 8-band parametric EQ with live analyzer | Working |
| **Audbol Compressor**: transfer curve, GR history, IN/GR/OUT meters | Working |
| **Audbol Limiter**: lookahead brickwall, true-peak detection | Working |
| **Audbol Reverb**: FDN reverb with decay and tone displays, freeze | Working |
| **Audbol Delay**: tempo-synced stereo / ping-pong / cross delay | Working |
| **Audbol Saturation**: 5 models, 4x oversampled, harmonics display | Working |
| **Audbol Chorus**: 1-4 voice chorus / flanger with live LFO and comb display | Working |
| **Audbol Utility**: gain, polarity, channels, width, bass mono, goniometer | Working |
| **Audbol Loudness**: BS.1770-4 / EBU R128 LUFS meter, LRA, true peak | Working |
| Oscilloscope | Planned |

The brand name is a placeholder; change it in `common/include/suite/Brand.h`.

## Building (Windows)

Requirements: Visual Studio 2022 (or Build Tools) with the C++ workload, CMake 3.25+.

```bash
git clone --recursive https://github.com/Audbol/audbol-plugins.git
cd audbol-plugins
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Output: `build/VST3/Release/AudbolEQ.vst3`. Copy it to
`C:\Program Files\Common Files\VST3`, or configure with
`-DSMTG_CREATE_PLUGIN_LINK=ON` (requires admin) to symlink it there.

Every build runs the SDK validator on each plug-in, so a conformance failure
fails the build. To run the extensive test set:

```bash
build/bin/Release/validator.exe -e build/VST3/Release/AudbolEQ.vst3
```

### CPU benchmark and automation fuzz

`build/bin/Release/bench.exe` (target `bench`) loads a plug-in headless and
reports CPU use as % of one core at 48 and 96 kHz:

```bash
build/bin/Release/bench.exe build/VST3/Release/AudbolReverb.vst3
```

`tools/bench/fuzz_all.ps1` hammers every plug-in with random parameter
changes every block (noise from silence to +9.5 dBFS, at 44.1 / 48 / 96 kHz)
and fails on NaN, infinity or runaway output.

### Testing with audio in REAPER

`tools/reaper/run_all.ps1 -SampleRates 44100,48000,96000` runs every case
below at each rate and prints one line per case.

`tools/reaper/run.ps1` runs a plug-in in REAPER with a generated test signal
(drum loop or pink noise), using REAPER's Dummy Audio driver so no sound card
is needed. It logs the parameters as the host sees them, measures the track's
output peak with the plug-in active vs. bypassed, and saves a screenshot of
the running editor. Your `reaper.ini` is backed up and restored. Close REAPER
first, then from PowerShell:

```powershell
& .\tools\reaper\run.ps1 -Case compressor   # any file name in tools/reaper/cases
```

Cases live in `tools/reaper/cases/*.lua`: plug-in name, signal (`drums`,
`pink`, `impulse` = a hit every 4 s, `sine` = 1 kHz and `bass` = 60 Hz,
both at -12 dBFS), parameter
values, and optional
checks. `EXPECT_MAX_PEAK_DB` / `EXPECT_MIN_PEAK_DB` give pass/fail peak
limits for the window `MEASURE_START`..3.8 s after playback starts;
`LATE_SETTINGS` (at `LATE_AT` s) changes parameters mid-play.
`EXPECT_PARAMS` checks the plug-in's own parameter values as REAPER displays
them (e.g. meter outputs), read at `PARAMS_AT` s. `loudness_reference.ps1` is
an independent BS.1770 implementation used to derive loudness expectations.
`WINDOWS`
checks min/max peaks in short windows of play position (optionally per
channel), e.g. that an echo lands at 0.5 s; `TEMPO` sets the project tempo.
REAPER's track meter needs ~0.3 s to fall below -45 dB after a sound, so
"silence" windows should start at least that long after the previous one.
The log also shows the latency REAPER compensates for.

To open a plug-in UI without any DAW, configure with
`-DSMTG_ENABLE_VST3_HOSTING_EXAMPLES=ON` and run
`build/bin/Release/editorhost.exe build/VST3/Release/AudbolEQ.vst3`
(no audio there, so meters and analyzers stay empty).

## Layout

```
common/                 shared library (suite_common)
  ParamSpec             declarative parameter table: mapping, display, parsing
  dsp/                  SVF filters + exact response, smoothers, FFT, analyzer
  vst/ProcessorBase     stereo I/O, param dispatch, versioned state, bypass crossfade, FTZ
  vst/ControllerBase    params/units from the table, UI-only state, AudioTap receiver
  vst/AudioTap          lock-free processor -> UI audio stream (IDataExchange)
  ui/EditorBase         code-built VSTGUI editor: binding, text entry, HiDPI, timer
  ui/Controls, Theme    knob, toggle, menu, segmented control, panels, palette
  ui/Meters             level / gain-reduction meter, scrolling dynamics history
  vst/DynamicsTap       shared IN/OUT/GR metering stream for dynamics plug-ins
  dsp/TruePeak          4x oversampled inter-sample peak detector
  dsp/Oversampler       4x polyphase FIR oversampler (32 samples latency)
  dsp/DelayLine         interpolating delay buffer
  dsp/LoudnessMeter     BS.1770-4 / EBU R128 loudness (M, S, I, LRA, true peak)
plugins/eq/             Audbol EQ
plugins/comp/           Audbol Compressor
plugins/limiter/        Audbol Limiter
plugins/reverb/         Audbol Reverb
plugins/delay/          Audbol Delay
plugins/saturation/     Audbol Saturation
plugins/chorus/         Audbol Chorus
plugins/utility/        Audbol Utility
plugins/loudness/       Audbol Loudness
tools/reaper/           automated REAPER audio test harness
cmake/SuitePlugin.cmake suite_add_plugin() helper
```

Adding a plug-in: create `plugins/<name>/` with a parameter table, a
`ProcessorBase` subclass, a `ControllerBase` subclass, an `EditorBase`
subclass and a factory (generate fresh class IDs), then
`add_subdirectory(plugins/<name>)` in the root `CMakeLists.txt`.

## Audbol EQ controls

- **Graph**: double-click empty space to add a band; drag a node for
  frequency/gain (vertical drag on cut filters sets resonance); Shift for fine
  control; mouse wheel on a node for Q; double-click a node to remove it;
  right-click a node for type, slope, stereo placement and reset.
- **Knobs**: drag vertically (Shift = fine), double-click for default, wheel to
  nudge, right-click or Ctrl-click to type a value (`2.5k`, `-3`, `0.7`).
- **Filter types**: Bell, Low/High Shelf, Low/High Cut (6-96 dB/oct), Notch,
  Band Pass, Tilt Shelf. Each band can run on Stereo, Left, Right, Mid or Side.

## Audbol Compressor controls

- **Transfer curve**: drag left/right for threshold, up/down for ratio
  (Shift = fine), wheel for knee, double-click to reset. The yellow dot shows
  the live input level on the curve.
- **History**: scrolling input (grey), output (blue) and gain reduction
  (orange, hanging from the top) with the threshold marked.
- **Meters**: IN / GR / OUT with peak hold; click a meter to reset its hold.
- **Controls**: threshold, ratio (up to inf:1), soft knee, attack, release
  with optional program-dependent auto release, makeup with auto gain,
  parallel mix, peak/RMS detection, stereo link, sidechain high-pass filter.

## Audbol Limiter controls

- **Gain** drives the signal into the **Ceiling**; the output never exceeds
  the ceiling. 3 ms lookahead (reported to the host as latency, so tracks stay
  aligned; bypass is latency-compensated too).
- **Release** with optional program-dependent **Auto** release, **Link** for
  stereo linking, **True Peak** for 4x oversampled inter-sample detection.
- History and IN / GR / OUT meters as in the compressor; the **Max** readouts
  hold the largest reduction and output peak until clicked.

## Audbol Reverb controls

- **Decay view** (time): pre-delay gap, early-reflection taps and the tail's
  low / mid / high decay slopes. Drag for decay, drag the PRE marker for
  pre-delay, wheel for size.
- **Tone view** (frequency): decay time across frequency over the live
  spectrum of the reverb signal. LOW and MID handles drag vertically (bass
  multiplier, decay); DAMP, LO CUT and HI CUT drag horizontally. The curve is
  computed from the same loop filter the DSP uses.
- **Space**: mix (equal-power), pre-delay, decay, size, diffusion, early
  reflections. **Tone**: damping, bass multiplier, low/high cut on the wet
  signal. **Modulation & stereo**: depth, rate, width.
- **Freeze** holds the current tail indefinitely. The tail length is reported
  to the host so offline renders don't cut it off.

## Audbol Delay controls

- **Echo view**: the echo pattern for the current mode (left above, right
  below, heights in dB) on a beat grid when synced. Drag left/right for time
  (note steps when synced; the lower half edits the right side when unlinked),
  up/down or wheel for feedback.
- **Repeat Tone**: the feedback filters' response for the 1st to 4th repeat;
  drag the handles for low cut / high cut.
- **Time**: Sync to host tempo with straight, dotted and triplet notes from
  1/32 to 1/1, or free time up to 4 s; Link uses the left time for both sides.
  Time changes glide like tape rather than clicking.
- **Mode**: Stereo, Ping-Pong (mono in, bouncing L/R) or Cross feedback.
- **Feedback & modulation**: feedback, low/high cut and drive inside the loop
  (each repeat gets darker and grittier), tape-style wow (depth, rate).
- **Output**: equal-power mix, width, ducking (echoes duck under the input).

## Audbol Saturation controls

- **Models**: Tape (soft, late knee), Tube (asymmetric, even harmonics), Warm
  (tanh), Hard (clip with a short knee), Fold (wavefolder).
- **Curve**: the transfer curve with the input range in use and a live dot.
  Drag up/down for drive, left/right for bias (asymmetry, adds even
  harmonics), double-click to reset.
- **Harmonics**: H1-H12 relative to the fundamental and THD, for a sine at the
  current input level, computed from the same curve the audio uses.
- **Drive**, **Bias**, **Tone** (tilt around 1 kHz), **Mix** (latency-aligned
  dry), **Output**, **Auto Gain** (matches output loudness to the input).
- Processing is 4x oversampled to keep harmonics from aliasing; the 32-sample
  latency is reported to the host.

## Audbol Chorus controls

- **LFO view**: every voice's delay over one LFO cycle (left solid, right
  dashed) with a playhead at the processor's actual LFO phase. Drag up/down
  for depth, left/right for rate, wheel for delay, double-click to reset.
- **Response now**: the instantaneous frequency response of dry + wet for
  each channel (including feedback and the wet filters), so you can watch the
  comb filter sweep. Combs denser than a pixel are shown as their average.
- **Modulation**: rate, depth (relative to the delay, so it works from
  flanger to chorus lengths), delay 0.5-40 ms, feedback +-90 % (flanger),
  stereo spread (LFO phase offset), sine/triangle shape, 1-4 voices.
- **Output**: mix (linear, so correlated low end stays at unity level),
  width, low/high cut on the wet signal (off at the ends of their ranges).

## Audbol Utility controls

- **Channels**: Stereo, Mono (L+R), Left or Right on both sides, Swap.
- **Gain** (-48..+24 dB), **Mute** (click-free), **DC** filter (5 Hz).
- **Width** 0 % (mono) to 200 %; **Pan** is a balance control (centre keeps
  both sides at unity; type `L 30`, `C`, `R 30`).
- **Inv L / Inv R** flip polarity per channel.
- **Bass Mono** removes stereo (side) content below the set frequency with a
  24 dB/oct filter, keeping the low end mono-safe.
- **Displays**: goniometer (mid vertical, side horizontal, auto-scaled),
  correlation (+1 mono .. -1 out of phase, with a mono-compatibility note),
  balance (energy, shown as a dB difference), and L / R / M / S meters.

## Audbol Loudness

- Measures per ITU-R BS.1770-4 / EBU R128: **Momentary** (400 ms),
  **Short-term** (3 s), gated **Integrated** (absolute -70 LUFS, relative
  -10 LU), **Loudness Range** (EBU 3342) and 4x oversampled **True Peak**.
  K-weighting is derived for the session sample rate.
- Measurement runs in the audio processor, so Integrated keeps counting with
  the window closed. The values are also VST3 output parameters, so hosts can
  display or record them.
- **Target**: -14 (streaming), -16 (Apple), -23 (EBU R128), -24 (ATSC). The
  Integrated readout turns green within +-1 LU and shows the gain needed;
  True Peak turns red above -1 dBTP.
- **History**: the last 60 s of momentary and short-term loudness with the
  target band and integrated line. **Reset Integrated** starts a new
  measurement. Audio passes through untouched.
