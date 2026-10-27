#pragma once

#include "CompController.h"

#include "vstgui/lib/cview.h"

namespace comp {

// Static input/output curve of the compressor with a live input marker.
//
//  drag horizontally  threshold     drag vertically  ratio
//  Shift-drag         fine          wheel            knee
//  double-click       reset threshold and ratio
class TransferView : public VSTGUI::CView
{
public:
	TransferView (const VSTGUI::CRect& r, CompController* controller);

	// Current detector level (dBFS) for the moving dot.
	void setInputLevel (float db);

	void draw (VSTGUI::CDrawContext* ctx) override;
	VSTGUI::CMouseEventResult onMouseDown (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseMoved (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseUp (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseCancel () override;
	void onMouseWheelEvent (VSTGUI::MouseWheelEvent& event) override;

private:
	VSTGUI::CRect plotRect () const;
	VSTGUI::CPoint toPoint (const VSTGUI::CRect& plot, double inDb, double outDb) const;
	void perform (uint32_t id, double plain);
	void endDrag ();

	CompController* ctrl;
	float inputDb {-120.0f};
	bool dragging {false};
	VSTGUI::CPoint lastMouse;
	double dragThreshold {0}, dragLogRatio {0};
};

} // namespace comp
