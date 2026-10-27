#pragma once

#include "suite/ParamSpec.h"

#include "pluginterfaces/base/funknown.h"

namespace loud {

static const Steinberg::FUID kProcessorUID (0x4F2C7F43, 0xB77B46BC, 0x829DAAA4, 0x96D570C5);
static const Steinberg::FUID kControllerUID (0x039AADC8, 0xAFB94D71, 0x81907A82, 0x89597BA8);

enum Param : uint32_t
{
	kTarget = 0,
	kReset = 1,
	// Read-only meter outputs (VST3 output parameters)
	kMomentary = 10,
	kShortTerm,
	kIntegrated,
	kRange,
	kTruePeakMax,
};

// Values sent to the UI every 100 ms (one frame per update).
enum TapStream : uint32_t
{
	kTapMomentary,
	kTapShortTerm,
	kTapIntegrated,
	kTapRange,
	kTapTruePeakMax,
	kTapTruePeakRecent,
	kNumTapStreams
};

constexpr double kTargets[] = {-14.0, -16.0, -23.0, -24.0};
constexpr double kTruePeakLimit = -1.0; // dBTP, common delivery limit

const suite::ParamTable& params ();

} // namespace loud
