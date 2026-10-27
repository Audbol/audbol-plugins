#pragma once

#include "suite/ui/Theme.h"

#include "vstgui/lib/cview.h"

#include <functional>
#include <string>
#include <vector>

namespace loud {

// One labelled numeric readout (title, big value, unit, a status line).
class ReadoutTile : public VSTGUI::CView
{
public:
	ReadoutTile (const VSTGUI::CRect& r, std::string title, std::string unit, VSTGUI::CCoord valueSize);

	void set (const std::string& value, VSTGUI::CColor color, const std::string& status = {});
	void draw (VSTGUI::CDrawContext* ctx) override;
	bool hitTest (const VSTGUI::CPoint&, const VSTGUI::Event&) override { return false; }

private:
	std::string title, unit, value {"--"}, status;
	VSTGUI::CColor color {suite::ui::colors::text};
	VSTGUI::CCoord valueSize;
};

// Scrolling 60 s history of momentary and short-term loudness with the
// target band and the integrated value.
class LoudnessHistory : public VSTGUI::CView
{
public:
	explicit LoudnessHistory (const VSTGUI::CRect& r);

	void set (const std::vector<float>& momentary, const std::vector<float>& shortTerm, double integrated,
	          double target);
	void draw (VSTGUI::CDrawContext* ctx) override;
	bool hitTest (const VSTGUI::CPoint&, const VSTGUI::Event&) override { return false; }

private:
	std::vector<float> m, s;
	double integrated {-70}, target {-14};
};

// Momentary push button.
class PushButton : public VSTGUI::CView
{
public:
	PushButton (const VSTGUI::CRect& r, std::string text, std::function<void ()> onClick);

	void draw (VSTGUI::CDrawContext* ctx) override;
	VSTGUI::CMouseEventResult onMouseDown (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseUp (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;

private:
	std::string text;
	std::function<void ()> click;
	bool pressed {false};
};

} // namespace loud
