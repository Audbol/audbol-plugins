#include "suite/ui/Controls.h"

#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cframe.h"
#include "vstgui/lib/cgradient.h"
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

	const CCoord avail = std::min (r.getWidth (), r.getHeight () - 34.0);
	const bool large = avail >= 80.0; // hero knobs get larger type
	const CCoord nameSize = large ? 13 : 11.5;
	const CCoord valueSize = large ? 13.5 : 11.5;

	CRect labelRect (r.left - 8, r.top, r.right + 8, r.top + 15);
	CRect valueRect (r.left - 10, r.bottom - 16, r.right + 10, r.bottom);
	const std::string name = label.empty () ? bridge->paramName (getTag ()) : label;
	drawText (ctx, name, labelRect, nameSize, colors::textDim);
	drawText (ctx, bridge->formatValue (getTag (), getValueNormalized ()), valueRect, valueSize,
	          active ? accent : colors::text, 1, true);

	const CCoord radius = avail * 0.5 - 5.0;
	const CPoint c (r.left + r.getWidth () * 0.5, r.top + 17 + avail * 0.5);
	CRect arcRect (c.x - radius, c.y - radius, c.x + radius, c.y + radius);
	auto at = [&c] (double deg, CCoord dist) {
		const double rad = deg * 3.14159265358979323846 / 180.0;
		return CPoint (c.x + std::cos (rad) * dist, c.y + std::sin (rad) * dist);
	};

	const float v = getValueNormalized ();
	const double valueAngle = kArcStart + kArcSweep * v;
	// Bipolar arcs grow from the default (e.g. 0 dB), which isn't always mid-range.
	const double fromAngle = bipolar ? kArcStart + kArcSweep * getDefaultValue () : kArcStart;
	const CColor arcColor = getMouseEnabled () ? accent : colors::textFaint;

	// Scale ticks just outside the arc
	ctx->setLineStyle (CLineStyle (CLineStyle::kLineCapRound));
	ctx->setLineWidth (1.0);
	for (int i = 0; i <= 10; ++i)
	{
		const double a = kArcStart + kArcSweep * i / 10.0;
		const bool major = i == 0 || i == 5 || i == 10;
		ctx->setFrameColor (withAlpha (colors::textFaint, major ? 200 : 110));
		ctx->drawLine (at (a, radius + 3.0), at (a, radius + (major ? 6.0 : 5.0)));
	}

	// Track and value arc (with a soft glow under it)
	const CCoord arcWidth = large ? 3.0 : 2.5;
	if (auto track = owned (ctx->createGraphicsPath ()))
	{
		track->addArc (arcRect, kArcStart, kArcStart + kArcSweep, true);
		ctx->setLineWidth (arcWidth);
		ctx->setFrameColor (colors::control);
		ctx->drawGraphicsPath (track, CDrawContext::kPathStroked);
	}
	if (std::abs (valueAngle - fromAngle) > 0.5)
	{
		if (auto arc = owned (ctx->createGraphicsPath ()))
		{
			arc->addArc (arcRect, std::min (fromAngle, valueAngle), std::max (fromAngle, valueAngle), true);
			ctx->setLineWidth (arcWidth + 4.0);
			ctx->setFrameColor (withAlpha (arcColor, active ? 60 : 34));
			ctx->drawGraphicsPath (arc, CDrawContext::kPathStroked);
			ctx->setLineWidth (arcWidth);
			ctx->setFrameColor (arcColor);
			ctx->drawGraphicsPath (arc, CDrawContext::kPathStroked);
		}
	}

	// Cap: drop shadow, shaded body, top highlight
	const CCoord bodyR = radius - (large ? 9.0 : 7.0);
	CRect shadow (c.x - bodyR, c.y - bodyR + 2.0, c.x + bodyR, c.y + bodyR + 3.0);
	ctx->setFillColor (CColor (0, 0, 0, 110));
	ctx->drawEllipse (shadow, kDrawFilled);
	CRect body (c.x - bodyR, c.y - bodyR, c.x + bodyR, c.y + bodyR);
	if (auto cap = owned (ctx->createGraphicsPath ()))
	{
		cap->addEllipse (body);
		const CColor top = active ? CColor (66, 71, 82, 255) : CColor (58, 62, 72, 255);
		const CColor bottom = active ? CColor (38, 41, 48, 255) : CColor (32, 35, 41, 255);
		if (auto g = owned (CGradient::create (0.0, 1.0, top, bottom)))
			ctx->fillLinearGradient (cap, *g, CPoint (c.x, body.top), CPoint (c.x, body.bottom));
	}
	ctx->setLineWidth (1.0);
	ctx->setFrameColor (CColor (255, 255, 255, 22));
	ctx->drawEllipse (CRect (body).inset (0.5, 0.5), kDrawStroked);

	// Pointer
	ctx->setLineWidth (large ? 3.0 : 2.5);
	ctx->setFrameColor (colors::text);
	ctx->drawLine (at (valueAngle, bodyR * 0.42), at (valueAngle, bodyR * 0.86));

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
	const CCoord radius = std::min<CCoord> (r.getHeight () * 0.5, 5.0);
	if (on)
	{
		fillRoundRect (ctx, r, radius, mix (colors::control, accent, hover ? 0.32f : 0.24f));
		strokeRoundRect (ctx, r, radius, withAlpha (accent, 170));
	}
	else
	{
		fillRoundRectGradient (ctx, r, radius, hover ? colors::controlHover : CColor (41, 44, 52, 255),
		                       hover ? colors::control : CColor (33, 36, 42, 255));
		strokeRoundRect (ctx, r, radius, colors::border);
	}
	drawText (ctx, text, r, 11, on ? mix (accent, colors::text, 0.35f) : colors::textDim, 1, true);
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
	fillRoundRectGradient (ctx, r, 5, hover && enabled ? colors::controlHover : CColor (41, 44, 52, 255),
	                       hover && enabled ? colors::control : CColor (33, 36, 42, 255));
	strokeRoundRect (ctx, r, 5, colors::border);
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
	fillRoundRect (ctx, r, 5, CColor (27, 29, 34, 255));
	strokeRoundRect (ctx, r, 5, colors::border);
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
			fillRoundRectGradient (ctx, inner, 4, CColor (54, 58, 67, 255), CColor (42, 45, 53, 255));
			strokeRoundRect (ctx, inner, 4, withAlpha (colors::accent, 150));
		}
		drawText (ctx, labels[i], seg, 11.5, i == current ? mix (colors::accent, colors::text, 0.35f) : colors::textDim,
		          1, i == current);
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
	drawPanel (ctx, r, radius, fill);
	if (!title.empty ())
	{
		CRect t (r.left + 14, r.top + 8, r.right - 12, r.top + 24);
		drawText (ctx, title, t, 11, colors::textDim, 0, true);
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
	if (auto path = owned (ctx->createRoundRectGraphicsPath (r, 0.0)))
		if (auto g = owned (CGradient::create (0.0, 1.0, CColor (27, 29, 34, 255), CColor (21, 23, 27, 255))))
			ctx->fillLinearGradient (path, *g, CPoint (r.left, r.top), CPoint (r.left, r.bottom));
	ctx->setFrameColor (CColor (0, 0, 0, 160));
	ctx->setLineWidth (1);
	ctx->drawLine (CPoint (r.left, r.bottom - 0.5), CPoint (r.right, r.bottom - 0.5));

	// Logo mark: a small knob (ring with a pointer notch) in the accent colour
	const CPoint c (r.left + 23, r.getCenter ().y);
	ctx->setLineStyle (CLineStyle (CLineStyle::kLineCapRound));
	ctx->setLineWidth (2.2);
	ctx->setFrameColor (colors::accent);
	ctx->drawEllipse (CRect (c.x - 7.5, c.y - 7.5, c.x + 7.5, c.y + 7.5), kDrawStroked);
	ctx->drawLine (CPoint (c.x - 1.6, c.y - 1.6), CPoint (c.x - 4.6, c.y - 4.6));
	ctx->setLineStyle (kLineSolid);

	// Wordmark: "audbol" | product
	std::string mark = suite;
	std::transform (mark.begin (), mark.end (), mark.begin (), [] (unsigned char ch) { return std::tolower (ch); });
	ctx->setFont (font (17, true));
	const CCoord markW = ctx->getStringWidth (mark.c_str ());
	CRect sRect (r.left + 40, r.top, r.left + 40 + markW, r.bottom);
	drawText (ctx, mark, sRect, 17, colors::text, 0, true);
	ctx->setFrameColor (colors::gridStrong);
	ctx->setLineWidth (1.0);
	const CCoord divX = std::round (sRect.right + 11) + 0.5;
	ctx->drawLine (CPoint (divX, r.top + 14), CPoint (divX, r.bottom - 14));
	CRect pRect (divX + 11, r.top, r.right, r.bottom);
	drawText (ctx, product, pRect, 16, colors::accent, 0, false);
	setDirty (false);
}

} // namespace suite::ui
