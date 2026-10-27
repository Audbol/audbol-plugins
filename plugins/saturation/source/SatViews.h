#pragma once

#include "SatController.h"

#include "suite/dsp/FFT.h"

#include "vstgui/lib/cview.h"

#include <complex>
#include <vector>

namespace sat {

// Transfer curve of the current model with drive and bias, the input range
// in use right now, and a dot at the current input peak.
//
//  drag up/down     drive          drag left/right   bias
//  Shift-drag       fine           double-click      reset both
class CurveView : public VSTGUI::CView
{
public:
	CurveView (const VSTGUI::CRect& r, SatController* controller);

	void setInputPeak (float linear);

	void draw (VSTGUI::CDrawContext* ctx) override;
	VSTGUI::CMouseEventResult onMouseDown (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseMoved (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseUp (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseCancel () override;

private:
	VSTGUI::CRect plotRect () const;
	void endDrag ();

	SatController* ctrl;
	float inputPeak {0};
	bool dragging {false};
	VSTGUI::CPoint lastMouse;
	double dragDrive {0}, dragBias {0};
};

// Harmonic spectrum the current settings produce for a sine at the live
// input level: bars for H1..H12 relative to the fundamental, plus THD.
class HarmonicsView : public VSTGUI::CView
{
public:
	HarmonicsView (const VSTGUI::CRect& r, SatController* controller);

	void setInputLevelDb (float db);
	void draw (VSTGUI::CDrawContext* ctx) override;
	bool hitTest (const VSTGUI::CPoint&, const VSTGUI::Event&) override { return false; }

private:
	void analyse ();

	SatController* ctrl;
	float levelDb {-12.0f};
	suite::dsp::FFT fft {10};
	std::vector<std::complex<float>> buffer;
	double harmonicsDb[13] {};
	double thdPercent {0};
};

} // namespace sat
