#pragma once

#include "suite/ParamSpec.h"
#include "suite/Presets.h"

#include "pluginterfaces/base/funknown.h"

#include <algorithm>
#include <cmath>

namespace clip {

static const Steinberg::FUID kProcessorUID (0x84AD2000, 0xB6CE47AA, 0x9616C1CC, 0x6A1B3432);
static const Steinberg::FUID kControllerUID (0x5479573A, 0x904A4919, 0xB92E24D0, 0xD4C2E751);

enum Param : uint32_t
{
	kBypass = 0,
	kInput,
	kCeiling,
	kSoftness,
	kOutput,
	kMatch,
	kDelta,
};

// The knee spans ceiling * (1 - k) .. ceiling * (1 + k); k at 100 % softness.
constexpr double kMaxKnee = 0.5;

// Clipping curve, memoryless; shared by the DSP and the display. `ceiling`
// is linear, `knee` in 0..kMaxKnee. Below the knee the signal is untouched;
// across it a parabola meets the line with slope 1 and flattens at the ceiling.
inline double clipSample (double x, double ceiling, double knee)
{
	const double a = std::abs (x);
	const double lo = ceiling * (1.0 - knee);
	if (a <= lo)
		return x;
	const double hi = ceiling * (1.0 + knee);
	if (a >= hi || knee <= 0.0)
		return std::copysign (ceiling, x);
	const double t = a - hi;
	return std::copysign (ceiling - t * t / (4.0 * ceiling * knee), x);
}

const suite::ParamTable& params ();
const suite::PresetList& presets ();

} // namespace clip
