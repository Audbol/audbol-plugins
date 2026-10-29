#include "GateParams.h"

using namespace suite;

namespace gate {

namespace {

ParamTable build ()
{
	ParamTable t;
	auto add = [&] (ParamSpec s, const char* shortName) {
		s.shortName = shortName;
		t.push_back (std::move (s));
	};
	t.push_back (makeBypass (kBypass));
	add (makeLinear (kThreshold, "Threshold", Unit::Decibel, -80.0, 0.0, -40.0), "Threshold");
	add (makeLinear (kRange, "Range", Unit::Decibel, 0.0, kMaxRangeDb, 40.0), "Range");
	add (makeChoice (kMode, "Mode", {"Gate", "Expand"}, 0), "Mode");
	add (makeLog (kRatio, "Ratio", Unit::Ratio, 1.2, 10.0, 2.0), "Ratio");
	add (makeLinear (kHysteresis, "Hysteresis", Unit::Decibel, 0.0, 12.0, 4.0), "Hyst");
	add (makeLog (kAttack, "Attack", Unit::Milliseconds, 0.01, 50.0, 0.5), "Attack");
	add (makePower (kHold, "Hold", Unit::Milliseconds, 0.0, 500.0, 50.0, 2.0), "Hold");
	add (makeLog (kRelease, "Release", Unit::Milliseconds, 5.0, 2000.0, 150.0), "Release");
	add (makeLog (kKeyHpf, "Key High-Pass", Unit::Hz, 20.0, 4000.0, 20.0), "Key HPF");
	add (makeLog (kKeyLpf, "Key Low-Pass", Unit::Hz, 500.0, 20000.0, 20000.0), "Key LPF");
	add (makeToggle (kListen, "Key Listen", false), "Listen");
	return t;
}

} // namespace

const ParamTable& params ()
{
	static const ParamTable table = build ();
	return table;
}

} // namespace gate
