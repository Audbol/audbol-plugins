#pragma once

#include "suite/ParamSpec.h"
#include "suite/Presets.h"

#include "pluginterfaces/base/funknown.h"

#include <algorithm>
#include <cmath>

namespace cho {

static const Steinberg::FUID kProcessorUID (0x0012760B, 0x57A644EF, 0xA91D5F52, 0x51DD404E);
static const Steinberg::FUID kControllerUID (0x20607A7A, 0x84E64073, 0x9987AB22, 0xBE11497F);

enum Param : uint32_t
{
	kBypass = 0,
	kMix,
	kRate,
	kDepth,
	kDelay,
	kVoices,
	kSpread,
	kFeedback,
	kShape,
	kWidth,
	kLowCut,
	kHighCut,
};

enum class Shape : int
{
	Sine,
	Triangle
};

enum TapStream : uint32_t
{
	kTapInput,
	kTapOutput,
	kNumTapStreams
};

constexpr int kMaxVoices = 4;
constexpr double kMinDelayMs = 0.5;
constexpr double kMaxDelayMs = 40.0;
// The wet filters switch off entirely at the ends of their ranges.
constexpr double kLowCutOffHz = 20.5;
constexpr double kHighCutOffHz = 19900.0;

// --- Voice geometry (shared by the DSP and the displays) -----------------

// Phase (0..1) of voice v on channel c, from the master LFO phase.
inline double voicePhase (double master, int v, int voices, int channel, double spreadDeg)
{
	double p = master + double (v) / voices + (channel == 1 ? spreadDeg / 360.0 : 0.0);
	return p - std::floor (p);
}

inline double lfo (Shape s, double phase)
{
	if (s == Shape::Triangle)
		return 1.0 - 4.0 * std::abs (phase - 0.5); // -1..1, peak at 0.5
	return std::sin (2.0 * 3.14159265358979323846 * phase);
}

// Centre delay of voice v: voices are spread +-15 % around the set delay so
// they don't move in lockstep.
inline double voiceCenterMs (double delayMs, int v, int voices)
{
	const double offset = voices > 1 ? (double (v) / (voices - 1) - 0.5) * 0.3 : 0.0;
	return delayMs * (1.0 + offset);
}

// Delay of voice v at a given voice phase. Depth is relative to the delay so
// short (flanger) and long (chorus) settings both stay above zero.
inline double voiceDelayMs (Shape s, double delayMs, double depth, int v, int voices, double phase)
{
	const double center = voiceCenterMs (delayMs, v, voices);
	return std::max (0.05, center * (1.0 + 0.9 * depth * lfo (s, phase)));
}

const suite::ParamTable& params ();
const suite::PresetList& presets ();

} // namespace cho
