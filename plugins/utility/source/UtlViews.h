#pragma once

#include "vstgui/lib/cview.h"

#include <string>
#include <vector>

namespace utl {

// Goniometer (vectorscope): L/R samples plotted with mid vertical and side
// horizontal, auto-scaled to the recent peak, with a short persistence.
class Goniometer : public VSTGUI::CView
{
public:
	explicit Goniometer (const VSTGUI::CRect& r);

	void update (const std::vector<float>& left, const std::vector<float>& right);
	void draw (VSTGUI::CDrawContext* ctx) override;
	bool hitTest (const VSTGUI::CPoint&, const VSTGUI::Event&) override { return false; }

private:
	std::vector<float> l, r;
	std::vector<float> prevL, prevR; // previous frame, drawn fainter
	double scale {1.0};
};

// Horizontal meter for a value in [-1, 1] with a centre mark, e.g.
// correlation (-1 out of phase .. +1 mono) or balance (L .. R).
class BipolarMeter : public VSTGUI::CView
{
public:
	BipolarMeter (const VSTGUI::CRect& r, std::string title, std::string leftLabel, std::string rightLabel,
	              bool colourByValue);

	void setValue (double v);
	void setReadout (std::string text, std::string status = {});
	void draw (VSTGUI::CDrawContext* ctx) override;
	bool hitTest (const VSTGUI::CPoint&, const VSTGUI::Event&) override { return false; }

private:
	std::string title, leftLabel, rightLabel, readout, status;
	bool colourByValue;
	double value {0};
};

} // namespace utl
