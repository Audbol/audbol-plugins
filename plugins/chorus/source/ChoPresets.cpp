#include "ChoParams.h"

#include "suite/Presets.h"

using suite::PresetList;

namespace cho {

namespace {

// Voices parameter is an index: 0 = 1 voice ... 3 = 4 voices
constexpr double v1 = 0, v2 = 1, v3 = 2, v4 = 3;
constexpr double Sine = 0, Triangle = 1;

PresetList build ()
{
	return {
	    // Chorus
	    {"Classic Chorus", "Chorus", {{kMix, 50}, {kRate, 0.6}, {kDepth, 40}, {kDelay, 12}, {kVoices, v2}, {kSpread, 90}}},
	    {"Subtle Width", "Chorus", {{kMix, 30}, {kRate, 0.3}, {kDepth, 20}, {kDelay, 15}, {kVoices, v2}, {kSpread, 120}}},
	    {"Lush Chorus", "Chorus", {{kMix, 50}, {kRate, 0.5}, {kDepth, 55}, {kDelay, 18}, {kVoices, v4}, {kSpread, 120}}},
	    {"80s Guitar", "Chorus", {{kMix, 50}, {kRate, 0.9}, {kDepth, 50}, {kDelay, 9}, {kVoices, v2}, {kSpread, 180}}},
	    {"Bass Chorus", "Chorus",
	     {{kMix, 45}, {kRate, 0.5}, {kDepth, 35}, {kDelay, 10}, {kVoices, v2}, {kLowCut, 200}}},
	    // Doubler & Ensemble
	    {"Vocal Doubler", "Doubler & Ensemble",
	     {{kMix, 40}, {kRate, 0.15}, {kDepth, 15}, {kDelay, 25}, {kVoices, v2}, {kSpread, 180}}},
	    {"Wide Doubler", "Doubler & Ensemble",
	     {{kMix, 50}, {kRate, 0.1}, {kDepth, 10}, {kDelay, 30}, {kVoices, v3}, {kSpread, 180}}},
	    {"Ensemble Strings", "Doubler & Ensemble",
	     {{kMix, 60}, {kRate, 0.8}, {kDepth, 45}, {kDelay, 14}, {kVoices, v4}, {kSpread, 150}, {kShape, Triangle}}},
	    // Flanger
	    {"Classic Flanger", "Flanger",
	     {{kMix, 50}, {kRate, 0.25}, {kDepth, 80}, {kDelay, 2}, {kVoices, v1}, {kFeedback, 60}}},
	    {"Jet Flanger", "Flanger", {{kMix, 50}, {kRate, 0.1}, {kDepth, 100}, {kDelay, 1}, {kVoices, v1}, {kFeedback, 85}}},
	    {"Negative Flanger", "Flanger",
	     {{kMix, 50}, {kRate, 0.3}, {kDepth, 70}, {kDelay, 1.5}, {kVoices, v1}, {kFeedback, -70}}},
	    // Vibrato
	    {"Vibrato", "Vibrato", {{kMix, 100}, {kRate, 5}, {kDepth, 30}, {kDelay, 5}, {kVoices, v1}, {kSpread, 0}}},
	    {"Slow Vibrato", "Vibrato", {{kMix, 100}, {kRate, 1.5}, {kDepth, 40}, {kDelay, 8}, {kVoices, v1}, {kSpread, 0}}},
	    {"Rotary Speaker", "Vibrato",
	     {{kMix, 60}, {kRate, 6}, {kDepth, 25}, {kDelay, 4}, {kVoices, v2}, {kSpread, 180}, {kShape, Sine}}},
	};
}

} // namespace

const PresetList& presets ()
{
	static const PresetList list = build ();
	return list;
}

} // namespace cho
