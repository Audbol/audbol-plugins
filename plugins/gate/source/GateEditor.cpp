#include "GateEditor.h"

#include "GateCurveView.h"

#include "suite/Brand.h"
#include "suite/dsp/Smoother.h"

#include "vstgui/lib/cframe.h"

#include <algorithm>

using namespace VSTGUI;
using namespace suite::ui;

namespace gate {

namespace {
constexpr int kWidth = 1000;
constexpr int kHeight = 560;
constexpr float kTick = 0.033f;
constexpr float kFloorDb = -120.0f;
} // namespace

GateEditor::GateEditor (GateController* controller) : EditorBase (controller, kWidth, kHeight)
{
	suite::ui::setAccent (suite::ui::accents::gate);
}

void GateEditor::build (CFrame*)
{
	add (new HeaderBar (CRect (0, 0, kWidth, 44), SUITE_NAME, "Gate"));
	addPresetBar (CRect (330, 10, 650, 34));
	auto listen = bind (add (new Toggle (CRect (826, 11, 900, 33), this, kListen, "LISTEN")), kListen);
	listen->setAccent (colors::warning);
	listen->setTooltipText ("Hear the key filter, to tune it to the part that should open the gate");
	bind (add (new Toggle (CRect (912, 11, 986, 33), this, kBypass, "BYPASS")), kBypass)->setAccent (colors::warning);

	// Displays
	history = add (new DynamicsHistory (CRect (12, 54, 560, 340), 80.0));
	curve = add (new GateCurveView (CRect (570, 54, 840, 340), gateCtrl ()));
	add (new Panel (CRect (850, 54, 988, 340), colors::panel, 8));
	meters.in = add (new LevelMeter (CRect (864, 66, 890, 328), "IN", LevelMeter::Mode::Level, 60.0));
	meters.gr = add (new LevelMeter (CRect (906, 66, 932, 328), "GR", LevelMeter::Mode::Reduction, 80.0));
	meters.out = add (new LevelMeter (CRect (948, 66, 974, 328), "OUT", LevelMeter::Mode::Level, 60.0));
	curve->setTooltipText ("Drag left/right: threshold\nDrag up/down: range\n"
	                       "Wheel: hysteresis (Gate) or ratio (Expand)\nDouble-click: reset");

	// Main controls: threshold and range lead, the rest set timing
	add (new Panel (CRect (12, 350, 650, 548), colors::panel, 8));
	for (auto [id, x] : {std::pair<uint32_t, CCoord> {kThreshold, 26}, {kRange, 140}})
		bind (add (new Knob (CRect (x, 366, x + 110, 512), this, id, this)), id);
	const uint32_t knobs[] = {kHysteresis, kRatio, kAttack, kHold, kRelease};
	for (int i = 0; i < 5; ++i)
	{
		const CCoord x = 266 + i * 76;
		auto k = bind (add (new Knob (CRect (x, 366, x + 76, 470), this, knobs[i], this)), knobs[i]);
		if (knobs[i] == kHysteresis)
		{
			hystKnob = k;
			k->setTooltipText ("The gate closes this far below the threshold, so it doesn't chatter");
		}
		if (knobs[i] == kRatio)
		{
			ratioKnob = k;
			k->setTooltipText ("Expand mode: how quickly quiet parts get quieter");
		}
		if (knobs[i] == kHold)
			k->setTooltipText ("Keeps the gate open this long after the signal drops");
	}

	// Key filter and mode
	add (new Panel (CRect (660, 350, 988, 548), colors::panel, 8, "Key filter"));
	bind (add (new Knob (CRect (676, 380, 762, 480), this, kKeyHpf, this)), kKeyHpf)
	    ->setTooltipText ("Ignore low frequencies when deciding to open (e.g. kick bleed on a snare)");
	bind (add (new Knob (CRect (778, 380, 864, 480), this, kKeyLpf, this)), kKeyLpf)
	    ->setTooltipText ("Ignore high frequencies when deciding to open (e.g. cymbals on a tom)");
	add (new Label (CRect (880, 380, 976, 394), "Mode", 11, colors::textDim));
	bind (add (new Segmented (CRect (880, 402, 976, 426), this, kMode, this)), kMode)
	    ->setTooltipText ("Gate: closes fully, down to the range\nExpand: turns quiet parts down gradually");
}

void GateEditor::teardown ()
{
	meters.clear ();
	history = nullptr;
	curve = nullptr;
	ratioKnob = hystKnob = nullptr;
}

void GateEditor::tick ()
{
	if (!curve)
		return;

	const auto m = gateCtrl ()->takeMeters ();
	meters.update (m.dyn.inPeak, m.dyn.outPeak, m.dyn.grMax, m.dyn.fresh, kTick);
	const float keyDb = m.dyn.fresh ? static_cast<float> (suite::dsp::gainToDb (m.keyPeak, kFloorDb)) : kFloorDb;
	if (m.dyn.fresh)
		history->push (keyDb, keyDb - m.dyn.grMax, m.dyn.grMax);
	history->setThreshold (ctrl ()->getPlain (kThreshold));

	// The dot follows the key with meter-like ballistics so it is readable.
	dotDb = keyDb >= dotDb ? keyDb : std::max (keyDb, dotDb - 30.0f * kTick);
	curve->setKeyLevel (dotDb);
	curve->invalid (); // follows knob/automation changes

	const bool gateMode = static_cast<Mode> (static_cast<int> (ctrl ()->getPlain (kMode))) == Mode::Gate;
	hystKnob->setAlphaValue (gateMode ? 1.0f : 0.4f);
	ratioKnob->setAlphaValue (gateMode ? 0.4f : 1.0f);
}

} // namespace gate
