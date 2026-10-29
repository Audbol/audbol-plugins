#include "ClipParams.h"

#include "suite/Presets.h"

using suite::PresetList;

namespace clip {

namespace {

constexpr double On = 1;

PresetList build ()
{
	return {
	    // Drums
	    {"Kick Clip", "Drums", {{kInput, 4}, {kCeiling, -1}, {kSoftness, 20}}},
	    {"Snare Crunch", "Drums", {{kInput, 6}, {kCeiling, -1}, {kSoftness, 10}}},
	    {"Drum Bus Peaks", "Drums", {{kInput, 3}, {kCeiling, -0.5}, {kSoftness, 40}}},
	    {"Parallel Grit", "Drums", {{kInput, 12}, {kCeiling, -6}, {kSoftness, 0}, {kOutput, 3}}},
	    // Bus & Master
	    {"Pre-Limiter Shave", "Bus & Master", {{kInput, 2}, {kCeiling, -0.3}, {kSoftness, 50}}},
	    {"Master Clean", "Bus & Master", {{kInput, 1.5}, {kCeiling, -0.1}, {kSoftness, 70}}},
	    {"Loud Master", "Bus & Master", {{kInput, 5}, {kCeiling, -0.3}, {kSoftness, 30}}},
	    {"Safety Ceiling", "Bus & Master", {{kCeiling, -1}, {kSoftness, 60}}},
	    // Tracks
	    {"Bass Weight", "Tracks", {{kInput, 6}, {kCeiling, -3}, {kSoftness, 60}, {kMatch, On}}},
	    {"808 Push", "Tracks", {{kInput, 9}, {kCeiling, -2}, {kSoftness, 35}, {kMatch, On}}},
	    {"Vocal Peak Control", "Tracks", {{kInput, 3}, {kCeiling, -3}, {kSoftness, 80}}},
	    // Creative
	    {"Square Fuzz", "Creative", {{kInput, 24}, {kCeiling, -12}, {kSoftness, 0}, {kOutput, 6}}},
	    {"Soft Saturate", "Creative", {{kInput, 10}, {kCeiling, -6}, {kSoftness, 100}, {kMatch, On}}},
	};
}

} // namespace

const PresetList& presets ()
{
	static const PresetList list = build ();
	return list;
}

} // namespace clip
