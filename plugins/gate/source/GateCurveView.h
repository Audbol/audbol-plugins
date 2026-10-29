#pragma once

#include "GateController.h"

#include "vstgui/lib/cview.h"

namespace gate {

// Static input/output curve of the gate or expander with the hysteresis zone
// and a live key-level marker.
//
//  drag horizontally  threshold     drag vertically  range
//  Shift-drag         fine          wheel            hysteresis (Gate) / ratio (Expand)
//  double-click       reset threshold and range
class GateCurveView : public VSTGUI::CView
{
public:
	GateCurveView (const VSTGUI::CRect& r, GateController* controller);

	// Current key level (dBFS) for the moving dot.
	void setKeyLevel (float db);

	void draw (VSTGUI::CDrawContext* ctx) override;
	VSTGUI::CMouseEventResult onMouseDown (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseMoved (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseUp (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseCancel () override;
	void onMouseWheelEvent (VSTGUI::MouseWheelEvent& event) override;

private:
	VSTGUI::CRect plotRect () const;
	VSTGUI::CPoint toPoint (const VSTGUI::CRect& plot, double inDb, double outDb) const;
	void endDrag ();

	GateController* ctrl;
	float keyDb {-150.0f};
	bool dragging {false};
	VSTGUI::CPoint lastMouse;
	double dragThreshold {0}, dragRange {0};
};

} // namespace gate
