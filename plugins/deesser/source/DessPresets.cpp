#include "DessParams.h"

using suite::PresetList;

namespace dess {

namespace {

constexpr double kBand = 0, kHigh = 1;
constexpr double kSplit = 0, kWide = 1;

PresetList build ()
{
	return {
	    // Vocals
	    {"Female Vocal", "Vocals",
	     {{kThreshold, -32}, {kRange, 8}, {kFrequency, 7500}, {kQ, 1.5}, {kRelease, 40}, {kFilter, kBand}}},
	    {"Male Vocal", "Vocals",
	     {{kThreshold, -30}, {kRange, 8}, {kFrequency, 5500}, {kQ, 1.5}, {kRelease, 40}, {kFilter, kBand}}},
	    {"Gentle Vocal", "Vocals",
	     {{kThreshold, -28}, {kRange, 4}, {kFrequency, 6500}, {kQ, 1.2}, {kRelease, 60}, {kFilter, kBand}}},
	    {"Strong Vocal", "Vocals",
	     {{kThreshold, -38}, {kRange, 14}, {kFrequency, 6500}, {kQ, 1.0}, {kRelease, 30}, {kFilter, kBand}}},
	    {"Rap Vocal", "Vocals",
	     {{kThreshold, -34}, {kRange, 10}, {kFrequency, 6000}, {kQ, 2.0}, {kRelease, 25}, {kFilter, kBand}}},
	    {"Airy Vocal", "Vocals",
	     {{kThreshold, -36}, {kRange, 6}, {kFrequency, 9000}, {kRelease, 45}, {kFilter, kHigh}, {kMode, kSplit}}},
	    {"Backing Vocals", "Vocals",
	     {{kThreshold, -36}, {kRange, 10}, {kFrequency, 7000}, {kRelease, 50}, {kFilter, kHigh}}},
	    {"Podcast Voice", "Vocals",
	     {{kThreshold, -30}, {kRange, 8}, {kFrequency, 5000}, {kQ, 1.0}, {kRelease, 50}, {kFilter, kBand}}},
	    {"Classic Wideband", "Vocals",
	     {{kThreshold, -30}, {kRange, 6}, {kFrequency, 6500}, {kQ, 1.2}, {kRelease, 50}, {kFilter, kBand},
	      {kMode, kWide}}},
	    // Instruments
	    {"Hi-Hat Tamer", "Instruments",
	     {{kThreshold, -30}, {kRange, 8}, {kFrequency, 9000}, {kQ, 1.2}, {kRelease, 30}, {kFilter, kBand}}},
	    {"Cymbal Wash Control", "Instruments",
	     {{kThreshold, -34}, {kRange, 6}, {kFrequency, 8000}, {kRelease, 80}, {kFilter, kHigh}}},
	    {"Guitar Squeak", "Instruments",
	     {{kThreshold, -36}, {kRange, 8}, {kFrequency, 3500}, {kQ, 2.5}, {kRelease, 20}, {kFilter, kBand}}},
	    {"Harsh Synth", "Instruments",
	     {{kThreshold, -32}, {kRange, 6}, {kFrequency, 4000}, {kQ, 1.0}, {kRelease, 60}, {kFilter, kBand}}},
	    // Mix & Master
	    {"Mix Bus Gentle", "Mix & Master",
	     {{kThreshold, -24}, {kRange, 3}, {kFrequency, 7000}, {kRelease, 80}, {kFilter, kHigh}}},
	    {"Master Sibilance", "Mix & Master",
	     {{kThreshold, -26}, {kRange, 3}, {kFrequency, 7000}, {kQ, 1.0}, {kRelease, 60}, {kFilter, kBand}}},
	};
}

} // namespace

const PresetList& presets ()
{
	static const PresetList list = build ();
	return list;
}

} // namespace dess
