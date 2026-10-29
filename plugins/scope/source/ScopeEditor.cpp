#include "ScopeEditor.h"

#include "ScopeView.h"

#include "suite/Brand.h"

#include "vstgui/lib/cframe.h"

#include <cstdio>

using namespace VSTGUI;
using namespace suite::ui;

namespace scope {

namespace {
constexpr int kEditorWidth = 1000;
constexpr int kEditorHeight = 560;
} // namespace

ScopeEditor::ScopeEditor (ScopeController* controller) : EditorBase (controller, kEditorWidth, kEditorHeight)
{
	suite::ui::setAccent (suite::ui::accents::scope);
}

void ScopeEditor::build (CFrame*)
{
	add (new HeaderBar (CRect (0, 0, kEditorWidth, 44), SUITE_NAME, "Scope"));
	add (new Label (CRect (250, 12, 316, 32), "Channels", 11, colors::textDim, 2));
	bind (add (new Segmented (CRect (322, 11, 582, 33), this, kChannels, this)), kChannels);
	add (new Label (CRect (594, 12, 650, 32), "Trigger", 11, colors::textDim, 2));
	bind (add (new Segmented (CRect (656, 11, 900, 33), this, kTrigger, this)), kTrigger);
	bind (add (new Toggle (CRect (912, 11, 986, 33), this, kFreeze, "FREEZE")), kFreeze);

	view = add (new ScopeView (CRect (12, 54, 988, 430), scopeCtrl ()));

	add (new Panel (CRect (12, 440, 640, 548), colors::panel, 8));
	timeKnob = bind (add (new Knob (CRect (28, 444, 108, 544), this, kTime, this)), kTime);
	levelKnob = bind (add (new Knob (CRect (128, 444, 208, 544), this, kTriggerLevel, this)), kTriggerLevel);
	levelKnob->setBipolar (true);
	bind (add (new Knob (CRect (228, 444, 308, 544), this, kGain, this)), kGain)->setBipolar (true);
	beatMenu = bind (add (new ChoiceMenu (CRect (330, 466, 430, 506), this, kBeatLength, this)), kBeatLength);
	beatMenu->setLabel ("Beat length");
	view->setTooltipText ("Drag sideways: time\nDrag up/down: gain\nDrag the yellow handle: trigger level");

	add (new Panel (CRect (650, 440, 988, 548), colors::panel, 8));
	add (new Label (CRect (666, 452, 816, 468), "Frequency", 11, colors::textDim, 0, true));
	add (new Label (CRect (830, 452, 980, 468), "Peak", 11, colors::textDim, 0, true));
	freqReadout = add (new Label (CRect (666, 474, 816, 510), "--", 26, colors::text, 0, true));
	peakReadout = add (new Label (CRect (830, 474, 980, 510), "--", 26, colors::text, 0, true));
	add (new Label (CRect (666, 514, 816, 530), "Hz", 11, colors::textDim, 0));
	add (new Label (CRect (830, 514, 980, 530), "dBFS", 11, colors::textDim, 0));
}

void ScopeEditor::teardown ()
{
	view = nullptr;
	timeKnob = levelKnob = nullptr;
	beatMenu = nullptr;
	freqReadout = peakReadout = nullptr;
}

void ScopeEditor::tick ()
{
	if (!view)
		return;
	const bool frozen = ctrl ()->getPlain (kFreeze) > 0.5;
	view->refresh (frozen);

	const auto mode = static_cast<Trigger> (static_cast<int> (ctrl ()->getPlain (kTrigger)));
	const bool sync = mode == Trigger::BeatSync;
	timeKnob->setAlphaValue (sync ? 0.35f : 1.0f);
	timeKnob->setMouseEnabled (!sync);
	beatMenu->setAlphaValue (sync ? 1.0f : 0.35f);
	beatMenu->setMouseEnabled (sync);
	levelKnob->setAlphaValue (mode == Trigger::RisingEdge ? 1.0f : 0.35f);
	levelKnob->setMouseEnabled (mode == Trigger::RisingEdge);

	if (frozen)
		return;
	char buf[32];
	const double f = scopeCtrl ()->frequency ();
	if (f > 0.0)
		std::snprintf (buf, sizeof (buf), f >= 1000.0 ? "%.0f" : "%.1f", f);
	else
		std::snprintf (buf, sizeof (buf), "--");
	freqReadout->setText (buf);
	const double peak = ctrl ()->getPlain (kPeak);
	if (peak <= -69.9)
		std::snprintf (buf, sizeof (buf), "-inf");
	else if (peak > -0.05)
		std::snprintf (buf, sizeof (buf), "0.0"); // avoid "-0.0"
	else
		std::snprintf (buf, sizeof (buf), "%.1f", peak);
	peakReadout->setText (buf);
}

} // namespace scope
