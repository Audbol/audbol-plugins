#include "SatEditor.h"

#include "SatViews.h"

#include "suite/Brand.h"

#include "vstgui/lib/cframe.h"

#include <algorithm>
#include <cmath>

using namespace VSTGUI;
using namespace suite::ui;

namespace sat {

namespace {
constexpr int kEditorWidth = 1000;
constexpr int kEditorHeight = 520;
constexpr float kTick = 0.033f;

float toDb (float g) { return g > 1e-6f ? 20.0f * std::log10 (g) : -120.0f; }
} // namespace

SatEditor::SatEditor (SatController* controller) : EditorBase (controller, kEditorWidth, kEditorHeight) {}

void SatEditor::build (CFrame*)
{
	add (new HeaderBar (CRect (0, 0, kEditorWidth, 44), SUITE_NAME, "Saturation"));
	add (new Label (CRect (480, 12, 536, 32), "Model", 11, colors::textDim, 2));
	bind (add (new Segmented (CRect (542, 11, 852, 33), this, kModel, this)), kModel);
	bind (add (new Toggle (CRect (912, 11, 986, 33), this, kBypass, "BYPASS")), kBypass)->setAccent (colors::warning);

	curve = add (new CurveView (CRect (12, 54, 332, 354), satCtrl ()));
	harmonics = add (new HarmonicsView (CRect (342, 54, 860, 354), satCtrl ()));
	add (new Panel (CRect (870, 54, 988, 354), colors::panel, 8));
	inMeter = add (new LevelMeter (CRect (888, 66, 916, 342), "IN", LevelMeter::Mode::Level, 60.0));
	outMeter = add (new LevelMeter (CRect (942, 66, 970, 342), "OUT", LevelMeter::Mode::Level, 60.0));

	add (new Panel (CRect (12, 364, 988, 508), colors::panel, 8));
	const uint32_t knobs[] = {kDrive, kBias, kTone, kMix, kOutput};
	for (int i = 0; i < 5; ++i)
	{
		const CCoord x = 30 + i * 110;
		auto k = bind (add (new Knob (CRect (x, 380, x + 90, 480), this, knobs[i], this)), knobs[i]);
		if (knobs[i] == kBias || knobs[i] == kTone || knobs[i] == kOutput)
			k->setBipolar (true);
		if (knobs[i] == kOutput)
			outputKnob = k;
	}
	bind (add (new Toggle (CRect (590, 412, 690, 436), this, kAutoGain, "AUTO GAIN")), kAutoGain);
	const char* tips[] = {
	    "Curve: drag up/down = drive",
	    "left/right = bias (even harmonics)",
	    "4x oversampled, tone tilts at 1 kHz",
	};
	for (int i = 0; i < 3; ++i)
		add (new Label (CRect (712, 396 + i * 20, 980, 412 + i * 20), tips[i], 10, colors::textFaint, 0));
}

void SatEditor::teardown ()
{
	curve = nullptr;
	harmonics = nullptr;
	inMeter = outMeter = nullptr;
	outputKnob = nullptr;
}

void SatEditor::tick ()
{
	if (!curve)
		return;
	auto lv = satCtrl ()->takeLevels ();
	inMeter->update (lv.fresh ? toDb (lv.inPeak) : -120.0f, kTick);
	outMeter->update (lv.fresh ? toDb (lv.outPeak) : -120.0f, kTick);

	// Peak with a gentle fall so the dot and harmonics are readable
	const float in = lv.fresh ? lv.inPeak : 0.0f;
	inputPeak = in >= inputPeak ? in : std::max (in, inputPeak * 0.9f);
	curve->setInputPeak (inputPeak);
	curve->invalid ();
	harmonics->setInputLevelDb (inputPeak > 0.001f ? toDb (inputPeak) : -12.0f);
	harmonics->invalid ();
}

} // namespace sat
