#include "UtlParams.h"

using namespace suite;

namespace utl {

namespace {

ParamTable build ()
{
	ParamTable t;
	auto add = [&] (ParamSpec s, const char* shortName) {
		s.shortName = shortName;
		t.push_back (std::move (s));
	};
	t.push_back (makeBypass (kBypass));
	add (makeLinear (kGain, "Gain", Unit::Decibel, -48.0, 24.0, 0.0), "Gain");
	add (makeToggle (kMute, "Mute", false), "Mute");
	add (makeToggle (kInvertL, "Invert Left", false), "Inv L");
	add (makeToggle (kInvertR, "Invert Right", false), "Inv R");
	add (makeChoice (kChannelMode, "Channels", {"Stereo", "Mono", "Left", "Right", "Swap"}, 0), "Channels");
	add (makeLinear (kWidth, "Width", Unit::Percent, 0.0, 200.0, 100.0), "Width");
	add (makeLinear (kPan, "Pan", Unit::Pan, -100.0, 100.0, 0.0), "Pan");
	add (makeToggle (kBassMono, "Bass Mono", false), "Bass Mono");
	add (makeLog (kBassMonoFreq, "Bass Mono Freq", Unit::Hz, 20.0, 500.0, 120.0), "Mono Below");
	add (makeToggle (kDcFilter, "DC Filter", false), "DC Filter");
	return t;
}

} // namespace

const ParamTable& params ()
{
	static const ParamTable table = build ();
	return table;
}

} // namespace utl
