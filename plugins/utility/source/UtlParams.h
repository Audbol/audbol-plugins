#pragma once

#include "suite/ParamSpec.h"

#include "pluginterfaces/base/funknown.h"

namespace utl {

static const Steinberg::FUID kProcessorUID (0xDE25108F, 0xA5DB468B, 0x94904C02, 0x8030E472);
static const Steinberg::FUID kControllerUID (0xE8786B46, 0xB7A64083, 0x930A8340, 0x83EDE71B);

enum Param : uint32_t
{
	kBypass = 0,
	kGain,
	kMute,
	kInvertL,
	kInvertR,
	kChannelMode,
	kWidth,
	kPan,
	kBassMono,
	kBassMonoFreq,
	kDcFilter,
};

enum class ChannelMode : int
{
	Stereo,
	Mono,
	Left,
	Right,
	Swap
};

// Tap: the processed left and right channels (for goniometer, correlation
// and meters).
enum TapStream : uint32_t
{
	kTapLeft,
	kTapRight,
	kNumTapStreams
};

const suite::ParamTable& params ();

} // namespace utl
