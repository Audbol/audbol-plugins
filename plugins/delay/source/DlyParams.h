#pragma once

#include "suite/ParamSpec.h"
#include "suite/Presets.h"

#include "pluginterfaces/base/funknown.h"

#include <algorithm>

namespace dly {

static const Steinberg::FUID kProcessorUID (0x82E96F29, 0xA60C4210, 0xA15411C7, 0x68C53061);
static const Steinberg::FUID kControllerUID (0x3B5C8DD3, 0x3FDC455F, 0xB812192E, 0x5DEFCC4A);

enum Param : uint32_t
{
	kBypass = 0,
	kMix,
	kSync,
	kTimeL,
	kTimeR,
	kNoteL,
	kNoteR,
	kLink,
	kFeedback,
	kMode,
	kLowCut,
	kHighCut,
	kDrive,
	kModDepth,
	kModRate,
	kWidth,
	kDucking,
};

enum class Mode : int
{
	Stereo,
	PingPong,
	Cross
};

enum TapStream : uint32_t
{
	kTapInput,
	kTapOutput,
	kNumTapStreams
};

constexpr double kMinTimeMs = 1.0;
constexpr double kMaxTimeMs = 4000.0;

// Note values, shortest to longest, in quarter notes.
constexpr int kNumNotes = 14;
constexpr const char* kNoteNames[kNumNotes] = {"1/32", "1/16T", "1/16", "1/8T", "1/16D", "1/8", "1/4T",
                                               "1/8D", "1/4",  "1/2T",  "1/4D", "1/2",  "1/2D",  "1/1"};
constexpr double kNoteBeats[kNumNotes] = {0.125, 1.0 / 6.0, 0.25, 1.0 / 3.0, 0.375, 0.5, 2.0 / 3.0,
                                          0.75,  1.0,       4.0 / 3.0, 1.5, 2.0, 3.0, 4.0};

// Delay time in ms for one side, from the sync state, note index, free time and tempo.
inline double delayMs (bool sync, int note, double freeMs, double tempo)
{
	if (!sync)
		return std::clamp (freeMs, kMinTimeMs, kMaxTimeMs);
	const double beats = kNoteBeats[std::clamp (note, 0, kNumNotes - 1)];
	return std::clamp (beats * 60000.0 / std::max (tempo, 1.0), kMinTimeMs, kMaxTimeMs);
}

const suite::ParamTable& params ();
const suite::PresetList& presets ();

} // namespace dly
