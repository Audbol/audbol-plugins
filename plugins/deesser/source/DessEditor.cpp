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

DessEditor::DessEditor (DessController* controller) : EditorBase (controller, kWidth, kHeight) {}

void DessEditor::build (CFrame*)
{
	add (new HeaderBar (CRect (0, 0, kWidth, 44), SUITE_NAME, "De-Esser"));
	addPresetBar (CRect (330, 10, 650, 34));
	bind (add (new Toggle (CRect (826, 11, 900, 33), this, kListen, "LISTEN")), kListen)->setAccent (colors::warning);
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
	const uint32_t knobs[] = {kThreshold, kRange, kFrequency, kQ, kRelease, kOutput};
	for (int i = 0; i < 6; ++i)
	{
		const CCoord x = 65 + i * 90;
		auto k = bind (add (new Knob (CRect (x, 370, x + 86, 470), this, knobs[i], this)), knobs[i]);
		if (knobs[i] == kQ)
			qKnob = k;
		if (knobs[i] == kOutput)
			k->setBipolar (true);
	}
	add (new Label (CRect (24, 506, 640, 520),
	                "Graph: drag left/right for frequency, up/down or wheel for Q, double-click to reset", 10,
	                colors::textFaint));
	add (new Label (CRect (24, 522, 640, 536),
	                "Listen solos what the detector hears, to find the harsh frequency", 10, colors::textFaint));

	// Detection and processing
	add (new Panel (CRect (660, 350, 988, 548), colors::panel, 8, "MODE"));
	add (new Label (CRect (676, 380, 812, 394), "Filter", 11, colors::textDim, 0));
	bind (add (new Segmented (CRect (676, 400, 812, 426), this, kFilter, this)), kFilter);
	add (new Label (CRect (836, 380, 972, 394), "Reduce", 11, colors::textDim, 0));
	bind (add (new Segmented (CRect (836, 400, 972, 426), this, kMode, this)), kMode);
	bind (add (new Toggle (CRect (676, 446, 812, 470), this, kStereoLink, "STEREO LINK")), kStereoLink);
	add (new Label (CRect (676, 492, 976, 506), "Band: sibilant range only.  High: everything above", 10,
	                colors::textFaint, 0));
	add (new Label (CRect (676, 510, 976, 524), "Split: turns down the band.  Wide: the whole signal", 10,
	                colors::textFaint, 0));
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
