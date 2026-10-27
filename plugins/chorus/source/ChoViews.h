#pragma once

#include "ChoController.h"

#include "vstgui/lib/cview.h"

namespace cho {

// One LFO cycle: each voice's delay against the master phase (left solid,
// right dashed), with a playhead at the processor's current phase.
//
//  drag up/down  depth     drag left/right  rate     wheel  delay
//  Shift-drag    fine      double-click     reset rate and depth
class LfoView : public VSTGUI::CView
{
public:
	LfoView (const VSTGUI::CRect& r, ChoController* controller);

	// Called on every UI tick; follows the processor's phase when audio runs.
	void advance (float dt);
	double phase () const { return playhead; }

	void draw (VSTGUI::CDrawContext* ctx) override;
	VSTGUI::CMouseEventResult onMouseDown (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseMoved (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseUp (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseCancel () override;
	void onMouseWheelEvent (VSTGUI::MouseWheelEvent& event) override;

private:
	VSTGUI::CRect plotRect () const;
	void endDrag ();

	ChoController* ctrl;
	double playhead {0};
	double lastProcessorPhase {-2.0};
	bool dragging {false};
	VSTGUI::CPoint lastMouse;
	double dragDepth {0}, dragLogRate {0};
};

// Instantaneous frequency response of dry + wet for both channels at the
// current LFO position: the comb filter that sweeps as the voices move.
class ResponseView : public VSTGUI::CView
{
public:
	ResponseView (const VSTGUI::CRect& r, ChoController* controller);

	void setPhase (double p) { phase = p; }
	void draw (VSTGUI::CDrawContext* ctx) override;
	bool hitTest (const VSTGUI::CPoint&, const VSTGUI::Event&) override { return false; }

private:
	ChoController* ctrl;
	double phase {0};
};

} // namespace cho
