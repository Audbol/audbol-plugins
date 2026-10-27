#include "suite/ui/Controls.h"

#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cframe.h"
#include "vstgui/lib/cgraphicspath.h"
#include "vstgui/lib/controls/coptionmenu.h"
#include "vstgui/lib/events.h"

#include <algorithm>
#include <cmath>

using namespace VSTGUI;

namespace suite::ui {

namespace {

constexpr double kArcStart = 135.0; // degrees, 0 = 3 o'clock, clockwise
constexpr double kArcSweep = 270.0;

void commit (CControl* c, float v)
{
	c->beginEdit ();
	c->setValueNormalized (v);
	c->valueChanged ();
	c->endEdit ();
	c->invalid ();
}

void drawChevron (CDrawContext* ctx, CPoint center, CColor color)
{
	if (auto path = owned (ctx->createGraphicsPath ()))
	{
		path->beginSubpath (center.x - 3.5, center.y - 1.5);
		path->addLine (center.x, center.y + 2.0);
		path->addLine (center.x + 3.5, center.y - 1.5);
		ctx->setFrameColor (color);
		ctx->setLineWidth (1.5);
		ctx->drawGraphicsPath (path, CDrawContext::kPathStroked);
	}
}

} // namespace

// --- Knob -----------------------------------------------------------------

Knob::Knob (const CRect& r, IControlListener* l, int32_t tag, ParamBridge* b)
: CControl (r, l, tag), bridge (b)
{
	setWheelInc (0.01f);
}

float Knob::quantize (float v)
{
	auto steps = bridge->stepCount (getTag ());
	v = std::clamp (v, 0.0f, 1.0f);
	return steps > 0 ? std::round (v * steps) / steps : v;
}

void Knob::draw (CDrawContext* ctx)
{
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	const CRect r = getViewSize ();
	const bool active = hover || dragging;

	CRect labelRect (r.left, r.top, r.right, r.top + 14);
	CRect valueRect (r.left - 8, r.bottom - 15, r.right + 8, r.bottom);
	const std::string name = label.empty () ? bridge->paramName (getTag ()) : label;
	drawText (ctx, name, labelRect, 11, colors::textDim);
	drawText (ctx, bridge->formatValue (getTag (), getValueNormalized ()), valueRect, 11,
	          active ? colors::text : colors::textDim);

	const CCoord avail = std::min (r.getWidth (), r.getHeight () - 32.0);
	const CCoord radius = avail * 0.5 - 3.0;
	const CPoint c (r.left + r.getWidth () * 0.5, r.top + 16 + avail * 0.5);
	CRect arcRect (c.x - radius, c.y - radius, c.x + radius, c.y + radius);

	const float v = getValueNormalized ();
	const double valueAngle = kArcStart + kArcSweep * v;
	// Bipolar arcs grow from the default (e.g. 0 dB), which isn't always mid-range.
	const double fromAngle = bipolar ? kArcStart + kArcSweep * getDefaultValue () : kArcStart;

	ctx->setLineStyle (CLineStyle (CLineStyle::kLineCapRound));
	ctx->setLineWidth (3.5);

	if (auto track = owned (ctx->createGraphicsPath ()))
	{
		track->addArc (arcRect, kArcStart, kArcStart + kArcSweep, true);
		ctx->setFrameColor (colors::control);
		ctx->drawGraphicsPath (track, CDrawContext::kPathStroked);
	}
	if (std::abs (valueAngle - fromAngle) > 0.5)
	{
		if (auto arc = owned (ctx->createGraphicsPath ()))
		{
			arc->addArc (arcRect, std::min (fromAngle, valueAngle), std::max (fromAngle, valueAngle), true);
			ctx->setFrameColor (getMouseEnabled () ? accent : colors::textFaint);
			ctx->drawGraphicsPath (arc, CDrawContext::kPathStroked);
		}
	}

	// Body
	const CCoord bodyR = radius - 6.0;
	CRect body (c.x - bodyR, c.y - bodyR, c.x + bodyR, c.y + bodyR);
	ctx->setFillColor (active ? colors::controlHover : colors::panelRaised);
	ctx->drawEllipse (body, kDrawFilled);
	ctx->setFrameColor (colors::border);
	ctx->setLineWidth (1.0);
	ctx->drawEllipse (body, kDrawStroked);

	// Pointer
	const double rad = valueAngle * 3.14159265358979323846 / 180.0;
	CPoint p0 (c.x + std::cos (rad) * bodyR * 0.3, c.y + std::sin (rad) * bodyR * 0.3);
	CPoint p1 (c.x + std::cos (rad) * bodyR * 0.85, c.y + std::sin (rad) * bodyR * 0.85);
	ctx->setLineWidth (2.0);
	ctx->setFrameColor (colors::text);
	ctx->drawLine (p0, p1);

	setDirty (false);
}

CMouseEventResult Knob::onMouseDown (CPoint& where, const CButtonState& buttons)
{
	if (buttons.isRightButton () || (buttons.isLeftButton () && buttons.isControlSet ()))
	{
		bridge->openTextEntry (this);
		return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
	}
	if (!buttons.isLeftButton ())
		return kMouseEventNotHandled;
	if (buttons.isDoubleClick ())
	{
		commit (this, getDefaultValue ());
		return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
	}
	beginEdit ();
	dragging = true;
	dragStartY = where.y;
	dragStartValue = getValueNormalized ();
	invalid ();
	return kMouseEventHandled;
}

CMouseEventResult Knob::onMouseMoved (CPoint& where, const CButtonState& buttons)
{
	if (!dragging)
		return kMouseEventNotHandled;
	const double pixelsForFullRange = buttons.isShiftSet () ? 1600.0 : 220.0;
	// Re-anchor on modifier changes so switching to fine mode doesn't jump.
	float v = dragStartValue + static_cast<float> ((dragStartY - where.y) / pixelsForFullRange);
	if (v < 0.0f || v > 1.0f)
	{
		v = std::clamp (v, 0.0f, 1.0f);
		dragStartValue = v;
		dragStartY = where.y;
	}
	v = quantize (v);
	if (v != getValueNormalized ())
	{
		setValueNormalized (v);
		valueChanged ();
		invalid ();
	}
	return kMouseEventHandled;
}

CMouseEventResult Knob::onMouseUp (CPoint&, const CButtonState&)
{
	if (dragging)
	{
		dragging = false;
		endEdit ();
		invalid ();
	}
	return kMouseEventHandled;
}

CMouseEventResult Knob::onMouseCancel ()
{
	if (dragging)
	{
		dragging = false;
		setValueNormalized (dragStartValue);
		valueChanged ();
		endEdit ();
		invalid ();
	}
	return kMouseEventHandled;
}

CMouseEventResult Knob::onMouseEntered (CPoint&, const CButtonState&)
{
	hover = true;
	invalid ();
	return kMouseEventHandled;
}

CMouseEventResult Knob::onMouseExited (CPoint&, const CButtonState&)
{
	hover = false;
	invalid ();
	return kMouseEventHandled;
}

void Knob::onMouseWheelEvent (MouseWheelEvent& event)
{
	if (event.deltaY == 0.0)
		return;
	auto steps = bridge->stepCount (getTag ());
	float delta = steps > 0 ? 1.0f / steps : (event.modifiers.has (ModifierKey::Shift) ? 0.002f : 0.01f);
	float v = quantize (getValueNormalized () + (event.deltaY > 0 ? delta : -delta));
	commit (this, v);
	event.consumed = true;
}

// --- Toggle ---------------------------------------------------------------

Toggle::Toggle (const CRect& r, IControlListener* l, int32_t tag, std::string t)
: CControl (r, l, tag), text (std::move (t))
{
}

void Toggle::draw (CDrawContext* ctx)
{
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	const CRect r = getViewSize ();
	const bool on = getValueNormalized () > 0.5f;
	const CCoord radius = std::min<CCoord> (r.getHeight () * 0.5, 6.0);
	if (on)
		fillRoundRect (ctx, r, radius, accent);
	else
		fillRoundRect (ctx, r, radius, hover ? colors::controlHover : colors::control);
	drawText (ctx, text, r, 11, on ? colors::background : colors::textDim, 1, true);
	setDirty (false);
}

CMouseEventResult Toggle::onMouseDown (CPoint&, const CButtonState& buttons)
{
	if (!buttons.isLeftButton ())
		return kMouseEventNotHandled;
	commit (this, getValueNormalized () > 0.5f ? 0.0f : 1.0f);
	return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
}

CMouseEventResult Toggle::onMouseEntered (CPoint&, const CButtonState&)
{
	hover = true;
	invalid ();
	return kMouseEventHandled;
}

CMouseEventResult Toggle::onMouseExited (CPoint&, const CButtonState&)
{
	hover = false;
	invalid ();
	return kMouseEventHandled;
}

// --- ChoiceMenu -----------------------------------------------------------

ChoiceMenu::ChoiceMenu (const CRect& r, IControlListener* l, int32_t tag, ParamBridge* b)
: CControl (r, l, tag), bridge (b)
{
}

void ChoiceMenu::draw (CDrawContext* ctx)
{
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	CRect r = getViewSize ();
	if (!label.empty ())
	{
		drawText (ctx, label, CRect (r.left, r.top, r.right, r.top + 14), 11, colors::textDim);
		r.top += 17;
	}
	const bool enabled = getMouseEnabled ();
	fillRoundRect (ctx, r, 5, hover && enabled ? colors::controlHover : colors::control);
	CRect textRect = r;
	textRect.inset (8, 0);
	textRect.right -= 10;
	drawText (ctx, bridge->formatValue (getTag (), getValueNormalized ()), textRect, 11,
	          enabled ? colors::text : colors::textFaint, 0);
	drawChevron (ctx, CPoint (r.right - 11, r.getCenter ().y), colors::textDim);
	setDirty (false);
}

void ChoiceMenu::select (int index)
{
	auto steps = bridge->stepCount (getTag ());
	if (steps <= 0)
		return;
	commit (this, static_cast<float> (std::clamp (index, 0, steps)) / steps);
}

CMouseEventResult ChoiceMenu::onMouseDown (CPoint&, const CButtonState& buttons)
{
	if (!buttons.isLeftButton () && !buttons.isRightButton ())
		return kMouseEventNotHandled;
	auto frame = getFrame ();
	if (!frame)
		return kMouseEventNotHandled;

	auto labels = bridge->choiceLabels (getTag ());
	auto steps = bridge->stepCount (getTag ());
	const int current = steps > 0 ? static_cast<int> (std::round (getValueNormalized () * steps)) : 0;

	auto menu = makeOwned<COptionMenu> (CRect (), nullptr, -1, nullptr, nullptr, COptionMenu::kCheckStyle);
	for (auto& l : labels)
		menu->addEntry (l.c_str ());
	menu->setCurrent (current);

	CPoint where (getViewSize ().left, getViewSize ().bottom);
	localToFrame (where);
	SharedPointer<ChoiceMenu> self (this);
	menu->popup (frame, where, [self] (COptionMenu* m) { self->select (m->getLastResult ()); });
	return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
}

CMouseEventResult ChoiceMenu::onMouseEntered (CPoint&, const CButtonState&)
{
	hover = true;
	invalid ();
	return kMouseEventHandled;
}

CMouseEventResult ChoiceMenu::onMouseExited (CPoint&, const CButtonState&)
{
	hover = false;
	invalid ();
	return kMouseEventHandled;
}

void ChoiceMenu::onMouseWheelEvent (MouseWheelEvent& event)
{
	auto steps = bridge->stepCount (getTag ());
	if (steps <= 0 || event.deltaY == 0.0)
		return;
	int current = static_cast<int> (std::round (getValueNormalized () * steps));
	select (current + (event.deltaY > 0 ? -1 : 1));
	event.consumed = true;
}

// --- Segmented ------------------------------------------------------------

Segmented::Segmented (const CRect& r, IControlListener* l, int32_t tag, ParamBridge* b)
: CControl (r, l, tag), bridge (b)
{
}

void Segmented::draw (CDrawContext* ctx)
{
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	const CRect r = getViewSize ();
	auto labels = bridge->choiceLabels (getTag ());
	if (labels.empty ())
		return;
	fillRoundRect (ctx, r, 5, colors::control);
	const int n = static_cast<int> (labels.size ());
	const int current = static_cast<int> (std::round (getValueNormalized () * (n - 1)));
	const CCoord w = r.getWidth () / n;
	for (int i = 0; i < n; ++i)
	{
		CRect seg (r.left + i * w, r.top, r.left + (i + 1) * w, r.bottom);
		if (i == current)
		{
			CRect inner = seg;
			inner.inset (2, 2);
			fillRoundRect (ctx, inner, 4, colors::accent);
		}
		drawText (ctx, labels[i], seg, 11, i == current ? colors::background : colors::textDim, 1,
		          i == current);
	}
	setDirty (false);
}

CMouseEventResult Segmented::onMouseDown (CPoint& where, const CButtonState& buttons)
{
	if (!buttons.isLeftButton ())
		return kMouseEventNotHandled;
	auto steps = bridge->stepCount (getTag ());
	if (steps <= 0)
		return kMouseEventNotHandled;
	const CRect r = getViewSize ();
	int index = static_cast<int> ((where.x - r.left) / (r.getWidth () / (steps + 1)));
	commit (this, static_cast<float> (std::clamp (index, 0, steps)) / steps);
	return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
}

// --- Label / Panel / Header -------------------------------------------------

Label::Label (const CRect& r, std::string t, CCoord s, CColor c, int a, bool b)
: CView (r), text (std::move (t)), size (s), color (c), align (a), bold (b)
{
	setMouseEnabled (false);
}

void Label::setText (std::string t)
{
	if (t != text)
	{
		text = std::move (t);
		invalid ();
	}
}

void Label::draw (CDrawContext* ctx)
{
	drawText (ctx, text, getViewSize (), size, color, align, bold);
	setDirty (false);
}

Panel::Panel (const CRect& r, CColor f, CCoord rad, std::string t)
: CView (r), fill (f), radius (rad), title (std::move (t))
{
	setMouseEnabled (false);
}

void Panel::draw (CDrawContext* ctx)
{
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	const CRect r = getViewSize ();
	fillRoundRect (ctx, r, radius, fill);
	if (!title.empty ())
	{
		CRect t (r.left + 12, r.top + 6, r.right - 12, r.top + 22);
		drawText (ctx, title, t, 10, colors::textFaint, 0, true);
	}
	setDirty (false);
}

HeaderBar::HeaderBar (const CRect& r, std::string s, std::string p)
: CView (r), suite (std::move (s)), product (std::move (p))
{
	setMouseEnabled (false);
}

void HeaderBar::draw (CDrawContext* ctx)
{
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	const CRect r = getViewSize ();
	ctx->setFillColor (colors::panel);
	ctx->drawRect (r, kDrawFilled);
	ctx->setFrameColor (colors::border);
	ctx->setLineWidth (1);
	ctx->drawLine (CPoint (r.left, r.bottom - 0.5), CPoint (r.right, r.bottom - 0.5));

	// Logo mark: a small accent diamond
	const CPoint c (r.left + 22, r.getCenter ().y);
	if (auto path = owned (ctx->createGraphicsPath ()))
	{
		path->beginSubpath (c.x, c.y - 7);
		path->addLine (c.x + 7, c.y);
		path->addLine (c.x, c.y + 7);
		path->addLine (c.x - 7, c.y);
		path->closeSubpath ();
		ctx->setFillColor (colors::accent);
		ctx->drawGraphicsPath (path, CDrawContext::kPathFilled);
	}

	ctx->setFont (font (15, true));
	const CCoord suiteW = ctx->getStringWidth (suite.c_str ());
	CRect sRect (r.left + 38, r.top, r.left + 38 + suiteW, r.bottom);
	drawText (ctx, suite, sRect, 15, colors::text, 0, true);
	CRect pRect (sRect.right + 6, r.top, r.right, r.bottom);
	drawText (ctx, product, pRect, 15, colors::accent, 0, false);
	setDirty (false);
}

} // namespace suite::ui
