#include "UtlViews.h"

#include "suite/ui/Theme.h"

#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgraphicspath.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace VSTGUI;
using namespace suite::ui;

namespace utl {

// --- Goniometer -------------------------------------------------------------

Goniometer::Goniometer (const CRect& r) : CView (r) { setMouseEnabled (false); }

void Goniometer::update (const std::vector<float>& left, const std::vector<float>& right)
{
	prevL.swap (l);
	prevR.swap (r);
	l = left;
	r = right;
	// Auto-scale: follow the peak quickly up, slowly down, never below -40 dBFS.
	double peak = 0.01;
	for (size_t i = 0; i < l.size (); ++i)
		peak = std::max ({peak, double (std::abs (l[i])), double (std::abs (r[i]))});
	scale = peak > scale ? peak : std::max (peak, scale * 0.97);
	invalid ();
}

void Goniometer::draw (CDrawContext* ctx)
{
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	const CRect v = getViewSize ();
	drawPanel (ctx, v);
	drawText (ctx, "Goniometer", CRect (v.left + 12, v.top + 8, v.left + 140, v.top + 22), 11, colors::textDim, 0, true);

	CRect plot = v;
	plot.inset (24, 30);
	plot.top -= 4;
	const CPoint c = plot.getCenter ();
	const double radius = std::min (plot.getWidth (), plot.getHeight ()) * 0.5;

	// Diamond outline with L / R diagonals and M / S axes
	ctx->setLineWidth (1.0);
	ctx->setFrameColor (colors::grid);
	if (auto path = owned (ctx->createGraphicsPath ()))
	{
		path->beginSubpath (c.x, c.y - radius);
		path->addLine (c.x + radius, c.y);
		path->addLine (c.x, c.y + radius);
		path->addLine (c.x - radius, c.y);
		path->closeSubpath ();
		ctx->drawGraphicsPath (path, CDrawContext::kPathStroked);
	}
	ctx->setFrameColor (colors::gridStrong);
	ctx->drawLine (CPoint (c.x, c.y - radius), CPoint (c.x, c.y + radius));
	ctx->drawLine (CPoint (c.x - radius, c.y), CPoint (c.x + radius, c.y));
	const double d = radius * 0.7071;
	ctx->setFrameColor (colors::grid);
	ctx->drawLine (CPoint (c.x - d, c.y - d), CPoint (c.x + d, c.y + d));
	ctx->drawLine (CPoint (c.x + d, c.y - d), CPoint (c.x - d, c.y + d));
	drawText (ctx, "M", CRect (c.x - 10, c.y - radius - 16, c.x + 10, c.y - radius - 2), 10, colors::textDim, 1, true);
	drawText (ctx, "+S", CRect (c.x - radius - 26, c.y - 7, c.x - radius - 4, c.y + 7), 10, colors::textDim, 1, true);
	drawText (ctx, "-S", CRect (c.x + radius + 4, c.y - 7, c.x + radius + 26, c.y + 7), 10, colors::textDim, 1, true);
	drawText (ctx, "L", CRect (c.x - d - 16, c.y - d - 16, c.x - d, c.y - d), 10, colors::textDim, 1, true);
	drawText (ctx, "R", CRect (c.x + d, c.y - d - 16, c.x + d + 16, c.y - d), 10, colors::textDim, 1, true);

	// Samples: up = mid, left = left-heavy side. Point size 1.6 px.
	const double k = radius / (scale * 1.4142);
	auto plotSet = [&] (const std::vector<float>& a, const std::vector<float>& b, CColor color) {
		ctx->setFillColor (color);
		for (size_t i = 0; i < a.size (); ++i)
		{
			const double m = (a[i] + b[i]) * k;
			const double s = (a[i] - b[i]) * k;
			const double x = std::clamp (c.x - s, plot.left, plot.right);
			const double y = std::clamp (c.y - m, plot.top, plot.bottom);
			ctx->drawRect (CRect (x - 0.8, y - 0.8, x + 0.8, y + 0.8), kDrawFilled);
		}
	};
	plotSet (prevL, prevR, withAlpha (colors::accent, 50));
	plotSet (l, r, withAlpha (colors::accent, 150));

	char buf[32];
	std::snprintf (buf, sizeof (buf), "scale %.0f dBFS", 20.0 * std::log10 (scale));
	drawText (ctx, buf, CRect (v.right - 140, v.bottom - 20, v.right - 12, v.bottom - 6), 10, colors::textFaint, 2);
	setDirty (false);
}

// --- BipolarMeter -----------------------------------------------------------

BipolarMeter::BipolarMeter (const CRect& r, std::string t, std::string ll, std::string rl, bool colour)
: CView (r), title (std::move (t)), leftLabel (std::move (ll)), rightLabel (std::move (rl)), colourByValue (colour)
{
	setMouseEnabled (false);
}

void BipolarMeter::setValue (double v)
{
	v = std::clamp (v, -1.0, 1.0);
	if (std::abs (v - value) > 1e-4)
	{
		value = v;
		invalid ();
	}
}

void BipolarMeter::setReadout (std::string text, std::string st)
{
	if (text != readout || st != status)
	{
		readout = std::move (text);
		status = std::move (st);
		invalid ();
	}
}

void BipolarMeter::draw (CDrawContext* ctx)
{
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	const CRect v = getViewSize ();
	drawPanel (ctx, v);
	drawText (ctx, title, CRect (v.left + 12, v.top + 8, v.left + 200, v.top + 22), 11, colors::textDim, 0, true);
	drawText (ctx, readout, CRect (v.right - 120, v.top + 4, v.right - 12, v.top + 26), 16, colors::text, 2, true);

	CRect bar (v.left + 30, v.top + 40, v.right - 30, v.top + 56);
	fillRoundRect (ctx, bar, 4, colors::control);
	const double cx = bar.getCenter ().x;
	const double x = cx + value * bar.getWidth () * 0.5;
	CColor color = colors::accent;
	if (colourByValue)
		color = value < 0.0 ? colors::danger : value < 0.3 ? colors::warning : CColor {105, 219, 124, 255};
	CRect fill (std::min (cx, x), bar.top + 2, std::max (cx, x), bar.bottom - 2);
	if (fill.getWidth () > 0.5)
		fillRoundRect (ctx, fill, 2, color);
	ctx->setFrameColor (colors::text);
	ctx->setLineWidth (2.0);
	ctx->drawLine (CPoint (x, bar.top - 3), CPoint (x, bar.bottom + 3));
	ctx->setFrameColor (colors::gridStrong);
	ctx->setLineWidth (1.0);
	ctx->drawLine (CPoint (cx, bar.top - 4), CPoint (cx, bar.bottom + 4));

	drawText (ctx, leftLabel, CRect (bar.left - 24, bar.bottom + 4, bar.left + 40, bar.bottom + 18), 10, colors::textFaint, 0);
	drawText (ctx, "0", CRect (cx - 10, bar.bottom + 4, cx + 10, bar.bottom + 18), 10, colors::textFaint);
	drawText (ctx, rightLabel, CRect (bar.right - 40, bar.bottom + 4, bar.right + 24, bar.bottom + 18), 10, colors::textFaint, 2);
	if (!status.empty ())
		drawText (ctx, status, CRect (v.left + 12, v.bottom - 22, v.right - 12, v.bottom - 6), 11, color, 0);
	setDirty (false);
}

} // namespace utl
