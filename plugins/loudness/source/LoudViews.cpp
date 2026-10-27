#include "LoudViews.h"

#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgraphicspath.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace VSTGUI;
using namespace suite::ui;

namespace loud {

// --- ReadoutTile ------------------------------------------------------------

ReadoutTile::ReadoutTile (const CRect& r, std::string t, std::string u, CCoord size)
: CView (r), title (std::move (t)), unit (std::move (u)), valueSize (size)
{
	setMouseEnabled (false);
}

void ReadoutTile::set (const std::string& v, CColor c, const std::string& st)
{
	if (v != value || c != color || st != status)
	{
		value = v;
		color = c;
		status = st;
		invalid ();
	}
}

void ReadoutTile::draw (CDrawContext* ctx)
{
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	const CRect r = getViewSize ();
	fillRoundRect (ctx, r, 8, colors::panel);
	drawText (ctx, title, CRect (r.left + 14, r.top + 10, r.right - 14, r.top + 24), 10, colors::textFaint, 0, true);
	const CCoord mid = r.top + 28 + valueSize * 0.7;
	drawText (ctx, value, CRect (r.left + 14, mid - valueSize * 0.7, r.right - 14, mid + valueSize * 0.7), valueSize,
	          color, 0, true);
	ctx->setFont (font (valueSize, true));
	const CCoord w = ctx->getStringWidth (value.c_str ());
	drawText (ctx, unit, CRect (r.left + 20 + w, mid - 2, r.right - 10, mid + valueSize * 0.55), 12,
	          colors::textDim, 0);
	if (!status.empty ())
		drawText (ctx, status, CRect (r.left + 14, r.bottom - 26, r.right - 14, r.bottom - 10), 11, colors::textDim, 0);
	setDirty (false);
}

// --- LoudnessHistory --------------------------------------------------------

namespace {
constexpr double kTop = 0.0;
constexpr double kBottom = -48.0;
}

LoudnessHistory::LoudnessHistory (const CRect& r) : CView (r) { setMouseEnabled (false); }

void LoudnessHistory::set (const std::vector<float>& momentary, const std::vector<float>& shortTerm, double i, double t)
{
	m = momentary;
	s = shortTerm;
	integrated = i;
	target = t;
	invalid ();
}

void LoudnessHistory::draw (CDrawContext* ctx)
{
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	const CRect r = getViewSize ();
	fillRoundRect (ctx, r, 8, colors::panel);
	drawText (ctx, "HISTORY  (last 60 s)", CRect (r.left + 12, r.top + 8, r.left + 240, r.top + 22), 10,
	          colors::textFaint, 0, true);
	drawText (ctx, "momentary", CRect (r.right - 220, r.top + 8, r.right - 130, r.top + 22), 10, colors::textDim, 2);
	drawText (ctx, "short-term", CRect (r.right - 120, r.top + 8, r.right - 12, r.top + 22), 10, colors::accent, 2);

	CRect plot = r;
	plot.left += 44;
	plot.right -= 12;
	plot.top += 30;
	plot.bottom -= 22;
	auto toY = [&] (double lufs) {
		return plot.top + plot.getHeight () * std::clamp ((kTop - lufs) / (kTop - kBottom), 0.0, 1.0);
	};

	ctx->setLineWidth (1.0);
	char buf[16];
	for (double l = kTop; l >= kBottom; l -= 6.0)
	{
		const double y = std::round (toY (l)) + 0.5;
		ctx->setFrameColor (colors::grid);
		ctx->drawLine (CPoint (plot.left, y), CPoint (plot.right, y));
		std::snprintf (buf, sizeof (buf), "%g", l);
		drawText (ctx, buf, CRect (plot.left - 44, y - 7, plot.left - 6, y + 7), 10, colors::textFaint, 2);
	}
	for (int sec = 0; sec <= 60; sec += 10)
	{
		const double x = std::round (plot.right - plot.getWidth () * sec / 60.0) + 0.5;
		ctx->setFrameColor (colors::grid);
		ctx->drawLine (CPoint (x, plot.top), CPoint (x, plot.bottom));
		std::snprintf (buf, sizeof (buf), sec == 0 ? "now" : "-%d s", sec);
		drawText (ctx, buf, CRect (x - 24, plot.bottom + 4, x + 24, plot.bottom + 18), 10, colors::textFaint);
	}

	ConcatClip clip (*ctx, plot);

	// Target band (+-1 LU) and line
	const CColor green {105, 219, 124, 255};
	ctx->setFillColor (withAlpha (green, 28));
	ctx->drawRect (CRect (plot.left, toY (target + 1.0), plot.right, toY (target - 1.0)), kDrawFilled);
	ctx->setFrameColor (withAlpha (green, 160));
	ctx->drawLine (CPoint (plot.left, toY (target)), CPoint (plot.right, toY (target)));

	auto curve = [&] (const std::vector<float>& v, CColor color, CCoord width) {
		if (v.size () < 2)
			return;
		auto path = owned (ctx->createGraphicsPath ());
		if (!path)
			return;
		const size_t n = v.size ();
		bool open = false;
		for (size_t i = 0; i < n; ++i)
		{
			const double x = plot.left + plot.getWidth () * i / (n - 1);
			if (v[i] <= -69.9f)
			{
				open = false; // no data / silence: leave a gap
				continue;
			}
			if (!open)
				path->beginSubpath (x, toY (v[i]));
			else
				path->addLine (x, toY (v[i]));
			open = true;
		}
		ctx->setFrameColor (color);
		ctx->setLineWidth (width);
		ctx->drawGraphicsPath (path, CDrawContext::kPathStroked);
	};
	curve (m, withAlpha (colors::textDim, 170), 1.0);
	curve (s, colors::accent, 2.2);

	if (integrated > -69.9)
	{
		static const CCoord dashes[] = {6.0, 4.0};
		ctx->setLineStyle (CLineStyle (CLineStyle::kLineCapButt, CLineStyle::kLineJoinMiter, 0.0, 2, dashes));
		ctx->setFrameColor (colors::text);
		ctx->setLineWidth (1.5);
		ctx->drawLine (CPoint (plot.left, toY (integrated)), CPoint (plot.right, toY (integrated)));
		ctx->setLineStyle (kLineSolid);
		drawText (ctx, "INTEGRATED", CRect (plot.left + 6, toY (integrated) - 16, plot.left + 120, toY (integrated) - 2), 9,
		          colors::text, 0, true);
	}
	setDirty (false);
}

// --- PushButton -------------------------------------------------------------

PushButton::PushButton (const CRect& r, std::string t, std::function<void ()> onClick)
: CView (r), text (std::move (t)), click (std::move (onClick))
{
}

void PushButton::draw (CDrawContext* ctx)
{
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	const CRect r = getViewSize ();
	fillRoundRect (ctx, r, 6, pressed ? colors::accent : colors::control);
	drawText (ctx, text, r, 11, pressed ? colors::background : colors::text, 1, true);
	setDirty (false);
}

CMouseEventResult PushButton::onMouseDown (CPoint&, const CButtonState& buttons)
{
	if (!buttons.isLeftButton ())
		return kMouseEventNotHandled;
	pressed = true;
	invalid ();
	return kMouseEventHandled;
}

CMouseEventResult PushButton::onMouseUp (CPoint& where, const CButtonState&)
{
	if (pressed && getViewSize ().pointInside (where) && click)
		click ();
	pressed = false;
	invalid ();
	return kMouseEventHandled;
}

} // namespace loud
