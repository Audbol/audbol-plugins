#include "LoudParams.h"

using namespace suite;

namespace loud {

namespace {

ParamSpec meter (uint32_t id, const char* name, const char* shortName, Unit unit, double min, double max)
{
	ParamSpec s {id, name, shortName, Taper::Linear, unit, min, max, min};
	s.flags = kReadOnly;
	return s;
}

ParamTable build ()
{
	ParamTable t;
	auto target = makeChoice (kTarget, "Target",
	                          {"-14 LUFS (Streaming)", "-16 LUFS (Apple)", "-23 LUFS (EBU R128)", "-24 LUFS (ATSC A/85)"}, 0);
	target.shortName = "Target";
	target.flags = 0; // a UI preference, not automatable
	t.push_back (target);
	auto reset = makeToggle (kReset, "Reset", false);
	reset.flags = 0;
	t.push_back (reset);

	t.push_back (meter (kMomentary, "Momentary", "M", Unit::Lufs, -70.0, 6.0));
	t.push_back (meter (kShortTerm, "Short-Term", "S", Unit::Lufs, -70.0, 6.0));
	t.push_back (meter (kIntegrated, "Integrated", "I", Unit::Lufs, -70.0, 6.0));
	t.push_back (meter (kRange, "Loudness Range", "LRA", Unit::LoudnessUnits, 0.0, 40.0));
	t.push_back (meter (kTruePeakMax, "True Peak Max", "TP", Unit::DbTruePeak, -70.0, 6.0));
	return t;
}

} // namespace

const ParamTable& params ()
{
	static const ParamTable table = build ();
	return table;
}

} // namespace loud
