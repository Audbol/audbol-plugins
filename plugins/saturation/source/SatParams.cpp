#include "SatParams.h"

using namespace suite;

namespace sat {

namespace {

ParamTable build ()
{
	ParamTable t;
	auto add = [&] (ParamSpec s, const char* shortName) {
		s.shortName = shortName;
		t.push_back (std::move (s));
	};
	t.push_back (makeBypass (kBypass));
	add (makeLinear (kDrive, "Drive", Unit::Decibel, 0.0, 36.0, 6.0), "Drive");
	add (makeChoice (kModel, "Model", {"Tape", "Tube", "Warm", "Hard", "Fold"}, 0), "Model");
	add (makeLinear (kBias, "Bias", Unit::Percent, -100.0, 100.0, 0.0), "Bias");
	add (makeLinear (kTone, "Tone", Unit::Decibel, -12.0, 12.0, 0.0), "Tone");
	add (makeLinear (kMix, "Mix", Unit::Percent, 0.0, 100.0, 100.0), "Mix");
	add (makeLinear (kOutput, "Output", Unit::Decibel, -24.0, 12.0, 0.0), "Output");
	add (makeToggle (kAutoGain, "Auto Gain", true), "Auto Gain");
	return t;
}

} // namespace

const ParamTable& params ()
{
	static const ParamTable table = build ();
	return table;
}

} // namespace sat
