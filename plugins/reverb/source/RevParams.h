#pragma once

#include "suite/ParamSpec.h"
#include "suite/Presets.h"

#include "pluginterfaces/base/funknown.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace rev {

static const Steinberg::FUID kProcessorUID (0xA9D64E26, 0xC15244A9, 0xA5425AF6, 0xFCD9FA3A);
static const Steinberg::FUID kControllerUID (0x4D89617C, 0x56ED4F64, 0x938B02DD, 0x16F04B34);

enum Param : uint32_t
{
	kBypass = 0,
	kMix,
	kPreDelay,
	kDecay,
	kSize,
	kDiffusion,
	kEarly,
	kWidth,
	kDamping,   // frequency above which the tail decays faster
	kBassMult,  // low-frequency decay multiplier
	kLowCut,
	kHighCut,
	kModDepth,
	kModRate,
	kFreeze,
};

// Tap stream layout sent to the UI.
enum TapStream : uint32_t
{
	kTapInput,  // max(|L|, |R|) of the dry input
	kTapOutput, // max(|L|, |R|) of the plug-in output
	kTapWet,    // mono wet signal, for the spectrum
	kNumTapStreams
};

// --- Shared design constants (DSP and displays) ---------------------------

constexpr int kNumLines = 8;
// Base FDN delay lengths in ms at Size = 100 % (mutually prime-ish).
constexpr double kLineMs[kNumLines] = {29.7, 37.1, 41.1, 43.7, 53.3, 59.9, 67.7, 73.1};

constexpr int kNumEarlyTaps = 12;
struct EarlyTap
{
	double ms;  // at Size = 100 %
	double gainL, gainR;
};
constexpr EarlyTap kEarlyTaps[kNumEarlyTaps] = {
    {4.3, 0.84, 0.00},  {7.1, 0.00, 0.79},  {11.9, 0.66, 0.31}, {14.6, 0.28, 0.63},
    {19.3, 0.55, 0.00}, {23.8, 0.00, 0.52}, {29.1, 0.42, 0.21}, {35.6, 0.19, 0.40},
    {41.2, 0.33, 0.00}, {48.7, 0.00, 0.29}, {57.3, 0.22, 0.12}, {66.9, 0.10, 0.18},
};

constexpr double kCrossoverHz = 250.0;
constexpr double kHighDecayRatio = 0.25; // decay above Damping, relative to Decay
constexpr double kMaxPreDelayMs = 250.0;

// Size 0..100 % -> delay scale.
inline double sizeScale (double sizePercent) { return 0.3 + 1.3 * sizePercent / 100.0; }

// Per-pass gain for a line of `delaySec` to reach -60 dB after `rt60Sec`.
inline double loopGain (double delaySec, double rt60Sec) { return std::pow (10.0, -3.0 * delaySec / rt60Sec); }

// Gains of the three bands of the loop filter.
struct LoopGains
{
	double low, mid, high;
};
inline LoopGains loopGains (double delaySec, double decaySec, double bassMult)
{
	return {loopGain (delaySec, decaySec * bassMult), loopGain (delaySec, decaySec),
	        loopGain (delaySec, decaySec * kHighDecayRatio)};
}

// Tail decay time (RT60, s) at a frequency. Evaluates the processor's loop
// filter H = gH + (gM - gH) LP(damping) + (gL - gM) LP(crossover) for a line
// of average length.
inline double decayAt (double freq, double decaySec, double bassMult, double damping, double sizePercent)
{
	double avgMs = 0.0;
	for (double ms : kLineMs)
		avgMs += ms;
	const double d = avgMs / kNumLines * sizeScale (sizePercent) * 0.001;
	const auto g = loopGains (d, decaySec, bassMult);
	auto lp = [freq] (double fc) {
		const double w = freq / fc;
		return std::pair<double, double> (1.0 / (1.0 + w * w), -w / (1.0 + w * w)); // 1 / (1 + jw)
	};
	const auto a = lp (damping);
	const auto b = lp (kCrossoverHz);
	const double re = g.high + (g.mid - g.high) * a.first + (g.low - g.mid) * b.first;
	const double im = (g.mid - g.high) * a.second + (g.low - g.mid) * b.second;
	const double mag = std::sqrt (re * re + im * im);
	return mag < 1.0 ? -3.0 * d / std::log10 (mag) : 1e9;
}

const suite::ParamTable& params ();
const suite::PresetList& presets ();

} // namespace rev
