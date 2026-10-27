#include "DlyEditor.h"

#include "DlyViews.h"

#include "suite/Brand.h"

#include "vstgui/lib/cframe.h"

#include <cmath>

using namespace VSTGUI;
using namespace suite::ui;

namespace dly {

namespace {
constexpr int kEditorWidth = 1000;
constexpr int kEditorHeight = 500;
constexpr float kTick = 0.033f;

float toDb (float g) { return g > 1e-6f ? 20.0f * std::log10 (g) : -120.0f; }
} // namespace

DlyEditor::DlyEditor (DlyController* controller) : EditorBase (controller, kEditorWidth, kEditorHeight) {}

void DlyEditor::build (CFrame*)
{
	add (new HeaderBar (CRect (0, 0, kEditorWidth, 44), SUITE_NAME, "Delay"));
	addPresetBar (CRect (230, 10, 540, 34));
	add (new Label (CRect (560, 12, 610, 32), "Mode", 11, colors::textDim, 2));
	bind (add (new Segmented (CRect (616, 11, 856, 33), this, kMode, this)), kMode);
	bind (add (new Toggle (CRect (912, 11, 986, 33), this, kBypass, "BYPASS")), kBypass)->setAccent (colors::warning);

	// Displays
	echoView = add (new EchoView (CRect (12, 54, 690, 330), dlyCtrl ()));
	filterView = add (new FilterView (CRect (700, 54, 900, 330), dlyCtrl ()));
	add (new Panel (CRect (910, 54, 988, 330), colors::panel, 8));
	inMeter = add (new LevelMeter (CRect (922, 66, 946, 318), "IN", LevelMeter::Mode::Level, 60.0));
	outMeter = add (new LevelMeter (CRect (954, 66, 978, 318), "OUT", LevelMeter::Mode::Level, 60.0));

	// Time: knobs when free, note menus when synced (same place)
	add (new Panel (CRect (12, 340, 300, 488), colors::panel, 8, "TIME"));
	timeL = bind (add (new Knob (CRect (24, 368, 104, 468), this, kTimeL, this)), kTimeL);
	timeR = bind (add (new Knob (CRect (112, 368, 192, 468), this, kTimeR, this)), kTimeR);
	noteL = bind (add (new ChoiceMenu (CRect (24, 384, 104, 424), this, kNoteL, this)), kNoteL);
	noteR = bind (add (new ChoiceMenu (CRect (112, 384, 192, 424), this, kNoteR, this)), kNoteR);
	noteL->setLabel ("Note L");
	noteR->setLabel ("Note R");
	bind (add (new Toggle (CRect (210, 380, 288, 404), this, kSync, "SYNC")), kSync);
	linkToggle = bind (add (new Toggle (CRect (210, 412, 288, 436), this, kLink, "LINK")), kLink);

	auto knobRow = [this] (std::initializer_list<uint32_t> ids, CCoord x0, CCoord pitch, CCoord w) {
		CCoord x = x0;
		for (auto id : ids)
		{
			bind (add (new Knob (CRect (x, 368, x + w, 468), this, id, this)), id);
			x += pitch;
		}
	};
	add (new Panel (CRect (310, 340, 760, 488), colors::panel, 8, "FEEDBACK & MODULATION"));
	knobRow ({kFeedback, kLowCut, kHighCut, kDrive, kModDepth, kModRate}, 318, 74, 70);
	add (new Panel (CRect (770, 340, 988, 488), colors::panel, 8, "OUTPUT"));
	knobRow ({kMix, kWidth, kDucking}, 782, 70, 64);
}

void DlyEditor::teardown ()
{
	echoView = nullptr;
	filterView = nullptr;
	inMeter = outMeter = nullptr;
	timeL = timeR = nullptr;
	noteL = noteR = nullptr;
	linkToggle = nullptr;
}

void DlyEditor::setEnabled (CView* v, bool enabled)
{
	v->setMouseEnabled (enabled);
	v->setAlphaValue (enabled ? 1.0f : 0.35f);
}

void DlyEditor::tick ()
{
	if (!echoView)
		return;
	auto lv = dlyCtrl ()->takeLevels ();
	inMeter->update (lv.fresh ? toDb (lv.inPeak) : -120.0f, kTick);
	outMeter->update (lv.fresh ? toDb (lv.outPeak) : -120.0f, kTick);
	echoView->invalid ();
	filterView->invalid ();

	const bool sync = ctrl ()->getPlain (kSync) > 0.5;
	const bool pingPong = static_cast<Mode> (static_cast<int> (ctrl ()->getPlain (kMode))) == Mode::PingPong;
	const bool linked = ctrl ()->getPlain (kLink) > 0.5 || pingPong;
	timeL->setVisible (!sync);
	timeR->setVisible (!sync);
	noteL->setVisible (sync);
	noteR->setVisible (sync);
	setEnabled (sync ? static_cast<CView*> (noteR) : timeR, !linked);
	setEnabled (linkToggle, !pingPong); // ping-pong always uses one time
}

} // namespace dly
