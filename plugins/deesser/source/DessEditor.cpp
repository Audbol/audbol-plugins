#include "DessEditor.h"

#include "BandView.h"

#include "suite/Brand.h"
#include "suite/dsp/Smoother.h"

#include "vstgui/lib/cframe.h"

using namespace VSTGUI;
using namespace suite::ui;

namespace dess {

namespace {
constexpr int kWidth = 1000;
constexpr int kHeight = 560;
constexpr float kTick = 0.033f;
constexpr float kFloorDb = -120.0f;
} // namespace

DessEditor::DessEditor (DessController* controller) : EditorBase (controller, kWidth, kHeight)
{
	suite::ui::setAccent (suite::ui::accents::deesser);
}

void DessEditor::build (CFrame*)
{
	add (new HeaderBar (CRect (0, 0, kWidth, 44), SUITE_NAME, "De-Esser"));
	addPresetBar (CRect (330, 10, 650, 34));
	auto listen = bind (add (new Toggle (CRect (826, 11, 900, 33), this, kListen, "LISTEN")), kListen);
	listen->setAccent (colors::warning);
	listen->setTooltipText ("Solo what the detector hears, to find the harsh frequency");
	bind (add (new Toggle (CRect (912, 11, 986, 33), this, kBypass, "BYPASS")), kBypass)->setAccent (colors::warning);

	// Displays
	history = add (new DynamicsHistory (CRect (12, 54, 500, 340)));
	bandView = add (new BandView (CRect (510, 54, 840, 340), dessCtrl ()));
	add (new Panel (CRect (850, 54, 988, 340), colors::panel, 8));
	meters.in = add (new LevelMeter (CRect (864, 66, 890, 328), "IN", LevelMeter::Mode::Level, 60.0));
	meters.gr = add (new LevelMeter (CRect (906, 66, 932, 328), "GR", LevelMeter::Mode::Reduction, 24.0));
	meters.out = add (new LevelMeter (CRect (948, 66, 974, 328), "OUT", LevelMeter::Mode::Level, 60.0));

	// Main controls
	add (new Panel (CRect (12, 350, 650, 548), colors::panel, 8));
	bandView->setTooltipText ("Drag left/right: frequency\nDrag up/down or wheel: Q\nDouble-click: reset");

	// Main controls: threshold and frequency lead
	add (new Panel (CRect (12, 350, 650, 548), colors::panel, 8));
	for (auto [id, x] : {std::pair<uint32_t, CCoord> {kThreshold, 26}, {kFrequency, 140}})
		bind (add (new Knob (CRect (x, 366, x + 110, 512), this, id, this)), id);
	const uint32_t knobs[] = {kRange, kQ, kRelease, kOutput};
	for (int i = 0; i < 4; ++i)
	{
		const CCoord x = 280 + i * 90;
		auto k = bind (add (new Knob (CRect (x, 366, x + 86, 470), this, knobs[i], this)), knobs[i]);
		if (knobs[i] == kQ)
			qKnob = k;
		if (knobs[i] == kOutput)
			k->setBipolar (true);
		if (knobs[i] == kRange)
			k->setTooltipText ("The most reduction allowed");
	}

	// Detection and processing
	add (new Panel (CRect (660, 350, 988, 548), colors::panel, 8, "Mode"));
	add (new Label (CRect (676, 384, 812, 398), "Filter", 11, colors::textDim, 0));
	bind (add (new Segmented (CRect (676, 404, 812, 430), this, kFilter, this)), kFilter)
	    ->setTooltipText ("Band: listen to the sibilant range only\nHigh: everything above the frequency");
	add (new Label (CRect (836, 384, 972, 398), "Reduce", 11, colors::textDim, 0));
	bind (add (new Segmented (CRect (836, 404, 972, 430), this, kMode, this)), kMode)
	    ->setTooltipText ("Split: turn down only that band\nWide: turn down the whole signal");
	bind (add (new Toggle (CRect (676, 456, 812, 480), this, kStereoLink, "STEREO LINK")), kStereoLink);
}

void DessEditor::teardown ()
{
	meters.clear ();
	history = nullptr;
	bandView = nullptr;
	qKnob = nullptr;
}

void DessEditor::tick ()
{
	if (!bandView)
		return;

	const auto m = dessCtrl ()->takeMeters ();
	meters.update (m.dyn.inPeak, m.dyn.outPeak, m.dyn.grMax, m.dyn.fresh, kTick);
	if (m.dyn.fresh)
	{
		// History: sidechain level before (line) and after reduction
		const float scDb = static_cast<float> (suite::dsp::gainToDb (m.scPeak, kFloorDb));
		history->push (scDb, scDb - m.dyn.grMax, m.dyn.grMax);
	}
	history->setThreshold (ctrl ()->getPlain (kThreshold));
	bandView->update (m.dyn.fresh ? m.dyn.grMax : 0.0f, kTick);

	const bool band = ctrl ()->getPlain (kFilter) < 0.5;
	qKnob->setAlphaValue (band ? 1.0f : 0.4f);
}

} // namespace dess
