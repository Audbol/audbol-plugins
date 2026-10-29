#include "TransParams.h"

using namespace suite;

namespace trans {

namespace {

ParamTable build ()
{
	ParamTable t;
	auto add = [&] (ParamSpec s, const char* shortName) {
		s.shortName = shortName;
		t.push_back (std::move (s));
	};
	t.push_back (makeBypass (kBypass));
	add (makeLinear (kAttack, "Attack", Unit::Percent, -100.0, 100.0, 0.0), "Attack");
	add (makeLinear (kSustain, "Sustain", Unit::Percent, -100.0, 100.0, 0.0), "Sustain");
	add (makeLog (kAttackTime, "Attack Time", Unit::Milliseconds, 2.0, 60.0, 15.0), "Atk Time");
	add (makeLog (kSustainTime, "Sustain Time", Unit::Milliseconds, 40.0, 1500.0, 250.0), "Sus Time");
	add (makeLinear (kMix, "Mix", Unit::Percent, 0.0, 100.0, 100.0), "Mix");
	add (makeLinear (kOutput, "Output", Unit::Decibel, -12.0, 12.0, 0.0), "Output");
	return t;
}

} // namespace

const ParamTable& params ()
{
	static const ParamTable table = build ();
	return table;
}

} // namespace trans
