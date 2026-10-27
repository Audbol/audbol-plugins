#include "LimParams.h"

using namespace suite;

namespace lim {

namespace {

ParamTable build ()
{
	ParamTable t;
	auto add = [&] (ParamSpec s, const char* shortName) {
		s.shortName = shortName;
		t.push_back (std::move (s));
	};
	t.push_back (makeBypass (kBypass));
	add (makeLinear (kInputGain, "Input Gain", Unit::Decibel, 0.0, 30.0, 0.0), "Gain");
	add (makeLinear (kCeiling, "Ceiling", Unit::Decibel, -12.0, 0.0, -0.3), "Ceiling");
	add (makeLog (kRelease, "Release", Unit::Milliseconds, 1.0, 1000.0, 60.0), "Release");
	add (makeToggle (kAutoRelease, "Auto Release", true), "Auto Rel");
	add (makeLinear (kStereoLink, "Stereo Link", Unit::Percent, 0.0, 100.0, 100.0), "Link");
	add (makeToggle (kTruePeak, "True Peak", true), "True Peak");
	return t;
}

} // namespace

const ParamTable& params ()
{
	static const ParamTable table = build ();
	return table;
}

} // namespace lim
