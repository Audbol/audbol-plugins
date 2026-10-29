#include "DessParams.h"

using namespace suite;

namespace dess {

namespace {

ParamTable build ()
{
	ParamTable t;
	auto add = [&] (ParamSpec s, const char* shortName) {
		s.shortName = shortName;
		t.push_back (std::move (s));
	};
	t.push_back (makeBypass (kBypass));
	add (makeLinear (kThreshold, "Threshold", Unit::Decibel, -60.0, 0.0, -30.0), "Threshold");
	add (makeLinear (kRange, "Range", Unit::Decibel, 0.0, 24.0, 8.0), "Range");
	add (makeLog (kFrequency, "Frequency", Unit::Hz, 2000.0, 16000.0, 6500.0), "Freq");
	add (makeLog (kQ, "Q", Unit::None, 0.5, 6.0, 1.5), "Q");
	add (makeLog (kRelease, "Release", Unit::Milliseconds, 5.0, 250.0, 40.0), "Release");
	add (makeChoice (kFilter, "Filter", {"Band", "High"}, 0), "Filter");
	add (makeChoice (kMode, "Mode", {"Split", "Wide"}, 0), "Mode");
	add (makeToggle (kListen, "Listen", false), "Listen");
	add (makeToggle (kStereoLink, "Stereo Link", true), "Link");
	add (makeLinear (kOutput, "Output", Unit::Decibel, -12.0, 12.0, 0.0), "Output");
	return t;
}

} // namespace

const ParamTable& params ()
{
	static const ParamTable table = build ();
	return table;
}

} // namespace dess
