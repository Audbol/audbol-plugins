#pragma once

#include "ScopeController.h"

#include "vstgui/lib/cview.h"

#include <string>
#include <vector>

namespace scope {

// Oscilloscope display.
//
//  Free         the newest `Time` of audio
//  Rising Edge  starts at the newest upward crossing of the trigger level
//               (with hysteresis) that still has a full window after it, so
//               periodic signals stand still; falls back to free run
//  Beat Sync    one sweep per beat length, following the song position; the
//               previous sweep stays visible (faded) ahead of the cursor
//
//  drag left/right  time (or beat length)    drag up/down   vertical gain
//  drag the level handle at the left edge    trigger level
//  wheel            time                     double-click   reset time and gain
class ScopeView : public VSTGUI::CView
{
public:
	ScopeView (const VSTGUI::CRect& r, ScopeController* controller);

	// Pull new samples (skipped while frozen) and redraw.
	void refresh (bool frozen);

	void draw (VSTGUI::CDrawContext* ctx) override;
	VSTGUI::CMouseEventResult onMouseDown (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseMoved (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseUp (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseCancel () override;
	void onMouseWheelEvent (VSTGUI::MouseWheelEvent& event) override;

private:
	struct Trace
	{
		std::vector<float> lo, hi; // per pixel column
		std::vector<bool> valid;
	};

	VSTGUI::CRect plotRect () const;
	double gainLinear () const;
	void buildTraces ();
	void addSample (Trace& t, double x, float v);
	float source (size_t i) const; // trigger source sample

	enum class Drag
	{
		None,
		View,
		Level
	};

	ScopeController* ctrl;
	std::vector<float> l, r, ppq;
	Trace traceA, traceB;         // L / R, or the single selected channel in A
	Trace prevA, prevB;           // previous beat cycle (Beat Sync)
	bool twoTraces {true};
	std::string status;
	double cursor {-1.0};         // Beat Sync sweep position 0..1
	double displaySeconds {0.02};

	Drag drag {Drag::None};
	VSTGUI::CPoint lastMouse;
	double dragTime {0}, dragGain {0}, dragLevel {0};
};

} // namespace scope
