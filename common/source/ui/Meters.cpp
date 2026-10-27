#include "suite/ui/Meters.h"

#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgraphicspath.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace VSTGUI;

namespace suite::ui {

namespace {
constexpr float kLevelFallDbPerSec = 24.0f;
constexpr float kReductionFallDbPerSec = 40.0f;
constexpr float kPeakHoldSeconds = 1.5f;
constexpr float kDisplayFloorDb = -120.0f;
constexpr CCoord kColumnWidth = 2.0;
const CColor kReductionColor {255, 140, 66, 255};
} // namespace

// --- LevelMeter -----------------------------------------------------------

LevelMeter::LevelMeter (const CRect& r, std::string l, Mode m, double rangeDb)
: CView (r), label (std::move (l)), mode (m), range (rangeDb)
{
	display = peak = mode == Mode::Level ? static_cast<float> (-range) : 0.0f;
}

double LevelMeter::toFraction (double db) const
{
	if (mode == Mode::Level)
		return std::clamp ((db + range) / range, 0.0, 1.0);
	return std::clamp (db / range, 0.0, 1.0);
}

void LevelMeter::update (float v, float dt)
{
	const bool level = mode == Mode::Level;
	const float floor = level ? static_cast<float> (-range) : 0.0f;
	v = level ? std::max (v, floor) : std::max (v, 0.0f);

	const float fall = (level ? kLevelFallDbPerSec : kReductionFallDbPerSec) * dt;
	const float prev = display;
	display = v >= display ? v : std::max (v, display - fall);

	peakAge += dt;
	if (v >= peak)
	{
		peak = v;
		peakAge = 0.0f;
	}
	else if (peakAge > kPeakHoldSeconds)
		peak = std::max (display, peak - fall);

	if (std::abs (display - prev) > 0.01f || peakAge < dt * 2)
		invalid ();
}

void LevelMeter::draw (CDrawContext* ctx)
{
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	const CRect r = getViewSize ();
	drawText (ctx, label, CRect (r.left - 6, r.top, r.right + 6, r.top + 14), 10, colors::textDim, 1, true);

	CRect bar (r.left, r.top + 18, r.right, r.bottom - 20);
	fillRoundRect (ctx, bar, 3, colors::control);

	const double h = bar.getHeight ();
	// Scale ticks every 6 dB
	ctx->setLineWidth (1.0);
	ctx->setFrameColor (withAlpha (colors::background, 160));
	for (double db = 6.0; db < range; db += 6.0)
	{
		const double f = mode == Mode::Level ? toFraction (-db) : toFraction (db);
		const double y = mode == Mode::Level ? bar.bottom - h * f : bar.top + h * f;
		ctx->drawLine (CPoint (bar.left, std::round (y) + 0.5), CPoint (bar.right, std::round (y) + 0.5));
	}

	if (mode == Mode::Level)
	{
		// Colour zones: normal / hot (> -6 dBFS) / clip (> -0.1 dBFS)
		const double f = toFraction (display);
		if (f > 0.0)
		{
			auto zone = [&] (double fromDb, double toDb, const CColor& c) {
				const double a = toFraction (fromDb), b = std::min (toFraction (toDb), f);
				if (b <= a)
					return;
				CRect seg (bar.left, bar.bottom - h * b, bar.right, bar.bottom - h * a);
				ctx->setFillColor (c);
				ctx->drawRect (seg, kDrawFilled);
			};
			zone (-range, -6.0, colors::accent);
			zone (-6.0, -0.1, colors::warning);
			zone (-0.1, 0.0, colors::danger);
		}
		const double py = bar.bottom - h * toFraction (peak);
		ctx->setFrameColor (peak > -0.1f ? colors::danger : colors::text);
		ctx->setLineWidth (2.0);
		if (peak > -range + 0.5)
			ctx->drawLine (CPoint (bar.left, py), CPoint (bar.right, py));
	}
	else
	{
		const double f = toFraction (display);
		if (f > 0.0)
		{
			CRect seg (bar.left, bar.top, bar.right, bar.top + h * f);
			ctx->setFillColor (kReductionColor);
			ctx->drawRect (seg, kDrawFilled);
		}
		const double py = bar.top + h * toFraction (peak);
		if (peak > 0.05f)
		{
			ctx->setFrameColor (colors::text);
			ctx->setLineWidth (2.0);
			ctx->drawLine (CPoint (bar.left, py), CPoint (bar.right, py));
		}
	}

	char buf[16];
	if (mode == Mode::Level)
	{
		if (peak <= -range + 0.5)
			std::snprintf (buf, sizeof (buf), "-inf");
		else if (peak > -0.05f)
			std::snprintf (buf, sizeof (buf), "0.0"); // avoid "-0.0"
		else
			std::snprintf (buf, sizeof (buf), "%.1f", peak);
	}
	else
		std::snprintf (buf, sizeof (buf), peak > 0.05f ? "-%.1f" : "0.0", peak);
	const CColor numColor = mode == Mode::Level && peak > -0.1f ? colors::danger : colors::text;
	drawText (ctx, buf, CRect (r.left - 10, r.bottom - 16, r.right + 10, r.bottom), 10, numColor);
	setDirty (false);
}

CMouseEventResult LevelMeter::onMouseDown (CPoint&, const CButtonState& buttons)
{
	if (!buttons.isLeftButton ())
		return kMouseEventNotHandled;
	peak = display;
	peakAge = 0.0f;
	invalid ();
	return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
}

// --- DynamicsHistory ------------------------------------------------------

DynamicsHistory::DynamicsHistory (const CRect& r, double rangeDb) : CView (r), range (rangeDb)
{
	setMouseEnabled (false);
	setViewSize (r, false);
}

void DynamicsHistory::setViewSize (const CRect& rect, bool inv)
{
	CView::setViewSize (rect, inv);
	const auto n = static_cast<size_t> (std::max (2.0, plotRect ().getWidth () / kColumnWidth));
	const float floor = static_cast<float> (-range);
	columns.assign (n, {floor, floor, 0.0f});
	head = 0;
}

CRect DynamicsHistory::plotRect () const
{
	CRect p = getViewSize ();
	p.inset (10, 10);
	p.left += 24; // dB labels
	return p;
}

double DynamicsHistory::levelToY (const CRect& plot, double db) const
{
	const double f = std::clamp (-db / range, 0.0, 1.0);
	return plot.top + plot.getHeight () * f;
}

void DynamicsHistory::push (float inDb, float outDb, float grDb)
{
	columns[head] = {inDb, outDb, grDb};
	head = (head + 1) % columns.size ();
	invalid ();
}

void DynamicsHistory::setThreshold (double db)
{
	if (std::abs (db - threshold) > 1e-4)
	{
		threshold = db;
		invalid ();
	}
}

void DynamicsHistory::draw (CDrawContext* ctx)
{
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	fillRoundRect (ctx, getViewSize (), 8, colors::panel);
	const CRect plot = plotRect ();

	// Grid
	ctx->setLineWidth (1.0);
	for (double db = 0.0; db >= -range; db -= 12.0)
	{
		const double y = std::round (levelToY (plot, db)) + 0.5;
		ctx->setFrameColor (db == 0.0 ? colors::gridStrong : colors::grid);
		ctx->drawLine (CPoint (plot.left, y), CPoint (plot.right, y));
		char buf[8];
		std::snprintf (buf, sizeof (buf), "%g", db);
		drawText (ctx, buf, CRect (plot.left - 30, y - 7, plot.left - 4, y + 7), 10, colors::textFaint, 2);
	}

	const size_t n = columns.size ();
	auto colX = [&] (size_t i) { return plot.left + i * kColumnWidth; };
	auto at = [&] (size_t i) -> const Column& { return columns[(head + i) % n]; };

	ConcatClip clip (*ctx, plot);

	// Input level: filled from the bottom
	if (auto area = owned (ctx->createGraphicsPath ()))
	{
		area->beginSubpath (colX (0), plot.bottom);
		for (size_t i = 0; i < n; ++i)
		{
			area->addLine (colX (i), levelToY (plot, at (i).in));
			area->addLine (colX (i) + kColumnWidth, levelToY (plot, at (i).in));
		}
		area->addLine (colX (n), plot.bottom);
		area->closeSubpath ();
		ctx->setFillColor (withAlpha (colors::textDim, 60));
		ctx->drawGraphicsPath (area, CDrawContext::kPathFilled);
	}

	// Output level: line
	if (auto line = owned (ctx->createGraphicsPath ()))
	{
		line->beginSubpath (colX (0), levelToY (plot, at (0).out));
		for (size_t i = 1; i < n; ++i)
			line->addLine (colX (i), levelToY (plot, at (i).out));
		ctx->setFrameColor (withAlpha (colors::accent, 200));
		ctx->setLineWidth (1.25);
		ctx->drawGraphicsPath (line, CDrawContext::kPathStroked);
	}

	// Gain reduction: hangs from the top, same dB scale
	if (auto gr = owned (ctx->createGraphicsPath ()))
	{
		auto edge = owned (ctx->createGraphicsPath ());
		gr->beginSubpath (colX (0), plot.top);
		for (size_t i = 0; i < n; ++i)
		{
			const double y = plot.top + plot.getHeight () * std::clamp (at (i).gr / range, 0.0, 1.0);
			gr->addLine (colX (i), y);
			if (i == 0)
				edge->beginSubpath (colX (i), y);
			else
				edge->addLine (colX (i), y);
		}
		gr->addLine (colX (n - 1), plot.top);
		gr->closeSubpath ();
		ctx->setFillColor (withAlpha (kReductionColor, 70));
		ctx->drawGraphicsPath (gr, CDrawContext::kPathFilled);
		ctx->setFrameColor (kReductionColor);
		ctx->setLineWidth (1.5);
		ctx->drawGraphicsPath (edge, CDrawContext::kPathStroked);
	}

	// Threshold marker
	const double ty = std::round (levelToY (plot, threshold)) + 0.5;
	ctx->setFrameColor (withAlpha (colors::text, 140));
	ctx->setLineWidth (1.0);
	const CCoord dashes[] = {4.0, 4.0};
	ctx->setLineStyle (CLineStyle (CLineStyle::kLineCapButt, CLineStyle::kLineJoinMiter, 0.0, 2, dashes));
	ctx->drawLine (CPoint (plot.left, ty), CPoint (plot.right, ty));
	ctx->setLineStyle (kLineSolid);
	// Label above the line, or below it when the line is near the top.
	const double labelTop = ty - 16 < plot.top ? ty + 3 : ty - 16;
	drawText (ctx, thresholdLabel, CRect (plot.right - 120, labelTop, plot.right - 4, labelTop + 14), 9,
	          colors::textDim, 2, true);

	drawText (ctx, "GAIN REDUCTION", CRect (plot.left + 6, plot.top + 4, plot.left + 200, plot.top + 18), 9,
	          kReductionColor, 0, true);
	setDirty (false);
}

// --- DynamicsDisplays -----------------------------------------------------

void DynamicsDisplays::update (float inPeak, float outPeak, float grDb, bool fresh, float dt)
{
	auto toDb = [] (float g) { return g > 1e-6f ? 20.0f * std::log10 (g) : kDisplayFloorDb; };
	const float kFloorDb = kDisplayFloorDb;
	const float inDb = fresh ? toDb (inPeak) : kFloorDb;
	const float outDb = fresh ? toDb (outPeak) : kFloorDb;
	const float gr = fresh ? grDb : 0.0f;
	if (in)
		in->update (inDb, dt);
	if (out)
		out->update (outDb, dt);
	if (this->gr)
		this->gr->update (gr, dt);
	if (history && fresh)
		history->push (inDb, outDb, gr);
}

} // namespace suite::ui
