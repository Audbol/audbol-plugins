#include "LimParams.h"

#include "suite/Presets.h"

using suite::PresetList;

namespace lim {

namespace {

constexpr double On = 1, Off = 0;

PresetList build ()
{
	return {
	    // Mastering
	    {"Transparent Master", "Mastering",
	     {{kInputGain, 3}, {kCeiling, -1}, {kRelease, 100}, {kAutoRelease, On}, {kTruePeak, On}}},
	    {"Streaming -1 dBTP", "Mastering",
	     {{kInputGain, 4}, {kCeiling, -1}, {kRelease, 80}, {kAutoRelease, On}, {kTruePeak, On}}},
	    {"Gentle Peak Control", "Mastering",
	     {{kInputGain, 1.5}, {kCeiling, -1}, {kRelease, 300}, {kAutoRelease, On}, {kTruePeak, On}}},
	    {"Broadcast Safe", "Mastering",
	     {{kInputGain, 2}, {kCeiling, -2}, {kRelease, 120}, {kAutoRelease, On}, {kTruePeak, On}}},
	    // Loud
	    {"Loud Master", "Loud",
	     {{kInputGain, 8}, {kCeiling, -0.8}, {kRelease, 50}, {kAutoRelease, On}, {kTruePeak, On}}},
	    {"Club Loud", "Loud", {{kInputGain, 12}, {kCeiling, -0.3}, {kRelease, 30}, {kAutoRelease, On}}},
	    {"Unlinked Stereo", "Loud",
	     {{kInputGain, 4}, {kCeiling, -1}, {kRelease, 60}, {kStereoLink, 0}, {kTruePeak, On}}},
	    // Safety
	    {"Safety Clipper", "Safety",
	     {{kInputGain, 0}, {kCeiling, -0.3}, {kRelease, 20}, {kAutoRelease, Off}, {kTruePeak, Off}}},
	    {"Drum Bus Peaks", "Safety",
	     {{kInputGain, 6}, {kCeiling, -3}, {kRelease, 25}, {kAutoRelease, Off}, {kStereoLink, 70}}},
	};
}

} // namespace

const PresetList& presets ()
{
	static const PresetList list = build ();
	return list;
}

} // namespace lim
