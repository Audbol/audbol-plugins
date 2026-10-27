#pragma once

#include "suite/ParamSpec.h"

#include "pluginterfaces/base/funknown.h"

namespace scope {

static const Steinberg::FUID kProcessorUID (0x72A6E0B8, 0xBF594C81, 0x8E68E6E8, 0x42FAA7D3);
static const Steinberg::FUID kControllerUID (0x59A6AA67, 0x33F742DC, 0xB7841621, 0xDDDD9938);

enum Param : uint32_t
{
	kTime = 0,
	kTrigger,
	kTriggerLevel,
	kChannels,
	kGain,
	kFreeze,
	kBeatLength,
	// Read-only measurements (VST3 output parameters)
	kFrequency = 10,
	kPeak,
};

enum class Trigger : int
{
	Free,
	RisingEdge,
	BeatSync
};

enum class Channels : int
{
	Both,
	Mid,
	Side,
	Left,
	Right
};

// Per-sample tap streams; the block info carries the measured frequency.
enum TapStream : uint32_t
{
	kTapLeft,
	kTapRight,
	kTapPpq, // song position in quarter notes, < 0 when not playing
	kNumTapStreams
};

constexpr int kNumBeatLengths = 7;
constexpr const char* kBeatLengthNames[kNumBeatLengths] = {"1/16", "1/8", "1/4", "1/2", "1 bar", "2 bars", "4 bars"};
constexpr double kBeatLengthQuarters[kNumBeatLengths] = {0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0};

const suite::ParamTable& params ();

} // namespace scope
