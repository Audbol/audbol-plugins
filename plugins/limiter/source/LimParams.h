#pragma once

#include "suite/ParamSpec.h"

#include "pluginterfaces/base/funknown.h"

namespace lim {

static const Steinberg::FUID kProcessorUID (0xFEEC859A, 0x8BF34F42, 0x8EC9E43D, 0x436B67A3);
static const Steinberg::FUID kControllerUID (0xB3197E4F, 0xCB6F47C3, 0x985DEB3A, 0xF029EB8C);

enum Param : uint32_t
{
	kBypass = 0,
	kInputGain,
	kCeiling,
	kRelease,
	kAutoRelease,
	kStereoLink,
	kTruePeak,
};

constexpr double kLookaheadMs = 3.0;

const suite::ParamTable& params ();

} // namespace lim
