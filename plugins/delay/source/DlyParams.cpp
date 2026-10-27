#include "DlyParams.h"

using namespace suite;

namespace dly {

namespace {

ParamTable build ()
{
	ParamTable t;
	auto add = [&] (ParamSpec s, const char* shortName) {
		s.shortName = shortName;
		t.push_back (std::move (s));
	};
	std::vector<std::string> notes (kNoteNames, kNoteNames + kNumNotes);

	t.push_back (makeBypass (kBypass));
	add (makeLinear (kMix, "Mix", Unit::Percent, 0.0, 100.0, 35.0), "Mix");
	add (makeToggle (kSync, "Sync", true), "Sync");
	add (makeLog (kTimeL, "Time Left", Unit::Milliseconds, kMinTimeMs, kMaxTimeMs, 375.0), "Time L");
	add (makeLog (kTimeR, "Time Right", Unit::Milliseconds, kMinTimeMs, kMaxTimeMs, 500.0), "Time R");
	add (makeChoice (kNoteL, "Note Left", notes, 5), "Note L");  // 1/8
	add (makeChoice (kNoteR, "Note Right", notes, 7), "Note R"); // 1/8D
	add (makeToggle (kLink, "Link L/R", true), "Link");
	add (makeLinear (kFeedback, "Feedback", Unit::Percent, 0.0, 100.0, 40.0), "Feedback");
	add (makeChoice (kMode, "Mode", {"Stereo", "Ping-Pong", "Cross"}, 0), "Mode");
	add (makeLog (kLowCut, "Low Cut", Unit::Hz, 20.0, 2000.0, 100.0), "Low Cut");
	add (makeLog (kHighCut, "High Cut", Unit::Hz, 1000.0, 20000.0, 8000.0), "High Cut");
	add (makeLinear (kDrive, "Drive", Unit::Percent, 0.0, 100.0, 0.0), "Drive");
	add (makeLinear (kModDepth, "Mod Depth", Unit::Percent, 0.0, 100.0, 0.0), "Mod Depth");
	add (makeLog (kModRate, "Mod Rate", Unit::Hz, 0.1, 5.0, 0.6), "Mod Rate");
	add (makeLinear (kWidth, "Width", Unit::Percent, 0.0, 100.0, 100.0), "Width");
	add (makeLinear (kDucking, "Ducking", Unit::Percent, 0.0, 100.0, 0.0), "Ducking");
	return t;
}

} // namespace

const ParamTable& params ()
{
	static const ParamTable table = build ();
	return table;
}

} // namespace dly
