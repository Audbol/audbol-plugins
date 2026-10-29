#include "GateParams.h"

#include "suite/Presets.h"

using suite::PresetList;

namespace gate {

namespace {

constexpr double Gate = 0, Expand = 1;

PresetList build ()
{
	return {
	    // Drums
	    {"Kick Gate", "Drums",
	     {{kThreshold, -30}, {kRange, 50}, {kMode, Gate}, {kHysteresis, 6}, {kAttack, 0.1}, {kHold, 60},
	      {kRelease, 120}, {kKeyLpf, 150}}},
	    {"Snare Gate", "Drums",
	     {{kThreshold, -26}, {kRange, 30}, {kMode, Gate}, {kHysteresis, 6}, {kAttack, 0.1}, {kHold, 40},
	      {kRelease, 150}, {kKeyHpf, 180}, {kKeyLpf, 8000}}},
	    {"Tom Gate", "Drums",
	     {{kThreshold, -32}, {kRange, 40}, {kMode, Gate}, {kHysteresis, 6}, {kAttack, 0.2}, {kHold, 120},
	      {kRelease, 350}, {kKeyHpf, 60}, {kKeyLpf, 3000}}},
	    {"Hi-Hat Tighten", "Drums",
	     {{kThreshold, -28}, {kRange, 18}, {kMode, Expand}, {kRatio, 3}, {kAttack, 0.2}, {kHold, 10},
	      {kRelease, 60}, {kKeyHpf, 2000}}},
	    {"Overhead Cleanup", "Drums",
	     {{kThreshold, -40}, {kRange, 12}, {kMode, Expand}, {kRatio, 1.8}, {kAttack, 1}, {kHold, 80},
	      {kRelease, 400}}},
	    // Vocals
	    {"Vocal Cleanup", "Vocals",
	     {{kThreshold, -45}, {kRange, 15}, {kMode, Expand}, {kRatio, 2}, {kAttack, 2}, {kHold, 100},
	      {kRelease, 250}, {kKeyHpf, 100}}},
	    {"Podcast Noise", "Vocals",
	     {{kThreshold, -50}, {kRange, 20}, {kMode, Expand}, {kRatio, 2.5}, {kAttack, 3}, {kHold, 150},
	      {kRelease, 300}, {kKeyHpf, 120}}},
	    {"Breath Softener", "Vocals",
	     {{kThreshold, -38}, {kRange, 9}, {kMode, Expand}, {kRatio, 1.6}, {kAttack, 5}, {kHold, 60},
	      {kRelease, 200}, {kKeyHpf, 200}}},
	    // Instruments
	    {"Guitar Amp Hiss", "Instruments",
	     {{kThreshold, -50}, {kRange, 30}, {kMode, Gate}, {kHysteresis, 8}, {kAttack, 1}, {kHold, 80},
	      {kRelease, 200}}},
	    {"Tight Metal Guitar", "Instruments",
	     {{kThreshold, -42}, {kRange, 60}, {kMode, Gate}, {kHysteresis, 4}, {kAttack, 0.05}, {kHold, 5},
	      {kRelease, 30}}},
	    {"Bass DI Cleanup", "Instruments",
	     {{kThreshold, -48}, {kRange, 20}, {kMode, Expand}, {kRatio, 2}, {kAttack, 2}, {kHold, 100},
	      {kRelease, 300}, {kKeyLpf, 2000}}},
	    // Creative
	    {"Gated Reverb Chop", "Creative",
	     {{kThreshold, -36}, {kRange, 80}, {kMode, Gate}, {kHysteresis, 2}, {kAttack, 0.1}, {kHold, 220},
	      {kRelease, 15}}},
	    {"Stutter", "Creative",
	     {{kThreshold, -14}, {kRange, 80}, {kMode, Gate}, {kHysteresis, 0}, {kAttack, 0.05}, {kHold, 0},
	      {kRelease, 5}}},
	};
}

} // namespace

const PresetList& presets ()
{
	static const PresetList list = build ();
	return list;
}

} // namespace gate
