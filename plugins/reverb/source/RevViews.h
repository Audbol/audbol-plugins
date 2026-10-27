#pragma once

#include "RevController.h"

#include "vstgui/lib/cview.h"

#include <vector>

namespace rev {

// Time-domain picture of the reverb: pre-delay gap, early-reflection taps and
// the tail's decay slopes for low, mid and high frequencies.
//
//  drag             decay time         drag the PRE marker  pre-delay
//  wheel            size               double-click         reset decay
class DecayView : public VSTGUI::CView
{
public:
	DecayView (const VSTGUI::CRect& r, RevController* controller);

	void draw (VSTGUI::CDrawContext* ctx) override;
	VSTGUI::CMouseEventResult onMouseDown (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseMoved (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseUp (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseCancel () override;
	void onMouseWheelEvent (VSTGUI::MouseWheelEvent& event) override;

private:
	enum class Drag
	{
		None,
		Decay,
		PreDelay
	};

	VSTGUI::CRect plotRect () const;
	double viewSeconds () const;
	double timeToX (const VSTGUI::CRect& plot, double sec) const;
	double dbToY (const VSTGUI::CRect& plot, double db) const;
	void perform (uint32_t id, double plain);
	void endDrag ();

	RevController* ctrl;
	Drag drag {Drag::None};
	VSTGUI::CPoint lastMouse;
	double dragValue {0};
	double frozenViewSeconds {0}; // keeps the time axis still while dragging
};

// Frequency-domain picture: decay time across frequency over the live
// spectrum of the reverb signal, with the wet low/high cut regions.
//
//  LOW handle (100 Hz)  drag vertically for bass multiplier
//  MID handle (1 kHz)   drag vertically for decay
//  DAMP handle          drag horizontally for damping frequency
//  low/high cut edges   drag horizontally
class ToneView : public VSTGUI::CView
{
public:
	ToneView (const VSTGUI::CRect& r, RevController* controller);

	void refresh (float dt);
	void draw (VSTGUI::CDrawContext* ctx) override;
	VSTGUI::CMouseEventResult onMouseDown (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseMoved (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseUp (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseCancel () override;

private:
	enum Handle
	{
		kNone = -1,
		kLowHandle,
		kMidHandle,
		kDampHandle,
		kLowCutHandle,
		kHighCutHandle,
		kNumHandles
	};

	VSTGUI::CRect plotRect () const;
	double freqToX (const VSTGUI::CRect& plot, double f) const;
	double xToFreq (const VSTGUI::CRect& plot, double x) const;
	double secToY (const VSTGUI::CRect& plot, double s) const;
	VSTGUI::CPoint handlePos (const VSTGUI::CRect& plot, int handle) const;
	int hitHandle (const VSTGUI::CPoint& p) const;
	void perform (uint32_t id, double plain);
	uint32_t handleParam (int handle) const;
	void endDrag ();

	RevController* ctrl;
	std::vector<float> columnFreqs;
	std::vector<float> spectrumDb;
	int dragging {kNone};
	int hover {kNone};
	VSTGUI::CPoint lastMouse;
	double dragValue {0};
};

} // namespace rev
