#include "CompEditor.h"

#include "TransferView.h"

#include "suite/Brand.h"
#include "suite/dsp/Smoother.h"

#include "vstgui/lib/cframe.h"

#include <algorithm>

using namespace VSTGUI;
using namespace suite::ui;

namespace comp {

namespace {
constexpr int kWidth = 1000;
constexpr int kHeight = 560;
constexpr float kTick = 0.033f;
constexpr float kFloorDb = -120.0f;
} // namespace

CompEditor::CompEditor (CompController* controller) : EditorBase (controller, kWidth, kHeight) {}

void CompEditor::build (CFrame*)
{
	add (new HeaderBar (CRect (0, 0, kWidth, 44), SUITE_NAME, "Compressor"));
	addPresetBar (CRect (330, 10, 650, 34));
	bind (add (new Toggle (CRect (912, 11, 986, 33), this, kBypass, "BYPASS")), kBypass)->setAccent (colors::warning);

	// Displays
	displays.history = add (new DynamicsHistory (CRect (12, 54, 560, 340)));
	transfer = add (new TransferView (CRect (570, 54, 840, 340), compCtrl ()));
	add (new Panel (CRect (850, 54, 988, 340), colors::panel, 8));
	displays.in = add (new LevelMeter (CRect (864, 66, 890, 328), "IN", LevelMeter::Mode::Level, 60.0));
	displays.gr = add (new LevelMeter (CRect (906, 66, 932, 328), "GR", LevelMeter::Mode::Reduction, 24.0));
	displays.out = add (new LevelMeter (CRect (948, 66, 974, 328), "OUT", LevelMeter::Mode::Level, 60.0));

	// Main controls
	add (new Panel (CRect (12, 350, 650, 548), colors::panel, 8));
	const uint32_t knobs[] = {kThreshold, kRatio, kKnee, kAttack, kRelease, kMakeup, kMix};
	for (int i = 0; i < 7; ++i)
	{
		const CCoord x = 20 + i * 90;
		auto k = bind (add (new Knob (CRect (x, 362, x + 86, 462), this, knobs[i], this)), knobs[i]);
		if (knobs[i] == kMakeup)
		{
			k->setBipolar (true);
			makeupKnob = k;
		}
	}
	bind (add (new Toggle (CRect (395, 472, 451, 494), this, kAutoRelease, "AUTO")), kAutoRelease);
	bind (add (new Toggle (CRect (485, 472, 541, 494), this, kAutoMakeup, "AUTO")), kAutoMakeup);
	add (new Label (CRect (24, 516, 640, 532),
	                "Transfer curve: drag left/right for threshold, up/down for ratio, wheel for knee, double-click to reset",
	                10, colors::textFaint, 0));

	// Sidechain
	add (new Panel (CRect (660, 350, 988, 548), colors::panel, 8, "SIDECHAIN"));
	scFreqKnob = bind (add (new Knob (CRect (676, 380, 762, 480), this, kScFilterFreq, this)), kScFilterFreq);
	bind (add (new Toggle (CRect (691, 488, 747, 510), this, kScFilterOn, "HPF")), kScFilterOn);
	bind (add (new Knob (CRect (778, 380, 864, 480), this, kStereoLink, this)), kStereoLink);
	add (new Label (CRect (880, 380, 976, 394), "Detection", 11, colors::textDim));
	bind (add (new Segmented (CRect (880, 402, 976, 426), this, kDetection, this)), kDetection);
}

void CompEditor::teardown ()
{
	displays.clear ();
	transfer = nullptr;
	makeupKnob = scFreqKnob = nullptr;
}

void CompEditor::tick ()
{
	if (!transfer)
		return;

	auto m = compCtrl ()->takeMeters ();
	displays.update (m.inPeak, m.outPeak, m.grMax, m.fresh, kTick);
	displays.history->setThreshold (ctrl ()->getPlain (kThreshold));
	const float inDb = m.fresh ? static_cast<float> (suite::dsp::gainToDb (m.inPeak, kFloorDb)) : kFloorDb;

	// The dot follows the input with meter-like ballistics so it is readable.
	dotDb = inDb >= dotDb ? inDb : std::max (inDb, dotDb - 30.0f * kTick);
	transfer->setInputLevel (dotDb);
	transfer->invalid (); // follows knob/automation changes

	// Disabled-looking controls when they have no effect
	const bool autoGain = ctrl ()->getPlain (kAutoMakeup) > 0.5;
	makeupKnob->setAlphaValue (autoGain ? 0.5f : 1.0f);
	const bool hpf = ctrl ()->getPlain (kScFilterOn) > 0.5;
	scFreqKnob->setAlphaValue (hpf ? 1.0f : 0.4f);
}

} // namespace comp
