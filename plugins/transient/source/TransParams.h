#pragma once

#include "suite/ParamSpec.h"
#include "suite/Presets.h"

#include "pluginterfaces/base/funknown.h"

namespace trans {

static const Steinberg::FUID kProcessorUID (0xA68D7A95, 0xC4164CA5, 0x8CDFF47F, 0x1C5412ED);
static const Steinberg::FUID kControllerUID (0x6E3E2027, 0x95A942C4, 0xB687BF5F, 0xFBC98B94);

enum Param : uint32_t
{
	kBypass = 0,
	kAttack,
	kSustain,
	kAttackTime,
	kSustainTime,
	kMix,
	kOutput,
};

enum TapStream : uint32_t
{
	kTapInput,  // max(|L|, |R|) before processing
	kTapOutput, // max(|L|, |R|) after processing
	kTapGain,   // applied gain in dB, signed
	kNumTapStreams
};

// At +-100 % the shaper changes the level by up to this much.
constexpr double kMaxShapeDb = 15.0;

const suite::ParamTable& params ();
const suite::PresetList& presets ();

} // namespace trans
