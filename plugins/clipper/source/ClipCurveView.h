#pragma once

#include "ClipController.h"

#include "vstgui/lib/cview.h"

namespace clip {

// Clipping curve from input (before the input gain) to output, with the
// ceiling, the knee and a dot at the current input peak.
//
//  drag up/down     input gain     drag left/right   softness
//  Shift-drag       fine           wheel             ceiling
//  double-click     reset input and softness
class ClipCurveView : public VSTGUI::CView
{
public:
	ClipCurveView (const VSTGUI::CRect& r, ClipController* controller);

	// Current input peak, linear, before the input gain.
	void setInputPeak (float linear);

	void draw (VSTGUI::CDrawContext* ctx) override;
	VSTGUI::CMouseEventResult onMouseDown (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseMoved (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseUp (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseCancel () override;
	void onMouseWheelEvent (VSTGUI::MouseWheelEvent& event) override;

private:
	VSTGUI::CRect plotRect () const;
	void endDrag ();

	ClipController* ctrl;
	float inputPeak {0};
	bool dragging {false};
	VSTGUI::CPoint lastMouse;
	double dragInput {0}, dragSoft {0};
};

} // namespace clip
