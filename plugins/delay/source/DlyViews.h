#pragma once

#include "DlyController.h"

#include "vstgui/lib/cview.h"

namespace dly {

// Timeline of the echo pattern: left echoes above the centre line, right
// echoes below, heights in dB. Follows the routing of the current mode, and
// shows the beat grid when synced.
//
//  drag left/right   delay time (note steps when synced; lower half edits
//                    the right side when L/R are unlinked)
//  drag up/down      feedback        wheel  feedback
class EchoView : public VSTGUI::CView
{
public:
	EchoView (const VSTGUI::CRect& r, DlyController* controller);

	void draw (VSTGUI::CDrawContext* ctx) override;
	VSTGUI::CMouseEventResult onMouseDown (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseMoved (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseUp (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseCancel () override;
	void onMouseWheelEvent (VSTGUI::MouseWheelEvent& event) override;

private:
	VSTGUI::CRect plotRect () const;
	double viewSeconds () const;
	void endDrag ();

	DlyController* ctrl;
	bool dragging {false};
	uint32_t timeParam {kTimeL};
	VSTGUI::CPoint lastMouse;
	double dragTime {0};     // log ms, or fractional note index when synced
	double dragFeedback {0}; // percent
	double frozenView {0};
};

// Response of the feedback-path filters for the 1st to 4th repeat, with
// draggable low-cut and high-cut handles.
class FilterView : public VSTGUI::CView
{
public:
	FilterView (const VSTGUI::CRect& r, DlyController* controller);

	void draw (VSTGUI::CDrawContext* ctx) override;
	VSTGUI::CMouseEventResult onMouseDown (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseMoved (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseUp (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseCancel () override;

private:
	VSTGUI::CRect plotRect () const;
	double freqToX (double f) const;
	double dbToY (double db) const;
	void endDrag ();

	DlyController* ctrl;
	uint32_t dragging {0};
	bool active {false};
	VSTGUI::CPoint lastMouse;
	double dragValue {0};
};

} // namespace dly
