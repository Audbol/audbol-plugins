#pragma once

#include "suite/ParamSpec.h"
#include "suite/Presets.h"

#include "pluginterfaces/base/funknown.h"

#include <algorithm>
#include <cmath>

namespace comp {

static const Steinberg::FUID kProcessorUID (0x6CABC6A0, 0xBA6F4AAF, 0x965A85BA, 0xBFFD65CD);
static const Steinberg::FUID kControllerUID (0xCC14AE61, 0xDBA5499D, 0x853EA2B6, 0x95C8B5C5);

enum Param : uint32_t
{
	kBypass = 0,
	kThreshold,
	kRatio,
	kKnee,
	kAttack,
	kRelease,
	kAutoRelease,
	kMakeup,
	kAutoMakeup,
	kMix,
	kDetection,
	kStereoLink,
	kScFilterOn,
	kScFilterFreq,
};

enum class Detection : int
{
	Peak,
	Rms
};

constexpr double kMaxRatio = 100.0; // shown as "inf : 1"

// Static gain computer with a quadratic soft knee (Giannoulis et al.).
// Shared by the DSP and the transfer-curve display.
inline double compressDb (double inDb, double threshold, double ratio, double knee)
{
	const double over = inDb - threshold;
	if (knee > 0.0 && 2.0 * std::abs (over) <= knee)
	{
		const double t = over + knee * 0.5;
		return inDb + (1.0 / ratio - 1.0) * t * t / (2.0 * knee);
	}
	return over > 0.0 ? threshold + over / ratio : inDb;
}

// Makeup that roughly restores loudness for typical program material.
inline double autoMakeupDb (double threshold, double ratio)
{
	return std::max (0.0, -threshold * (1.0 - 1.0 / ratio) * 0.5);
}

const suite::ParamTable& params ();
const suite::PresetList& presets ();

} // namespace comp
