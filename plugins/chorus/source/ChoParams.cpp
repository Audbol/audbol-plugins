#include "ChoParams.h"

using namespace suite;

namespace cho {

namespace {

ParamTable build ()
{
	ParamTable t;
	auto add = [&] (ParamSpec s, const char* shortName) {
		s.shortName = shortName;
		t.push_back (std::move (s));
	};
	t.push_back (makeBypass (kBypass));
	add (makeLinear (kMix, "Mix", Unit::Percent, 0.0, 100.0, 50.0), "Mix");
	add (makeLog (kRate, "Rate", Unit::Hz, 0.05, 10.0, 0.6), "Rate");
	add (makeLinear (kDepth, "Depth", Unit::Percent, 0.0, 100.0, 40.0), "Depth");
	add (makeLog (kDelay, "Delay", Unit::Milliseconds, kMinDelayMs, kMaxDelayMs, 12.0), "Delay");
	add (makeChoice (kVoices, "Voices", {"1", "2", "3", "4"}, 1), "Voices");
	add (makeLinear (kSpread, "Stereo Spread", Unit::Degrees, 0.0, 180.0, 90.0), "Spread");
	add (makeLinear (kFeedback, "Feedback", Unit::Percent, -90.0, 90.0, 0.0), "Feedback");
	add (makeChoice (kShape, "LFO Shape", {"Sine", "Triangle"}, 0), "Shape");
	add (makeLinear (kWidth, "Width", Unit::Percent, 0.0, 100.0, 100.0), "Width");
	add (makeLog (kLowCut, "Low Cut", Unit::Hz, 20.0, 1000.0, 80.0), "Low Cut");
	add (makeLog (kHighCut, "High Cut", Unit::Hz, 1000.0, 20000.0, 14000.0), "High Cut");
	return t;
}

} // namespace

const ParamTable& params ()
{
	static const ParamTable table = build ();
	return table;
}

} // namespace cho
