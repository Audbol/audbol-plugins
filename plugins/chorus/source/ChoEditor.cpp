#include "ChoEditor.h"

#include "ChoViews.h"

#include "suite/Brand.h"

#include "vstgui/lib/cframe.h"

#include <cmath>

using namespace VSTGUI;
using namespace suite::ui;

namespace cho {

namespace {
constexpr int kEditorWidth = 1000;
constexpr int kEditorHeight = 500;
constexpr float kTick = 0.033f;

float toDb (float g) { return g > 1e-6f ? 20.0f * std::log10 (g) : -120.0f; }
} // namespace

ChoEditor::ChoEditor (ChoController* controller) : EditorBase (controller, kEditorWidth, kEditorHeight) {}

void ChoEditor::build (CFrame*)
{
	add (new HeaderBar (CRect (0, 0, kEditorWidth, 44), SUITE_NAME, "Chorus"));
	add (new Label (CRect (650, 12, 710, 32), "Voices", 11, colors::textDim, 2));
	bind (add (new Segmented (CRect (716, 11, 896, 33), this, kVoices, this)), kVoices);
	bind (add (new Toggle (CRect (912, 11, 986, 33), this, kBypass, "BYPASS")), kBypass)->setAccent (colors::warning);

	lfoView = add (new LfoView (CRect (12, 54, 500, 330), choCtrl ()));
	responseView = add (new ResponseView (CRect (510, 54, 900, 330), choCtrl ()));
	add (new Panel (CRect (910, 54, 988, 330), colors::panel, 8));
	inMeter = add (new LevelMeter (CRect (922, 66, 946, 318), "IN", LevelMeter::Mode::Level, 60.0));
	outMeter = add (new LevelMeter (CRect (954, 66, 978, 318), "OUT", LevelMeter::Mode::Level, 60.0));

	add (new Panel (CRect (12, 340, 560, 488), colors::panel, 8, "MODULATION"));
	const uint32_t mod[] = {kRate, kDepth, kDelay, kFeedback, kSpread};
	for (int i = 0; i < 5; ++i)
	{
		const CCoord x = 20 + i * 88;
		auto k = bind (add (new Knob (CRect (x, 368, x + 84, 468), this, mod[i], this)), mod[i]);
		if (mod[i] == kFeedback)
			k->setBipolar (true);
	}
	auto shapeMenu = bind (add (new ChoiceMenu (CRect (466, 382, 550, 422), this, kShape, this)), kShape);
	shapeMenu->setLabel ("Shape");

	add (new Panel (CRect (570, 340, 988, 488), colors::panel, 8, "OUTPUT"));
	const uint32_t outKnobs[] = {kMix, kWidth, kLowCut, kHighCut};
	for (int i = 0; i < 4; ++i)
	{
		const CCoord x = 584 + i * 100;
		bind (add (new Knob (CRect (x, 368, x + 90, 468), this, outKnobs[i], this)), outKnobs[i]);
	}
}

void ChoEditor::teardown ()
{
	lfoView = nullptr;
	responseView = nullptr;
	inMeter = outMeter = nullptr;
}

void ChoEditor::tick ()
{
	if (!lfoView)
		return;
	auto lv = choCtrl ()->takeLevels ();
	inMeter->update (lv.fresh ? toDb (lv.inPeak) : -120.0f, kTick);
	outMeter->update (lv.fresh ? toDb (lv.outPeak) : -120.0f, kTick);
	lfoView->advance (kTick);
	responseView->setPhase (lfoView->phase ());
	responseView->invalid ();
}

} // namespace cho
