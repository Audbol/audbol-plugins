#include "RevEditor.h"

#include "RevViews.h"

#include "suite/Brand.h"

#include "vstgui/lib/cframe.h"

#include <cmath>

using namespace VSTGUI;
using namespace suite::ui;

namespace rev {

namespace {
constexpr int kEditorWidth = 1000;
constexpr int kEditorHeight = 632;
constexpr float kTick = 0.033f;

float toDb (float g) { return g > 1e-6f ? 20.0f * std::log10 (g) : -120.0f; }
} // namespace

RevEditor::RevEditor (RevController* controller) : EditorBase (controller, kEditorWidth, kEditorHeight) {}

void RevEditor::build (CFrame*)
{
	add (new HeaderBar (CRect (0, 0, kEditorWidth, 44), SUITE_NAME, "Reverb"));
	addPresetBar (CRect (260, 10, 580, 34));
	bind (add (new Toggle (CRect (826, 11, 900, 33), this, kFreeze, "FREEZE")), kFreeze);
	bind (add (new Toggle (CRect (912, 11, 986, 33), this, kBypass, "BYPASS")), kBypass)->setAccent (colors::warning);

	// Displays
	decayView = add (new DecayView (CRect (12, 54, 480, 330), revCtrl ()));
	toneView = add (new ToneView (CRect (490, 54, 890, 330), revCtrl ()));
	add (new Panel (CRect (900, 54, 988, 330), colors::panel, 8));
	inMeter = add (new LevelMeter (CRect (912, 66, 938, 318), "IN", LevelMeter::Mode::Level, 60.0));
	outMeter = add (new LevelMeter (CRect (950, 66, 976, 318), "OUT", LevelMeter::Mode::Level, 60.0));

	auto knobRow = [this] (const uint32_t* ids, int count, CCoord x0, CCoord y, CCoord pitch) {
		for (int i = 0; i < count; ++i)
		{
			const CCoord x = x0 + i * pitch;
			bind (add (new Knob (CRect (x, y, x + 86, y + 100), this, ids[i], this)), ids[i]);
		}
	};

	add (new Panel (CRect (12, 340, 560, 482), colors::panel, 8, "SPACE"));
	const uint32_t space[] = {kMix, kPreDelay, kDecay, kSize, kDiffusion, kEarly};
	knobRow (space, 6, 20, 368, 90);

	add (new Panel (CRect (570, 340, 988, 482), colors::panel, 8, "TONE"));
	const uint32_t tone[] = {kDamping, kBassMult, kLowCut, kHighCut};
	knobRow (tone, 4, 586, 368, 100);

	add (new Panel (CRect (12, 492, 560, 620), colors::panel, 8, "MODULATION & STEREO"));
	const uint32_t mod[] = {kModDepth, kModRate, kWidth};
	knobRow (mod, 3, 20, 516, 90);

	add (new Panel (CRect (570, 492, 988, 620), colors::panel, 8, "TIPS"));
	const char* tips[] = {
	    "Decay view: drag for decay time, drag PRE for pre-delay, wheel for size",
	    "Tone view: LOW / MID handles drag vertically, DAMP and cuts horizontally",
	    "Shift = fine adjustment, double-click a handle to reset it",
	    "Freeze holds the current tail indefinitely",
	};
	for (int i = 0; i < 4; ++i)
		add (new Label (CRect (586, 524 + i * 20, 980, 540 + i * 20), tips[i], 10, colors::textFaint, 0));
}

void RevEditor::teardown ()
{
	decayView = nullptr;
	toneView = nullptr;
	inMeter = outMeter = nullptr;
}

void RevEditor::tick ()
{
	if (!decayView)
		return;
	auto lv = revCtrl ()->takeLevels ();
	inMeter->update (lv.fresh ? toDb (lv.inPeak) : -120.0f, kTick);
	outMeter->update (lv.fresh ? toDb (lv.outPeak) : -120.0f, kTick);
	toneView->refresh (kTick);
	decayView->invalid (); // follows knob and automation changes
}

} // namespace rev
