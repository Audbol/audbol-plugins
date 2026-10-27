#include "RevParams.h"

#include "suite/Presets.h"

using suite::PresetList;

namespace rev {

namespace {

PresetList build ()
{
	return {
	    // Rooms
	    {"Small Room", "Rooms",
	     {{kMix, 20}, {kPreDelay, 5}, {kDecay, 600}, {kSize, 25}, {kDiffusion, 70}, {kEarly, 70}, {kDamping, 7000},
	      {kLowCut, 120}, {kHighCut, 11000}}},
	    {"Vocal Booth", "Rooms",
	     {{kMix, 15}, {kPreDelay, 3}, {kDecay, 400}, {kSize, 15}, {kDiffusion, 60}, {kEarly, 80}, {kDamping, 6000},
	      {kLowCut, 150}}},
	    {"Drum Room", "Rooms",
	     {{kMix, 22}, {kPreDelay, 8}, {kDecay, 900}, {kSize, 40}, {kDiffusion, 75}, {kEarly, 65}, {kDamping, 8000},
	      {kLowCut, 100}, {kHighCut, 13000}}},
	    {"Live Room", "Rooms",
	     {{kMix, 25}, {kPreDelay, 12}, {kDecay, 1300}, {kSize, 50}, {kDiffusion, 80}, {kEarly, 55}, {kDamping, 9000}}},
	    {"Short Ambience", "Rooms",
	     {{kMix, 18}, {kPreDelay, 0}, {kDecay, 300}, {kSize, 30}, {kDiffusion, 60}, {kEarly, 100}}},
	    // Halls
	    {"Concert Hall", "Halls",
	     {{kMix, 28}, {kPreDelay, 25}, {kDecay, 2800}, {kSize, 85}, {kDiffusion, 85}, {kEarly, 40}, {kDamping, 6500},
	      {kBassMult, 1.2}, {kLowCut, 80}, {kHighCut, 12000}, {kModDepth, 25}}},
	    {"Large Hall", "Halls",
	     {{kMix, 30}, {kPreDelay, 35}, {kDecay, 4200}, {kSize, 95}, {kDiffusion, 90}, {kEarly, 35}, {kDamping, 6000},
	      {kBassMult, 1.3}, {kModDepth, 30}}},
	    {"Dark Hall", "Halls",
	     {{kMix, 28}, {kPreDelay, 30}, {kDecay, 3500}, {kSize, 90}, {kDiffusion, 85}, {kEarly, 30}, {kDamping, 3000},
	      {kBassMult, 1.4}, {kHighCut, 7000}}},
	    // Plates
	    {"Vocal Plate", "Plates",
	     {{kMix, 22}, {kPreDelay, 20}, {kDecay, 1800}, {kSize, 55}, {kDiffusion, 95}, {kEarly, 20}, {kDamping, 9000},
	      {kLowCut, 200}, {kHighCut, 12000}, {kModDepth, 35}}},
	    {"Bright Plate", "Plates",
	     {{kMix, 25}, {kPreDelay, 10}, {kDecay, 2200}, {kSize, 60}, {kDiffusion, 95}, {kEarly, 15}, {kDamping, 14000},
	      {kLowCut, 250}, {kHighCut, 16000}}},
	    {"Snare Plate", "Plates",
	     {{kMix, 25}, {kPreDelay, 5}, {kDecay, 1400}, {kSize, 50}, {kDiffusion, 95}, {kEarly, 25}, {kDamping, 10000},
	      {kLowCut, 200}}},
	    // Spaces
	    {"Chamber", "Spaces",
	     {{kMix, 25}, {kPreDelay, 15}, {kDecay, 1600}, {kSize, 60}, {kDiffusion, 80}, {kEarly, 50}, {kDamping, 7000},
	      {kBassMult, 1.1}}},
	    {"Cathedral", "Spaces",
	     {{kMix, 35}, {kPreDelay, 45}, {kDecay, 8000}, {kSize, 100}, {kDiffusion, 90}, {kEarly, 30}, {kDamping, 5000},
	      {kBassMult, 1.5}, {kLowCut, 60}, {kModDepth, 30}}},
	    // Creative
	    {"Ambient Wash", "Creative",
	     {{kMix, 45}, {kPreDelay, 60}, {kDecay, 12000}, {kSize, 100}, {kDiffusion, 100}, {kEarly, 10}, {kDamping, 5500},
	      {kModDepth, 60}, {kModRate, 0.4}}},
	    {"Endless Pad", "Creative",
	     {{kMix, 50}, {kPreDelay, 40}, {kDecay, 15000}, {kSize, 100}, {kDiffusion, 100}, {kEarly, 0}, {kDamping, 12000},
	      {kLowCut, 300}, {kModDepth, 80}, {kModRate, 1.2}}},
	    {"Lo-Fi Space", "Creative",
	     {{kMix, 30}, {kDecay, 2000}, {kSize, 70}, {kDamping, 2000}, {kLowCut, 300}, {kHighCut, 4000},
	      {kModDepth, 50}, {kModRate, 2}}},
	};
}

} // namespace

const PresetList& presets ()
{
	static const PresetList list = build ();
	return list;
}

} // namespace rev
