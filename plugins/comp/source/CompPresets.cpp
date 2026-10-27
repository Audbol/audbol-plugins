#include "CompParams.h"

#include "suite/Presets.h"

using suite::PresetList;

namespace comp {

namespace {

constexpr double Peak = 0, Rms = 1, On = 1;

PresetList build ()
{
	return {
	    // Vocals
	    {"Vocal Leveler", "Vocals",
	     {{kThreshold, -24}, {kRatio, 3}, {kKnee, 8}, {kAttack, 8}, {kRelease, 120}, {kAutoRelease, On},
	      {kMakeup, 4}, {kDetection, Rms}}},
	    {"Vocal Control", "Vocals",
	     {{kThreshold, -20}, {kRatio, 4.5}, {kKnee, 6}, {kAttack, 3}, {kRelease, 80}, {kMakeup, 5}}},
	    {"Gentle Vocal", "Vocals",
	     {{kThreshold, -18}, {kRatio, 2}, {kKnee, 12}, {kAttack, 15}, {kRelease, 200}, {kAutoMakeup, On}}},
	    {"Acoustic Guitar", "Vocals",
	     {{kThreshold, -22}, {kRatio, 3}, {kKnee, 8}, {kAttack, 10}, {kRelease, 150}, {kMakeup, 3}}},
	    // Drums
	    {"Kick Punch", "Drums",
	     {{kThreshold, -18}, {kRatio, 4}, {kKnee, 4}, {kAttack, 25}, {kRelease, 80}, {kMakeup, 3}}},
	    {"Snare Snap", "Drums",
	     {{kThreshold, -22}, {kRatio, 5}, {kKnee, 3}, {kAttack, 15}, {kRelease, 60}, {kMakeup, 4}}},
	    {"Drum Bus Glue", "Drums",
	     {{kThreshold, -14}, {kRatio, 2}, {kKnee, 6}, {kAttack, 30}, {kRelease, 200}, {kAutoRelease, On},
	      {kMakeup, 2}, {kScFilterOn, On}, {kScFilterFreq, 90}}},
	    {"Parallel Smash", "Drums",
	     {{kThreshold, -40}, {kRatio, 20}, {kKnee, 0}, {kAttack, 1}, {kRelease, 60}, {kMakeup, 18}, {kMix, 35}}},
	    {"Room Crush", "Drums",
	     {{kThreshold, -45}, {kRatio, 100}, {kKnee, 0}, {kAttack, 0.5}, {kRelease, 250}, {kMakeup, 22}, {kMix, 25}}},
	    // Bus & Master
	    {"Bass Tamer", "Bus & Master",
	     {{kThreshold, -20}, {kRatio, 4}, {kKnee, 6}, {kAttack, 20}, {kRelease, 150}, {kMakeup, 3}, {kDetection, Rms}}},
	    {"Mix Bus Glue", "Bus & Master",
	     {{kThreshold, -12}, {kRatio, 2}, {kKnee, 10}, {kAttack, 30}, {kRelease, 300}, {kAutoRelease, On},
	      {kMakeup, 1.5}, {kScFilterOn, On}, {kScFilterFreq, 120}}},
	    {"Master Gentle", "Bus & Master",
	     {{kThreshold, -10}, {kRatio, 1.5}, {kKnee, 12}, {kAttack, 50}, {kRelease, 400}, {kAutoRelease, On},
	      {kMakeup, 1}, {kDetection, Rms}}},
	    // Creative
	    {"Pumping", "Creative",
	     {{kThreshold, -30}, {kRatio, 8}, {kKnee, 2}, {kAttack, 2}, {kRelease, 400}, {kMakeup, 8}}},
	    {"Transient Softener", "Creative",
	     {{kThreshold, -16}, {kRatio, 6}, {kKnee, 4}, {kAttack, 0.1}, {kRelease, 50}, {kMakeup, 2}, {kDetection, Peak}}},
	};
}

} // namespace

const PresetList& presets ()
{
	static const PresetList list = build ();
	return list;
}

} // namespace comp
