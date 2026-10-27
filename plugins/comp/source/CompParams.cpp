#include "CompParams.h"

using namespace suite;

namespace comp {

namespace {

ParamTable build ()
{
	ParamTable t;
	auto add = [&] (ParamSpec s, const char* shortName) {
		s.shortName = shortName;
		t.push_back (std::move (s));
	};
	t.push_back (makeBypass (kBypass));
	add (makeLinear (kThreshold, "Threshold", Unit::Decibel, -60.0, 0.0, -18.0), "Threshold");
	add (makeLog (kRatio, "Ratio", Unit::Ratio, 1.0, kMaxRatio, 4.0), "Ratio");
	add (makeLinear (kKnee, "Knee", Unit::Decibel, 0.0, 24.0, 6.0), "Knee");
	add (makeLog (kAttack, "Attack", Unit::Milliseconds, 0.05, 250.0, 10.0), "Attack");
	add (makeLog (kRelease, "Release", Unit::Milliseconds, 5.0, 2500.0, 150.0), "Release");
	add (makeToggle (kAutoRelease, "Auto Release", false), "Auto Rel");
	add (makeLinear (kMakeup, "Makeup Gain", Unit::Decibel, -12.0, 36.0, 0.0), "Makeup");
	add (makeToggle (kAutoMakeup, "Auto Makeup", false), "Auto Gain");
	add (makeLinear (kMix, "Mix", Unit::Percent, 0.0, 100.0, 100.0), "Mix");
	add (makeChoice (kDetection, "Detection", {"Peak", "RMS"}, 0), "Detect");
	add (makeLinear (kStereoLink, "Stereo Link", Unit::Percent, 0.0, 100.0, 100.0), "Link");
	add (makeToggle (kScFilterOn, "Sidechain HPF", false), "SC HPF");
	add (makeLog (kScFilterFreq, "Sidechain HPF Freq", Unit::Hz, 20.0, 500.0, 100.0), "SC Freq");
	return t;
}

} // namespace

const ParamTable& params ()
{
	static const ParamTable table = build ();
	return table;
}

} // namespace comp
