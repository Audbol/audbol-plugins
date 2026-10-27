#include "LoudEditor.h"

#include "LoudViews.h"

#include "suite/Brand.h"

#include "vstgui/lib/cframe.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace VSTGUI;
using namespace suite::ui;

namespace loud {

namespace {
constexpr int kEditorWidth = 1000;
constexpr int kEditorHeight = 560;
constexpr float kTick = 0.033f;
const CColor kGreen {105, 219, 124, 255};

std::string lufsText (double v)
{
	if (v <= -69.9)
		return "-inf";
	char buf[16];
	std::snprintf (buf, sizeof (buf), "%.1f", v);
	return buf;
}
} // namespace

LoudEditor::LoudEditor (LoudController* controller) : EditorBase (controller, kEditorWidth, kEditorHeight) {}

void LoudEditor::build (CFrame*)
{
	add (new HeaderBar (CRect (0, 0, kEditorWidth, 44), SUITE_NAME, "Loudness"));
	add (new Label (CRect (560, 12, 616, 32), "Target", 11, colors::textDim, 2));
	bind (add (new ChoiceMenu (CRect (622, 11, 812, 33), this, kTarget, this)), kTarget);
	add (new PushButton (CRect (824, 11, 986, 33), "RESET INTEGRATED", [this] { loudCtrl ()->requestReset (); }));

	integrated = add (new ReadoutTile (CRect (12, 54, 330, 184), "INTEGRATED", "LUFS", 46));
	shortTerm = add (new ReadoutTile (CRect (340, 54, 502, 184), "SHORT-TERM", "LUFS", 28));
	momentary = add (new ReadoutTile (CRect (512, 54, 674, 184), "MOMENTARY", "LUFS", 28));
	range = add (new ReadoutTile (CRect (684, 54, 830, 184), "LOUDNESS RANGE", "LU", 28));
	truePeak = add (new ReadoutTile (CRect (840, 54, 988, 184), "TRUE PEAK MAX", "dBTP", 28));

	history = add (new LoudnessHistory (CRect (12, 194, 890, 548)));
	add (new Panel (CRect (900, 194, 988, 548), colors::panel, 8));
	meterM = add (new LevelMeter (CRect (914, 206, 940, 536), "M", LevelMeter::Mode::Level, 48.0));
	meterS = add (new LevelMeter (CRect (948, 206, 974, 536), "S", LevelMeter::Mode::Level, 48.0));
}

void LoudEditor::teardown ()
{
	integrated = shortTerm = momentary = range = truePeak = nullptr;
	history = nullptr;
	meterM = meterS = nullptr;
}

void LoudEditor::tick ()
{
	if (!history)
		return;
	const auto& v = loudCtrl ()->latest ();
	const double target = kTargets[std::clamp (static_cast<int> (ctrl ()->getPlain (kTarget)), 0, 3)];
	char buf[64];

	// Integrated, coloured against the target (+-1 LU is on target)
	CColor iColor = colors::text;
	std::string status = "Waiting for audio";
	if (v.integrated > -69.9f)
	{
		const double diff = v.integrated - target;
		iColor = std::abs (diff) <= 1.0 ? kGreen : diff > 0.0 ? colors::warning : colors::accent;
		std::snprintf (buf, sizeof (buf), "%+.1f dB gain to reach %g LUFS", -diff, target);
		status = std::abs (diff) <= 1.0 ? "On target" : buf;
	}
	integrated->set (lufsText (v.integrated), iColor, status);
	shortTerm->set (lufsText (v.shortTerm), colors::text);
	momentary->set (lufsText (v.momentary), colors::text);
	std::snprintf (buf, sizeof (buf), "%.1f", v.range);
	range->set (buf, colors::text);
	const bool overPeak = v.truePeakMax > kTruePeakLimit;
	std::snprintf (buf, sizeof (buf), "%.1f", v.truePeakMax);
	truePeak->set (v.truePeakMax <= -69.9f ? "-inf" : buf, overPeak ? colors::danger : colors::text,
	               overPeak ? "over -1 dBTP" : "");

	loudCtrl ()->copyHistory (histM, histS);
	history->set (histM, histS, v.integrated, target);
	meterM->update (v.momentary, kTick);
	meterS->update (v.shortTerm, kTick);
}

} // namespace loud
