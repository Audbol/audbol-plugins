#pragma once

#include "suite/ParamSpec.h"
#include "suite/Presets.h"

#include "pluginterfaces/base/funknown.h"

#include <algorithm>
#include <cmath>

namespace gate {

static const Steinberg::FUID kProcessorUID (0xDEA3D7CF, 0x97C348D9, 0xB8A3FCFC, 0xC3CABB9D);
static const Steinberg::FUID kControllerUID (0x43B8C9B8, 0xCEA34F62, 0xAC7D99C4, 0xB732B812);

enum Param : uint32_t
{
	kBypass = 0,
	kThreshold,
	kRange,
	kMode,
	kRatio,
	kHysteresis,
	kAttack,
	kHold,
	kRelease,
	kKeyHpf,
	kKeyLpf,
	kListen,
};

// Gate closes fully (down to Range) below the threshold; Expand turns quiet
// parts down gradually, by Ratio.
enum class Mode : int
{
	Gate,
	Expand
};

// Dynamics tap streams plus the key (detector) level for the history.
enum TapStream : uint32_t
{
	kTapKey = 3, // after suite::kNumDynStreams
	kNumTapStreams
};

constexpr double kMaxRangeDb = 80.0;

// Static attenuation (dB, >= 0) for a key level, ignoring hysteresis and
// timing. Shared by the DSP (expander) and the curve display.
inline double staticReductionDb (Mode mode, double levelDb, double threshold, double range, double ratio)
{
	if (levelDb >= threshold)
		return 0.0;
	if (mode == Mode::Gate)
		return range;
	return std::min (range, (threshold - levelDb) * (ratio - 1.0));
}

const suite::ParamTable& params ();
const suite::PresetList& presets ();

} // namespace gate
