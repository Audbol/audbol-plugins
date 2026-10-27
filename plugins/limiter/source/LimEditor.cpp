#include "LimEditor.h"

#include "suite/Brand.h"

#include "vstgui/lib/cframe.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace VSTGUI;
using namespace suite::ui;

namespace lim {

namespace {
constexpr int kWidth = 1000;
constexpr int kHeight = 520;
constexpr float kTick = 0.033f;

// Large numeric readout that resets its maximum when clicked.
class ResetLabel : public Label
{
public:
	ResetLabel (const CRect& r, std::function<void ()> onClick)
	: Label (r, "", 26, colors::text, 1, true), click (std::move (onClick))
	{
		setMouseEnabled (true);
	}
	CMouseEventResult onMouseDown (CPoint&, const CButtonState& b) override
	{
		if (!b.isLeftButton ())
			return kMouseEventNotHandled;
		click ();
		return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
	}

private:
	std::function<void ()> click;
};
} // namespace

LimEditor::LimEditor (LimController* controller) : EditorBase (controller, kWidth, kHeight) {}

void LimEditor::build (CFrame*)
{
	add (new HeaderBar (CRect (0, 0, kWidth, 44), SUITE_NAME, "Limiter"));
	bind (add (new Toggle (CRect (912, 11, 986, 33), this, kBypass, "BYPASS")), kBypass)->setAccent (colors::warning);

	displays.history = add (new DynamicsHistory (CRect (12, 54, 760, 350), 36.0));
	displays.history->setThresholdLabel ("CEILING");
	add (new Panel (CRect (770, 54, 988, 350), colors::panel, 8));
	displays.in = add (new LevelMeter (CRect (790, 66, 822, 338), "IN", LevelMeter::Mode::Level, 48.0));
	displays.gr = add (new LevelMeter (CRect (863, 66, 895, 338), "GR", LevelMeter::Mode::Reduction, 24.0));
	displays.out = add (new LevelMeter (CRect (936, 66, 968, 338), "OUT", LevelMeter::Mode::Level, 48.0));

	// Controls
	add (new Panel (CRect (12, 360, 760, 508), colors::panel, 8));
	const uint32_t knobs[] = {kInputGain, kCeiling, kRelease, kStereoLink};
	for (int i = 0; i < 4; ++i)
	{
		const CCoord x = 24 + i * 100;
		bind (add (new Knob (CRect (x, 372, x + 90, 472), this, knobs[i], this)), knobs[i]);
	}
	bind (add (new Toggle (CRect (239, 478, 299, 500), this, kAutoRelease, "AUTO")), kAutoRelease);
	bind (add (new Toggle (CRect (446, 396, 566, 420), this, kTruePeak, "TRUE PEAK")), kTruePeak);
	char info[96];
	std::snprintf (info, sizeof (info), "%.0f ms lookahead, inter-sample peak detection", kLookaheadMs);
	add (new Label (CRect (446, 428, 750, 444), info, 10, colors::textFaint, 0));
	add (new Label (CRect (446, 448, 750, 464), "Gain drives into the ceiling; output never exceeds it", 10,
	                colors::textFaint, 0));

	// Readouts
	add (new Panel (CRect (770, 360, 988, 508), colors::panel, 8, "MAX  (click to reset)"));
	add (new Label (CRect (782, 384, 876, 400), "Reduction", 11, colors::textDim));
	add (new Label (CRect (882, 384, 976, 400), "Output peak", 11, colors::textDim));
	grReadout = add (new ResetLabel (CRect (782, 404, 876, 444), [this] { maxGr = 0.0f; }));
	peakReadout = add (new ResetLabel (CRect (882, 404, 976, 444), [this] { maxOutDb = -120.0f; }));
	add (new Label (CRect (782, 448, 876, 462), "dB", 10, colors::textFaint));
	add (new Label (CRect (882, 448, 976, 462), "dBFS", 10, colors::textFaint));
}

void LimEditor::teardown ()
{
	displays.clear ();
	grReadout = peakReadout = nullptr;
}

void LimEditor::tick ()
{
	if (!grReadout)
		return;
	auto m = limCtrl ()->takeMeters ();
	displays.update (m.inPeak, m.outPeak, m.grMax, m.fresh, kTick);
	displays.history->setThreshold (ctrl ()->getPlain (kCeiling));

	if (m.fresh)
	{
		maxGr = std::max (maxGr, m.grMax);
		if (m.outPeak > 0.0f)
			maxOutDb = std::max (maxOutDb, 20.0f * std::log10 (m.outPeak));
	}
	char buf[32];
	std::snprintf (buf, sizeof (buf), maxGr > 0.05f ? "-%.1f" : "0.0", maxGr);
	grReadout->setText (buf);
	if (maxOutDb <= -119.0f)
		std::snprintf (buf, sizeof (buf), "-inf");
	else
		std::snprintf (buf, sizeof (buf), "%.2f", maxOutDb);
	peakReadout->setText (buf);
}

} // namespace lim
