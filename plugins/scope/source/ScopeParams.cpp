#include "ScopeParams.h"

using namespace suite;

namespace scope {

namespace {

// Display settings are saved with the plug-in but not automated.
ParamSpec setting (ParamSpec s, const char* shortName)
{
	s.shortName = shortName;
	s.flags = 0;
	return s;
}

ParamSpec measurement (uint32_t id, const char* name, Unit unit, double min, double max)
{
	ParamSpec s {id, name, name, Taper::Linear, unit, min, max, min};
	s.flags = kReadOnly;
	return s;
}

ParamTable build ()
{
	ParamTable t;
	t.push_back (setting (makeLog (kTime, "Time", Unit::Milliseconds, 1.0, 2000.0, 20.0), "Time"));
	t.push_back (setting (makeChoice (kTrigger, "Trigger", {"Free", "Rising Edge", "Beat Sync"}, 1), "Trigger"));
	t.push_back (setting (makeLinear (kTriggerLevel, "Trigger Level", Unit::Percent, -100.0, 100.0, 0.0), "Level"));
	t.push_back (setting (makeChoice (kChannels, "Channels", {"L + R", "Mid", "Side", "Left", "Right"}, 0), "Channels"));
	t.push_back (setting (makeLinear (kGain, "Vertical Gain", Unit::Decibel, -12.0, 36.0, 0.0), "Gain"));
	t.push_back (setting (makeToggle (kFreeze, "Freeze", false), "Freeze"));
	t.push_back (setting (makeChoice (kBeatLength, "Beat Length",
	                                  std::vector<std::string> (kBeatLengthNames, kBeatLengthNames + kNumBeatLengths), 2),
	                      "Length"));
	t.push_back (measurement (kFrequency, "Frequency", Unit::Hz, 0.0, 20000.0));
	t.push_back (measurement (kPeak, "Peak", Unit::Decibel, -70.0, 6.0));
	return t;
}

} // namespace

const ParamTable& params ()
{
	static const ParamTable table = build ();
	return table;
}

} // namespace scope
