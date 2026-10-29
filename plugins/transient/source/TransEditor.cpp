#include "TransEditor.h"

#include "TransientView.h"

#include "suite/Brand.h"

#include "vstgui/lib/cframe.h"

#include <cmath>

using namespace VSTGUI;
using namespace suite::ui;

namespace trans {

namespace {
constexpr int kWidth = 1000;
constexpr int kHeight = 520;
constexpr float kTick = 0.033f;

float toDb (float g) { return g > 1e-6f ? 20.0f * std::log10 (g) : -120.0f; }
} // namespace

TransEditor::TransEditor (TransController* controller) : EditorBase (controller, kWidth, kHeight)
{
	suite::ui::setAccent (suite::ui::accents::transient);
}

void TransEditor::build (CFrame*)
{
	add (new HeaderBar (CRect (0, 0, kWidth, 44), SUITE_NAME, "Transient Shaper"));
	addPresetBar (CRect (400, 10, 720, 34));
	bind (add (new Toggle (CRect (912, 11, 986, 33), this, kBypass, "BYPASS")), kBypass)->setAccent (colors::warning);

	view = add (new TransientView (CRect (12, 54, 860, 354)));
	view->setTooltipText ("Grey: input, line: output\nShaping: boost above the line, cut below it");
	add (new Panel (CRect (870, 54, 988, 354), colors::panel, 8));
	inMeter = add (new LevelMeter (CRect (888, 66, 916, 342), "IN", LevelMeter::Mode::Level, 60.0));
	outMeter = add (new LevelMeter (CRect (942, 66, 970, 342), "OUT", LevelMeter::Mode::Level, 60.0));

	add (new Panel (CRect (12, 364, 988, 508), colors::panel, 8));
	// Attack and sustain lead; the times set what counts as each
	for (auto [id, x] : {std::pair<uint32_t, CCoord> {kAttack, 40}, {kSustain, 200}})
	{
		auto k = bind (add (new Knob (CRect (x, 370, x + 120, 504), this, id, this)), id);
		k->setBipolar (true);
		k->setTooltipText (id == kAttack ? "Up: punchier hits\nDown: softer, smoother hits"
		                                 : "Up: longer ring and room\nDown: tighter, drier sound");
	}
	const uint32_t knobs[] = {kAttackTime, kSustainTime, kMix, kOutput};
	for (int i = 0; i < 4; ++i)
	{
		const CCoord x = 400 + i * 145;
		auto k = bind (add (new Knob (CRect (x, 380, x + 90, 480), this, knobs[i], this)), knobs[i]);
		if (knobs[i] == kOutput)
			k->setBipolar (true);
		if (knobs[i] == kAttackTime)
			k->setTooltipText ("How long the start of a hit lasts: short for clicks, long for body");
		if (knobs[i] == kSustainTime)
			k->setTooltipText ("How much of the tail the sustain control reaches");
	}
}

void TransEditor::teardown ()
{
	view = nullptr;
	inMeter = outMeter = nullptr;
}

void TransEditor::tick ()
{
	if (!view)
		return;
	const auto m = transCtrl ()->takeMeters ();
	const float inDb = m.fresh ? toDb (m.inPeak) : -120.0f;
	const float outDb = m.fresh ? toDb (m.outPeak) : -120.0f;
	inMeter->update (inDb, kTick);
	outMeter->update (outDb, kTick);
	if (m.fresh)
		view->push (inDb, outDb, m.gainMax, m.gainMin);
}

} // namespace trans
