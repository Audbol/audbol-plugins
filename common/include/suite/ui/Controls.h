#pragma once

#include "suite/ui/Theme.h"

#include "vstgui/lib/controls/ccontrol.h"
#include "vstgui/lib/cview.h"

#include <functional>
#include <string>
#include <vector>

namespace suite::ui {

// What widgets need to know about the parameter they are bound to.
// Implemented by EditorBase so widgets stay independent of VST3.
struct ParamBridge
{
	virtual ~ParamBridge () = default;
	virtual std::string formatValue (int32_t tag, double normalized) = 0;
	virtual std::string paramName (int32_t tag) = 0;
	virtual int32_t stepCount (int32_t tag) = 0;
	virtual std::vector<std::string> choiceLabels (int32_t tag) = 0;
	virtual void openTextEntry (VSTGUI::CControl* control) = 0;
};

// Rotary knob: drag vertically (Shift = fine), double-click = default,
// wheel to nudge, right-click or Ctrl-click to type a value.
class Knob : public VSTGUI::CControl
{
public:
	Knob (const VSTGUI::CRect& r, VSTGUI::IControlListener* listener, int32_t tag, ParamBridge* bridge);

	void setAccent (const VSTGUI::CColor& c) { accent = c; invalid (); }
	void setBipolar (bool b) { bipolar = b; }
	void setLabel (std::string l) { label = std::move (l); }

	void draw (VSTGUI::CDrawContext* ctx) override;
	VSTGUI::CMouseEventResult onMouseDown (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseMoved (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseUp (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseCancel () override;
	VSTGUI::CMouseEventResult onMouseEntered (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseExited (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	void onMouseWheelEvent (VSTGUI::MouseWheelEvent& event) override;

	CLASS_METHODS_NOCOPY (Knob, CControl)

private:
	float quantize (float v);

	ParamBridge* bridge;
	VSTGUI::CColor accent {colors::accent};
	std::string label;
	bool bipolar {false};
	bool hover {false};
	bool dragging {false};
	float dragStartValue {0};
	VSTGUI::CCoord dragStartY {0};
};

// Pill-shaped on/off button.
class Toggle : public VSTGUI::CControl
{
public:
	Toggle (const VSTGUI::CRect& r, VSTGUI::IControlListener* listener, int32_t tag, std::string text);

	void setAccent (const VSTGUI::CColor& c) { accent = c; invalid (); }
	void setText (std::string t) { text = std::move (t); invalid (); }

	void draw (VSTGUI::CDrawContext* ctx) override;
	VSTGUI::CMouseEventResult onMouseDown (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseEntered (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseExited (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;

	CLASS_METHODS_NOCOPY (Toggle, CControl)

private:
	std::string text;
	VSTGUI::CColor accent {colors::accent};
	bool hover {false};
};

// Drop-down selector for choice parameters.
class ChoiceMenu : public VSTGUI::CControl
{
public:
	ChoiceMenu (const VSTGUI::CRect& r, VSTGUI::IControlListener* listener, int32_t tag, ParamBridge* bridge);

	void setLabel (std::string l) { label = std::move (l); }

	void draw (VSTGUI::CDrawContext* ctx) override;
	VSTGUI::CMouseEventResult onMouseDown (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseEntered (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseExited (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	void onMouseWheelEvent (VSTGUI::MouseWheelEvent& event) override;

	CLASS_METHODS_NOCOPY (ChoiceMenu, CControl)

private:
	void select (int index);

	ParamBridge* bridge;
	std::string label;
	bool hover {false};
};

// Inline segmented selector for short choice lists.
class Segmented : public VSTGUI::CControl
{
public:
	Segmented (const VSTGUI::CRect& r, VSTGUI::IControlListener* listener, int32_t tag, ParamBridge* bridge);

	void draw (VSTGUI::CDrawContext* ctx) override;
	VSTGUI::CMouseEventResult onMouseDown (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;

	CLASS_METHODS_NOCOPY (Segmented, CControl)

private:
	ParamBridge* bridge;
};

// Static text.
class Label : public VSTGUI::CView
{
public:
	Label (const VSTGUI::CRect& r, std::string text, VSTGUI::CCoord size = 11,
	       VSTGUI::CColor color = colors::textDim, int align = 1, bool bold = false);

	void setText (std::string t);
	void draw (VSTGUI::CDrawContext* ctx) override;

private:
	std::string text;
	VSTGUI::CCoord size;
	VSTGUI::CColor color;
	int align;
	bool bold;
};

// Rounded background panel (purely decorative, ignores the mouse).
class Panel : public VSTGUI::CView
{
public:
	Panel (const VSTGUI::CRect& r, VSTGUI::CColor fill = colors::panel, VSTGUI::CCoord radius = 8,
	       std::string title = {});

	void draw (VSTGUI::CDrawContext* ctx) override;
	bool hitTest (const VSTGUI::CPoint&, const VSTGUI::Event&) override { return false; }

private:
	VSTGUI::CColor fill;
	VSTGUI::CCoord radius;
	std::string title;
};

// Top bar with the suite / plug-in name.
class HeaderBar : public VSTGUI::CView
{
public:
	HeaderBar (const VSTGUI::CRect& r, std::string suite, std::string product);

	void draw (VSTGUI::CDrawContext* ctx) override;
	bool hitTest (const VSTGUI::CPoint&, const VSTGUI::Event&) override { return false; }

private:
	std::string suite;
	std::string product;
};

} // namespace suite::ui
