#include "RevParams.h"

using namespace suite;

namespace rev {

namespace {

ParamTable build ()
{
	ParamTable t;
	auto add = [&] (ParamSpec s, const char* shortName) {
		s.shortName = shortName;
		t.push_back (std::move (s));
	};
	t.push_back (makeBypass (kBypass));
	add (makeLinear (kMix, "Mix", Unit::Percent, 0.0, 100.0, 30.0), "Mix");
	add (makePower (kPreDelay, "Pre-Delay", Unit::Milliseconds, 0.0, kMaxPreDelayMs, 20.0, 2.0), "Pre-Delay");
	add (makeLog (kDecay, "Decay", Unit::Milliseconds, 200.0, 20000.0, 2500.0), "Decay");
	add (makeLinear (kSize, "Size", Unit::Percent, 0.0, 100.0, 60.0), "Size");
	add (makeLinear (kDiffusion, "Diffusion", Unit::Percent, 0.0, 100.0, 80.0), "Diffusion");
	add (makeLinear (kEarly, "Early Reflections", Unit::Percent, 0.0, 100.0, 50.0), "Early");
	add (makeLinear (kWidth, "Width", Unit::Percent, 0.0, 100.0, 100.0), "Width");
	add (makeLog (kDamping, "Damping", Unit::Hz, 1000.0, 20000.0, 6000.0), "Damping");
	add (makeLog (kBassMult, "Bass Multiplier", Unit::None, 0.5, 2.0, 1.0), "Bass Mult");
	add (makeLog (kLowCut, "Low Cut", Unit::Hz, 20.0, 1000.0, 60.0), "Low Cut");
	add (makeLog (kHighCut, "High Cut", Unit::Hz, 1000.0, 20000.0, 12000.0), "High Cut");
	add (makeLinear (kModDepth, "Mod Depth", Unit::Percent, 0.0, 100.0, 30.0), "Mod Depth");
	add (makeLog (kModRate, "Mod Rate", Unit::Hz, 0.1, 5.0, 0.8), "Mod Rate");
	add (makeToggle (kFreeze, "Freeze", false), "Freeze");
	return t;
}

} // namespace

const ParamTable& params ()
{
	static const ParamTable table = build ();
	return table;
}

} // namespace rev
