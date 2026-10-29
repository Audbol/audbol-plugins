#pragma once

#include "suite/ParamSpec.h"
#include "suite/Presets.h"
#include "suite/dsp/Svf.h"

#include "pluginterfaces/base/funknown.h"

#include <algorithm>

namespace dess {

static const Steinberg::FUID kProcessorUID (0x0491D0B3, 0x01944FE5, 0x983369A6, 0x2FCC3B90);
static const Steinberg::FUID kControllerUID (0x7A20308A, 0x2D74465C, 0xB1C7EC06, 0x58604E57);

enum Param : uint32_t
{
	kBypass = 0,
	kThreshold,
	kRange,
	kFrequency,
	kQ,
	kRelease,
	kFilter,
	kMode,
	kListen,
	kStereoLink,
	kOutput,
};

// Sidechain filter shape: a band around Frequency, or everything above it.
enum class Filter : int
{
	Band,
	High
};

// Split reduces only the filtered band; Wide turns the whole signal down.
enum class Mode : int
{
	Split,
	Wide
};

constexpr double kRatio = 6.0;
constexpr double kKneeDb = 4.0;

// Gain reduction (dB, >= 0) for a sidechain level, with a soft knee and the
// Range cap. Shared by the DSP and the displays.
inline double reductionDb (double levelDb, double threshold, double range)
{
	const double over = levelDb - threshold;
	const double slope = 1.0 - 1.0 / kRatio;
	double gr;
	if (over <= -kKneeDb * 0.5)
		gr = 0.0;
	else if (over >= kKneeDb * 0.5)
		gr = over * slope;
	else
	{
		const double t = over + kKneeDb * 0.5;
		gr = slope * t * t / (2.0 * kKneeDb);
	}
	return std::min (gr, range);
}

// The sidechain (and split-band) filter for the current settings.
inline suite::dsp::BandDesign designDetector (Filter filter, double freq, double q, double sampleRate)
{
	using suite::dsp::FilterType;
	return filter == Filter::Band ? suite::dsp::designBand (FilterType::BandPass, freq, q, 0.0, 1, sampleRate)
	                              : suite::dsp::designBand (FilterType::LowCut, freq, 0.707, 0.0, 1, sampleRate);
}

const suite::ParamTable& params ();
const suite::PresetList& presets ();

} // namespace dess
