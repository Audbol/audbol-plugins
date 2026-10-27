#include "SatParams.h"

#include "suite/Presets.h"

using suite::PresetList;

namespace sat {

namespace {

constexpr double Tape = 0, Tube = 1, Warm = 2, Hard = 3, Fold = 4;

PresetList build ()
{
	return {
	    // Subtle
	    {"Tape Glue", "Subtle", {{kModel, Tape}, {kDrive, 4}}},
	    {"Console Color", "Subtle", {{kModel, Warm}, {kDrive, 3}, {kTone, 0.5}}},
	    {"Warm Bus", "Subtle", {{kModel, Warm}, {kDrive, 6}, {kMix, 60}}},
	    // Warmth
	    {"Tape Warmth", "Warmth", {{kModel, Tape}, {kDrive, 8}, {kTone, -1.5}}},
	    {"Tube Warmth", "Warmth", {{kModel, Tube}, {kDrive, 6}, {kBias, 20}}},
	    {"Hot Tape", "Warmth", {{kModel, Tape}, {kDrive, 14}, {kTone, -2}}},
	    // Drive
	    {"Tube Drive", "Drive", {{kModel, Tube}, {kDrive, 15}, {kBias, 35}, {kTone, 1}}},
	    {"Bass Growl", "Drive", {{kModel, Tube}, {kDrive, 18}, {kBias, 40}, {kTone, -3}, {kMix, 70}}},
	    {"Vocal Edge", "Drive", {{kModel, Warm}, {kDrive, 10}, {kTone, 3}, {kMix, 50}}},
	    {"Drum Crunch", "Drive", {{kModel, Hard}, {kDrive, 12}, {kTone, 2}}},
	    {"Parallel Grit", "Drive", {{kModel, Hard}, {kDrive, 18}, {kMix, 25}}},
	    // Creative
	    {"Fuzz", "Creative", {{kModel, Hard}, {kDrive, 30}, {kTone, 4}}},
	    {"Lo-Fi Dirt", "Creative", {{kModel, Hard}, {kDrive, 20}, {kTone, -8}}},
	    {"Wavefolder", "Creative", {{kModel, Fold}, {kDrive, 14}}},
	    {"Wild Fold", "Creative", {{kModel, Fold}, {kDrive, 26}, {kBias, 30}, {kTone, 2}}},
	};
}

} // namespace

const PresetList& presets ()
{
	static const PresetList list = build ();
	return list;
}

} // namespace sat
