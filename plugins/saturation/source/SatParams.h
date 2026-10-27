#pragma once

#include "suite/ParamSpec.h"
#include "suite/Presets.h"

#include "pluginterfaces/base/funknown.h"

#include <algorithm>
#include <cmath>

namespace sat {

static const Steinberg::FUID kProcessorUID (0x831577D0, 0xEED24C5A, 0x94E9F5B9, 0xA59D1937);
static const Steinberg::FUID kControllerUID (0x00FF7EB4, 0x45524A03, 0xAA125022, 0xBB05900C);

enum Param : uint32_t
{
	kBypass = 0,
	kDrive,
	kModel,
	kBias,
	kTone,
	kMix,
	kOutput,
	kAutoGain,
};

enum class Model : int
{
	Tape,
	Tube,
	Warm,
	Hard,
	Fold,
	kCount
};

enum TapStream : uint32_t
{
	kTapInput,
	kTapOutput,
	kNumTapStreams
};

// --- Shapers (memoryless; shared by the DSP and the displays) -------------

inline double shape (Model m, double x)
{
	switch (m)
	{
		case Model::Tape:
		{
			// Soft knee that stays linear longer than tanh, then flattens:
			// x / (1 + x^4)^(1/4), two square roots instead of pow().
			const double x2 = x * x;
			return x / std::sqrt (std::sqrt (1.0 + x2 * x2));
		}
		case Model::Tube:
			// Asymmetric: positive half compresses sooner -> even harmonics.
			return x >= 0.0 ? 1.0 - std::exp (-x) : (std::exp (1.6 * x) - 1.0) / 1.6;
		case Model::Warm: return std::tanh (x);
		case Model::Hard:
		{
			// Hard clip at +-1 with a short cubic knee from 0.9 to 1.1.
			const double a = std::abs (x);
			if (a <= 0.9)
				return x;
			if (a >= 1.1)
				return std::copysign (1.0, x);
			// y = 1 - 2.5 t^2 meets the line at 0.9 with slope 1 and flattens at 1.1.
			const double t = a - 1.1; // -0.2 .. 0
			return std::copysign (1.0 - 2.5 * t * t, x);
		}
		case Model::Fold:
			// Wavefolder: reflects back once it passes full scale.
			return std::sin (1.5707963267948966 * x);
		case Model::kCount: break;
	}
	return x;
}

// Drive (linear gain) and bias (asymmetry, -1..1) applied, static DC removed.
inline double saturate (Model m, double x, double driveGain, double bias)
{
	const double b = bias * 0.5;
	return shape (m, x * driveGain + b) - shape (m, b);
}

const suite::ParamTable& params ();
const suite::PresetList& presets ();

} // namespace sat
