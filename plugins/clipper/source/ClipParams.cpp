#include "ClipParams.h"

using namespace suite;

namespace clip {

namespace {

ParamTable build ()
{
	ParamTable t;
	auto add = [&] (ParamSpec s, const char* shortName) {
		s.shortName = shortName;
		t.push_back (std::move (s));
	};
	t.push_back (makeBypass (kBypass));
	add (makeLinear (kInput, "Input", Unit::Decibel, -12.0, 24.0, 0.0), "Input");
	add (makeLinear (kCeiling, "Ceiling", Unit::Decibel, -24.0, 0.0, -0.3), "Ceiling");
	add (makeLinear (kSoftness, "Softness", Unit::Percent, 0.0, 100.0, 30.0), "Soft");
	add (makeLinear (kOutput, "Output", Unit::Decibel, -24.0, 12.0, 0.0), "Output");
	add (makeToggle (kMatch, "Gain Match", false), "Match");
	add (makeToggle (kDelta, "Delta", false), "Delta");
	return t;
}

} // namespace

const ParamTable& params ()
{
	static const ParamTable table = build ();
	return table;
}

} // namespace clip
