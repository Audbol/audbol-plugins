#include "DlyParams.h"

#include "suite/Presets.h"

using suite::PresetList;

namespace dly {

namespace {

constexpr double On = 1, Off = 0;
// Note indices (see kNoteNames) and modes
constexpr double n8 = 5, n8D = 7, n4 = 8, n4D = 10;
constexpr double Stereo = 0, PingPong = 1, Cross = 2;

PresetList build ()
{
	return {
	    // Synced
	    {"Quarter Note", "Synced", {{kNoteL, n4}, {kFeedback, 30}, {kMix, 25}}},
	    {"Eighth Note", "Synced", {{kNoteL, n8}, {kFeedback, 35}, {kMix, 25}}},
	    {"Dotted Eighth", "Synced", {{kNoteL, n8D}, {kFeedback, 40}, {kMix, 25}}},
	    {"Ping-Pong Eighths", "Synced", {{kNoteL, n8}, {kMode, PingPong}, {kFeedback, 45}, {kMix, 30}}},
	    {"Ping-Pong Dotted", "Synced", {{kNoteL, n8D}, {kMode, PingPong}, {kFeedback, 50}, {kMix, 30}}},
	    {"Stereo Eighth + Dotted", "Synced", {{kLink, Off}, {kNoteL, n8}, {kNoteR, n8D}, {kFeedback, 35}, {kMix, 25}}},
	    // Vocal
	    {"Vocal Slap", "Vocal",
	     {{kSync, Off}, {kTimeL, 110}, {kFeedback, 5}, {kMix, 18}, {kLowCut, 200}, {kHighCut, 7000}}},
	    {"Vocal Throw", "Vocal",
	     {{kNoteL, n4}, {kFeedback, 35}, {kMix, 30}, {kDucking, 60}, {kLowCut, 250}, {kHighCut, 6000}}},
	    {"Ducked Echo", "Vocal", {{kNoteL, n8D}, {kFeedback, 40}, {kMix, 30}, {kDucking, 80}}},
	    {"Slapback Guitar", "Vocal", {{kSync, Off}, {kTimeL, 90}, {kFeedback, 0}, {kMix, 22}, {kHighCut, 8000}}},
	    // Tape & Character
	    {"Tape Echo", "Tape & Character",
	     {{kSync, Off}, {kTimeL, 340}, {kFeedback, 45}, {kMix, 30}, {kDrive, 35}, {kModDepth, 25}, {kModRate, 0.6},
	      {kLowCut, 150}, {kHighCut, 4500}}},
	    {"Worn Tape", "Tape & Character",
	     {{kSync, Off}, {kTimeL, 280}, {kFeedback, 55}, {kMix, 35}, {kDrive, 55}, {kModDepth, 50}, {kModRate, 1.1},
	      {kLowCut, 200}, {kHighCut, 3000}}},
	    {"Analog Dub", "Tape & Character",
	     {{kNoteL, n4D}, {kFeedback, 70}, {kMix, 35}, {kDrive, 30}, {kLowCut, 250}, {kHighCut, 2500}}},
	    // Creative
	    {"Cross Feedback Space", "Creative",
	     {{kSync, Off}, {kLink, Off}, {kTimeL, 300}, {kTimeR, 450}, {kMode, Cross}, {kFeedback, 50}, {kMix, 30}}},
	    {"Infinite Repeat", "Creative",
	     {{kNoteL, n4}, {kFeedback, 95}, {kMix, 35}, {kLowCut, 300}, {kHighCut, 6000}}},
	    {"Granular Wobble", "Creative",
	     {{kSync, Off}, {kTimeL, 60}, {kFeedback, 60}, {kModDepth, 100}, {kModRate, 4}, {kMix, 40}, {kMode, Stereo}}},
	};
}

} // namespace

const PresetList& presets ()
{
	static const PresetList list = build ();
	return list;
}

} // namespace dly
