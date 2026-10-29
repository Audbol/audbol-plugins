#pragma once

#include "DessController.h"

#include "vstgui/lib/cview.h"

#include <vector>

namespace dess {

// Input spectrum with the detection filter and the live reduction curve.
//
//  drag horizontally  frequency     drag vertically  Q (Band filter)
//  Shift-drag         fine          wheel            Q
//  double-click       reset frequency and Q
class BandView : public VSTGUI::CView
{
public:
	BandView (const VSTGUI::CRect& r, DessController* controller);

	// Once per UI tick: current gain reduction (dB) and the spectrum update.
	void update (float grDb, float dtSeconds);

	void draw (VSTGUI::CDrawContext* ctx) override;
	void setViewSize (const VSTGUI::CRect& rect, bool invalid = true) override;
	VSTGUI::CMouseEventResult onMouseDown (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseMoved (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseUp (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseCancel () override;
	void onMouseWheelEvent (VSTGUI::MouseWheelEvent& event) override;

private:
	void layout ();
	double freqToX (double f) const;
	double gainToY (double db) const;
	double spectrumToY (double db) const;
	// Output/input response in dB at `freq` for a given reduction.
	double responseDb (double freq, double grDb) const;
	void perform (uint32_t id, double plain);
	void endDrag ();

	DessController* ctrl;
	VSTGUI::CRect plot;
	std::vector<float> columnFreqs, spectrumDb;
	float grDb {0.0f};
	bool dragging {false};
	VSTGUI::CPoint lastMouse;
	double dragLogFreq {0}, dragLogQ {0};
};

} // namespace dess
