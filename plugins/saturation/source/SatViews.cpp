#include "SatViews.h"

#include "suite/dsp/Smoother.h"
#include "suite/ui/Theme.h"

#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgraphicspath.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace VSTGUI;
using namespace suite::ui;

namespace sat {

namespace {

const CColor kEvenColor {255, 169, 77, 255};

void performEdit (SatController* ctrl, uint32_t id, double plain)
{
	if (auto s = ctrl->spec (id))
	{
		const double n = s->toNormalized (plain);
		ctrl->setParamNormalized (id, n);
		ctrl->performEdit (id, n);
	}
}

struct Settings
{
	Model model;
	double gain;
	double bias;
};

Settings currentSettings (SatController* ctrl)
{
	return {static_cast<Model> (static_cast<int> (ctrl->getPlain (kModel))),
	        suite::dsp::dbToGain (ctrl->getPlain (kDrive)), ctrl->getPlain (kBias) / 100.0};
}

} // namespace

// --- CurveView --------------------------------------------------------------

CurveView::CurveView (const CRect& r, SatController* c) : CView (r), ctrl (c) {}

void CurveView::setInputPeak (float linear)
{
	if (std::abs (linear - inputPeak) > 1e-4f)
	{
		inputPeak = linear;
		invalid ();
	}
}

CRect CurveView::plotRect () const
{
	CRect p = getViewSize ();
	p.left += 16;
	p.right -= 16;
	p.top += 30;
	p.bottom -= 16;
	// Square plot
	const CCoord side = std::min (p.getWidth (), p.getHeight ());
	const CPoint c = p.getCenter ();
	return CRect (c.x - side / 2, c.y - side / 2, c.x + side / 2, c.y + side / 2);
}

void CurveView::draw (CDrawContext* ctx)
{
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	const CRect r = getViewSize ();
	drawPanel (ctx, r);
	const CRect plot = plotRect ();
	const auto s = currentSettings (ctrl);
	drawText (ctx, "Curve", CRect (r.left + 12, r.top + 8, r.left + 100, r.top + 22), 11, colors::textDim, 0, true);
	char buf[48];
	std::snprintf (buf, sizeof (buf), "%+.1f dB   bias %+.0f%%", ctrl->getPlain (kDrive), s.bias * 100.0);
	drawText (ctx, buf, CRect (r.right - 200, r.top + 8, r.right - 12, r.top + 22), 11, colors::text, 2);

	auto toPoint = [&] (double x, double y) {
		return CPoint (plot.left + plot.getWidth () * (x + 1.0) * 0.5,
		               plot.bottom - plot.getHeight () * (std::clamp (y, -1.1, 1.1) + 1.0) * 0.5);
	};

	// Grid
	ctx->setLineWidth (1.0);
	for (double v : {-1.0, -0.5, 0.0, 0.5, 1.0})
	{
		const CPoint a = toPoint (v, -1.0), b = toPoint (v, 1.0);
		const CPoint c = toPoint (-1.0, v), d = toPoint (1.0, v);
		ctx->setFrameColor (v == 0.0 ? colors::gridStrong : colors::grid);
		ctx->drawLine (a, b);
		ctx->drawLine (c, d);
	}

	ConcatClip clip (*ctx, plot);

	// Range of the curve the input is using right now
	const double in = std::min (1.0, static_cast<double> (inputPeak));
	if (in > 0.001)
	{
		ctx->setFillColor (withAlpha (colors::accent, 22));
		ctx->drawRect (CRect (toPoint (-in, 1.0).x, plot.top, toPoint (in, 1.0).x, plot.bottom), kDrawFilled);
	}

	// Unity reference (dashed)
	static const CCoord dashes[] = {4.0, 4.0};
	ctx->setLineStyle (CLineStyle (CLineStyle::kLineCapButt, CLineStyle::kLineJoinMiter, 0.0, 2, dashes));
	ctx->setFrameColor (colors::gridStrong);
	ctx->drawLine (toPoint (-1.0, -1.0), toPoint (1.0, 1.0));
	ctx->setLineStyle (kLineSolid);

	// Curve
	if (auto path = owned (ctx->createGraphicsPath ()))
	{
		const int steps = static_cast<int> (plot.getWidth ());
		for (int i = 0; i <= steps; ++i)
		{
			const double x = -1.0 + 2.0 * i / steps;
			const CPoint p = toPoint (x, saturate (s.model, x, s.gain, s.bias));
			if (i == 0)
				path->beginSubpath (p);
			else
				path->addLine (p);
		}
		ctx->setFrameColor (colors::accent);
		ctx->setLineWidth (2.5);
		ctx->setLineStyle (CLineStyle (CLineStyle::kLineCapRound, CLineStyle::kLineJoinRound));
		ctx->drawGraphicsPath (path, CDrawContext::kPathStroked);
		ctx->setLineStyle (kLineSolid);
	}

	// Live input dot
	if (in > 0.001)
	{
		const CPoint p = toPoint (in, saturate (s.model, in, s.gain, s.bias));
		ctx->setFillColor (withAlpha (colors::warning, 60));
		ctx->drawEllipse (CRect (p.x - 9, p.y - 9, p.x + 9, p.y + 9), kDrawFilled);
		ctx->setFillColor (colors::warning);
		ctx->drawEllipse (CRect (p.x - 4.5, p.y - 4.5, p.x + 4.5, p.y + 4.5), kDrawFilled);
	}
	setDirty (false);
}

CMouseEventResult CurveView::onMouseDown (CPoint& where, const CButtonState& buttons)
{
	if (!buttons.isLeftButton ())
		return kMouseEventNotHandled;
	if (buttons.isDoubleClick ())
	{
		ctrl->editNormalized (kDrive, ctrl->spec (kDrive)->defaultNormalized ());
		ctrl->editNormalized (kBias, ctrl->spec (kBias)->defaultNormalized ());
		invalid ();
		return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
	}
	dragging = true;
	lastMouse = where;
	dragDrive = ctrl->getPlain (kDrive);
	dragBias = ctrl->getPlain (kBias);
	ctrl->beginEdit (kDrive);
	ctrl->beginEdit (kBias);
	return kMouseEventHandled;
}

CMouseEventResult CurveView::onMouseMoved (CPoint& where, const CButtonState& buttons)
{
	if (!dragging)
		return kMouseEventNotHandled;
	const double fine = buttons.isShiftSet () ? 0.15 : 1.0;
	dragDrive = std::clamp (dragDrive - (where.y - lastMouse.y) * fine * 0.15, 0.0, 36.0);
	dragBias = std::clamp (dragBias + (where.x - lastMouse.x) * fine * 0.6, -100.0, 100.0);
	lastMouse = where;
	performEdit (ctrl, kDrive, dragDrive);
	performEdit (ctrl, kBias, dragBias);
	invalid ();
	return kMouseEventHandled;
}

void CurveView::endDrag ()
{
	if (!dragging)
		return;
	dragging = false;
	ctrl->endEdit (kDrive);
	ctrl->endEdit (kBias);
}

CMouseEventResult CurveView::onMouseUp (CPoint&, const CButtonState&)
{
	endDrag ();
	return kMouseEventHandled;
}

CMouseEventResult CurveView::onMouseCancel ()
{
	endDrag ();
	return kMouseEventHandled;
}

// --- HarmonicsView ----------------------------------------------------------

namespace {
constexpr int kCycles = 16;   // fundamental lands in FFT bin 16 (no window needed)
constexpr int kNumHarmonics = 12;
constexpr double kTopDb = 6.0;
constexpr double kBottomDb = -96.0;
} // namespace

HarmonicsView::HarmonicsView (const CRect& r, SatController* c) : CView (r), ctrl (c)
{
	setMouseEnabled (false);
	buffer.resize (fft.size ());
}

void HarmonicsView::setInputLevelDb (float db)
{
	levelDb = std::clamp (db, -30.0f, 0.0f);
}

void HarmonicsView::analyse ()
{
	const auto s = currentSettings (ctrl);
	const int n = fft.size ();
	const double amp = suite::dsp::dbToGain (levelDb);
	for (int i = 0; i < n; ++i)
	{
		const double x = amp * std::sin (2.0 * 3.14159265358979323846 * kCycles * i / n);
		buffer[i] = {static_cast<float> (saturate (s.model, x, s.gain, s.bias)), 0.0f};
	}
	fft.forward (buffer.data ());
	const double fundamental = std::abs (buffer[kCycles]);
	double rest = 0.0;
	for (int h = 1; h <= kNumHarmonics; ++h)
	{
		const double mag = std::abs (buffer[h * kCycles]);
		harmonicsDb[h] = fundamental > 1e-12 ? 20.0 * std::log10 (std::max (mag, 1e-12) / fundamental) : kBottomDb;
		if (h >= 2)
			rest += mag * mag;
	}
	thdPercent = fundamental > 1e-12 ? 100.0 * std::sqrt (rest) / fundamental : 0.0;
}

void HarmonicsView::draw (CDrawContext* ctx)
{
	analyse ();
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	const CRect r = getViewSize ();
	drawPanel (ctx, r);
	drawText (ctx, "Harmonics", CRect (r.left + 12, r.top + 8, r.left + 120, r.top + 22), 11, colors::textDim, 0, true);
	char buf[96];
	std::snprintf (buf, sizeof (buf), "THD %.2f %%   for a sine at %.0f dBFS", thdPercent, levelDb);
	drawText (ctx, buf, CRect (r.right - 320, r.top + 8, r.right - 12, r.top + 22), 11, colors::text, 2);

	CRect plot = r;
	plot.left += 44;
	plot.right -= 16;
	plot.top += 34;
	plot.bottom -= 26;
	auto dbToY = [&] (double db) {
		return plot.top + plot.getHeight () * std::clamp ((kTopDb - db) / (kTopDb - kBottomDb), 0.0, 1.0);
	};

	ctx->setLineWidth (1.0);
	for (double db = 0.0; db >= kBottomDb; db -= 24.0)
	{
		const double y = std::round (dbToY (db)) + 0.5;
		ctx->setFrameColor (db == 0.0 ? colors::gridStrong : colors::grid);
		ctx->drawLine (CPoint (plot.left, y), CPoint (plot.right, y));
		std::snprintf (buf, sizeof (buf), "%g", db);
		drawText (ctx, buf, CRect (plot.left - 40, y - 7, plot.left - 6, y + 7), 10, colors::textFaint, 2);
	}

	const double slot = plot.getWidth () / kNumHarmonics;
	for (int h = 1; h <= kNumHarmonics; ++h)
	{
		const double x0 = plot.left + slot * (h - 1) + slot * 0.2;
		const double x1 = plot.left + slot * h - slot * 0.2;
		const double y = dbToY (harmonicsDb[h]);
		const CColor color = h == 1 ? colors::textDim : (h % 2 == 0 ? kEvenColor : colors::accent);
		if (y < plot.bottom - 0.5)
			fillRoundRect (ctx, CRect (x0, y, x1, plot.bottom), 2, withAlpha (color, 210));
		std::snprintf (buf, sizeof (buf), "%d", h);
		drawText (ctx, buf, CRect (x0 - 6, plot.bottom + 5, x1 + 6, plot.bottom + 19), 10,
		          h == 1 ? colors::textDim : color);
	}
	// Legend
	drawText (ctx, "odd", CRect (plot.right - 120, plot.top + 2, plot.right - 66, plot.top + 16), 10, colors::accent, 2);
	drawText (ctx, "even", CRect (plot.right - 60, plot.top + 2, plot.right - 4, plot.top + 16), 10, kEvenColor, 2);
	setDirty (false);
}

} // namespace sat
