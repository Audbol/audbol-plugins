#include "ClipEditor.h"

#include "ClipCurveView.h"

#include "suite/Brand.h"
#include "suite/dsp/Smoother.h"

#include "vstgui/lib/cframe.h"

#include <algorithm>
#include <cstdio>

using namespace VSTGUI;
using namespace suite::ui;

namespace clip {

namespace {
constexpr int kWidth = 1000;
constexpr int kHeight = 560;
constexpr float kTick = 0.033f;
constexpr float kHoldSeconds = 1.5f;
} // namespace

ClipEditor::ClipEditor (ClipController* controller) : EditorBase (controller, kWidth, kHeight)
{
	suite::ui::setAccent (suite::ui::accents::clipper);
}

void ClipEditor::build (CFrame*)
{
	add (new HeaderBar (CRect (0, 0, kWidth, 44), SUITE_NAME, "Clipper"));
	addPresetBar (CRect (330, 10, 650, 34));
	auto delta = bind (add (new Toggle (CRect (826, 11, 900, 33), this, kDelta, "DELTA")), kDelta);
	delta->setAccent (colors::warning);
	delta->setTooltipText ("Hear only what the clipper removes");
	bind (add (new Toggle (CRect (912, 11, 986, 33), this, kBypass, "BYPASS")), kBypass)->setAccent (colors::warning);

	// Displays
	displays.history = add (new DynamicsHistory (CRect (12, 54, 560, 340), 48.0));
	displays.history->setThresholdLabel ("Ceiling");
	curve = add (new ClipCurveView (CRect (570, 54, 840, 340), clipCtrl ()));
	add (new Panel (CRect (850, 54, 988, 340), colors::panel, 8));
	displays.in = add (new LevelMeter (CRect (864, 66, 890, 328), "IN", LevelMeter::Mode::Level, 60.0));
	displays.gr = add (new LevelMeter (CRect (906, 66, 932, 328), "CLIP", LevelMeter::Mode::Reduction, 12.0));
	displays.out = add (new LevelMeter (CRect (948, 66, 974, 328), "OUT", LevelMeter::Mode::Level, 60.0));
	curve->setTooltipText ("Drag up/down: input gain\nDrag left/right: softness\nWheel: ceiling\n"
	                       "4x oversampled; double-click: reset");

	// Main controls: input and ceiling lead
	add (new Panel (CRect (12, 350, 650, 548), colors::panel, 8));
	for (auto [id, x] : {std::pair<uint32_t, CCoord> {kInput, 26}, {kCeiling, 140}})
		bind (add (new Knob (CRect (x, 366, x + 110, 512), this, id, this)), id);
	bind (add (new Knob (CRect (300, 366, 396, 470), this, kSoftness, this)), kSoftness)
	    ->setTooltipText ("0 %: hard clipping, loudest\n100 %: rounds peaks off gently, fewer harsh harmonics");
	outputKnob = bind (add (new Knob (CRect (440, 366, 536, 470), this, kOutput, this)), kOutput);
	outputKnob->setBipolar (true);
	bind (add (new Toggle (CRect (458, 482, 518, 504), this, kMatch, "MATCH")), kMatch)
	    ->setTooltipText ("Turn the output down by the input gain, to compare at equal level");

	// Readouts
	add (new Panel (CRect (660, 350, 988, 548), colors::panel, 8, "Last 1.5 seconds"));
	add (new Label (CRect (676, 392, 816, 408), "Most clipping", 11, colors::textDim, 0, true));
	add (new Label (CRect (830, 392, 972, 408), "Peak out", 11, colors::textDim, 0, true));
	clipReadout = add (new Label (CRect (676, 414, 816, 450), "0.0", 26, colors::text, 0, true));
	peakReadout = add (new Label (CRect (830, 414, 972, 450), "-inf", 26, colors::text, 0, true));
	add (new Label (CRect (676, 454, 816, 470), "dB", 11, colors::textDim, 0));
	add (new Label (CRect (830, 454, 972, 470), "dBFS", 11, colors::textDim, 0));
}

void ClipEditor::teardown ()
{
	displays.clear ();
	curve = nullptr;
	outputKnob = nullptr;
	clipReadout = peakReadout = nullptr;
}

void ClipEditor::tick ()
{
	if (!curve)
		return;

	auto m = clipCtrl ()->takeMeters ();
	displays.update (m.inPeak, m.outPeak, m.grMax, m.fresh, kTick);
	displays.history->setThreshold (ctrl ()->getPlain (kCeiling));

	// The dot sits on the curve's input axis, i.e. before the input gain
	const float raw = m.fresh ? m.inPeak / static_cast<float> (suite::dsp::dbToGain (ctrl ()->getPlain (kInput))) : 0.0f;
	dotPeak = raw >= dotPeak ? raw : std::max (raw, dotPeak * 0.9f);
	curve->setInputPeak (dotPeak);
	curve->invalid ();

	// Held readouts
	holdAge += kTick;
	if (holdAge > kHoldSeconds)
	{
		heldClip = heldPeak = 0.0f;
		holdAge = 0.0f;
	}
	if (m.fresh)
	{
		heldClip = std::max (heldClip, m.grMax);
		heldPeak = std::max (heldPeak, m.outPeak);
	}
	char buf[32];
	std::snprintf (buf, sizeof (buf), "%.1f", heldClip);
	clipReadout->setText (buf);
	const double peakDb = suite::dsp::gainToDb (heldPeak, -200.0);
	if (peakDb < -90.0)
		std::snprintf (buf, sizeof (buf), "-inf");
	else
		std::snprintf (buf, sizeof (buf), "%.1f", peakDb > -0.05 && peakDb < 0.0 ? 0.0 : peakDb);
	peakReadout->setText (buf);
}

} // namespace clip
