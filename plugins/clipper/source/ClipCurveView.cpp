#include "ClipCurveView.h"

#include "suite/dsp/Smoother.h"
#include "suite/ui/Theme.h"

#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgraphicspath.h"
#include "vstgui/lib/events.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace VSTGUI;
using namespace suite::ui;
using suite::dsp::dbToGain;

namespace clip {

ClipCurveView::ClipCurveView (const CRect& r, ClipController* c) : CView (r), ctrl (c) {}

void ClipCurveView::setInputPeak (float linear)
{
	if (std::abs (linear - inputPeak) > 1e-4f)
	{
		inputPeak = linear;
		invalid ();
	}
}

CRect ClipCurveView::plotRect () const
{
	CRect p = getViewSize ();
	p.left += 16;
	p.right -= 16;
	p.top += 30;
	p.bottom -= 16;
	const CCoord side = std::min (p.getWidth (), p.getHeight ());
	const CPoint c = p.getCenter ();
	return CRect (c.x - side / 2, c.y - side / 2, c.x + side / 2, c.y + side / 2);
}

void ClipCurveView::draw (CDrawContext* ctx)
{
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	const CRect r = getViewSize ();
	drawPanel (ctx, r);
	const CRect plot = plotRect ();

	const double inDb = ctrl->getPlain (kInput);
	const double g = dbToGain (inDb);
	const double ceilDb = ctrl->getPlain (kCeiling);
	const double c = dbToGain (ceilDb);
	const double k = ctrl->getPlain (kSoftness) / 100.0 * kMaxKnee;

	drawText (ctx, "Curve", CRect (r.left + 12, r.top + 8, r.left + 100, r.top + 22), 11, colors::textDim, 0, true);
	char buf[48];
	std::snprintf (buf, sizeof (buf), "%+.1f dB   ceiling %.1f", inDb, ceilDb);
	drawText (ctx, buf, CRect (r.right - 200, r.top + 8, r.right - 12, r.top + 22), 11, colors::text, 2);

	auto toPoint = [&] (double x, double y) {
		return CPoint (plot.left + plot.getWidth () * (x + 1.0) * 0.5,
		               plot.bottom - plot.getHeight () * (std::clamp (y, -1.1, 1.1) + 1.0) * 0.5);
	};

	// Grid
	ctx->setLineWidth (1.0);
	for (double v : {-1.0, -0.5, 0.0, 0.5, 1.0})
	{
		ctx->setFrameColor (v == 0.0 ? colors::gridStrong : colors::grid);
		ctx->drawLine (toPoint (v, -1.0), toPoint (v, 1.0));
		ctx->drawLine (toPoint (-1.0, v), toPoint (1.0, v));
	}

	ConcatClip plotClip (*ctx, plot);

	// Where clipping starts (input side): shade beyond the knee's start
	const double startX = std::min (1.0, c * (1.0 - k) / g);
	ctx->setFillColor (withAlpha (colors::accent, 18));
	ctx->drawRect (CRect (plot.left, plot.top, toPoint (-startX, 0).x, plot.bottom), kDrawFilled);
	ctx->drawRect (CRect (toPoint (startX, 0).x, plot.top, plot.right, plot.bottom), kDrawFilled);

	// Unity reference and ceiling lines (dashed)
	static const CCoord dashes[] = {4.0, 4.0};
	ctx->setLineStyle (CLineStyle (CLineStyle::kLineCapButt, CLineStyle::kLineJoinMiter, 0.0, 2, dashes));
	ctx->setFrameColor (colors::gridStrong);
	ctx->drawLine (toPoint (-1.0, -1.0), toPoint (1.0, 1.0));
	ctx->setFrameColor (withAlpha (colors::text, 110));
	ctx->drawLine (toPoint (-1.0, c), toPoint (1.0, c));
	ctx->drawLine (toPoint (-1.0, -c), toPoint (1.0, -c));
	ctx->setLineStyle (kLineSolid);

	// Curve
	if (auto path = owned (ctx->createGraphicsPath ()))
	{
		const int steps = static_cast<int> (plot.getWidth ());
		for (int i = 0; i <= steps; ++i)
		{
			const double x = -1.0 + 2.0 * i / steps;
			const CPoint p = toPoint (x, clipSample (x * g, c, k));
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
	const double in = std::min (1.0, static_cast<double> (inputPeak));
	if (in > 0.001)
	{
		const CPoint p = toPoint (in, clipSample (in * g, c, k));
		ctx->setFillColor (withAlpha (colors::warning, 60));
		ctx->drawEllipse (CRect (p.x - 9, p.y - 9, p.x + 9, p.y + 9), kDrawFilled);
		ctx->setFillColor (colors::warning);
		ctx->drawEllipse (CRect (p.x - 4.5, p.y - 4.5, p.x + 4.5, p.y + 4.5), kDrawFilled);
	}
	setDirty (false);
}

CMouseEventResult ClipCurveView::onMouseDown (CPoint& where, const CButtonState& buttons)
{
	if (!buttons.isLeftButton ())
		return kMouseEventNotHandled;
	if (buttons.isDoubleClick ())
	{
		ctrl->editNormalized (kInput, ctrl->spec (kInput)->defaultNormalized ());
		ctrl->editNormalized (kSoftness, ctrl->spec (kSoftness)->defaultNormalized ());
		invalid ();
		return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
	}
	dragging = true;
	lastMouse = where;
	dragInput = ctrl->getPlain (kInput);
	dragSoft = ctrl->getPlain (kSoftness);
	ctrl->beginEdit (kInput);
	ctrl->beginEdit (kSoftness);
	return kMouseEventHandled;
}

CMouseEventResult ClipCurveView::onMouseMoved (CPoint& where, const CButtonState& buttons)
{
	if (!dragging)
		return kMouseEventNotHandled;
	const double fine = buttons.isShiftSet () ? 0.15 : 1.0;
	dragInput = std::clamp (dragInput - (where.y - lastMouse.y) * fine * 0.12, -12.0, 24.0);
	dragSoft = std::clamp (dragSoft + (where.x - lastMouse.x) * fine * 0.5, 0.0, 100.0);
	lastMouse = where;
	for (auto [id, v] : {std::pair<uint32_t, double> {kInput, dragInput}, {kSoftness, dragSoft}})
	{
		const double n = ctrl->spec (id)->toNormalized (v);
		ctrl->setParamNormalized (id, n);
		ctrl->performEdit (id, n);
	}
	invalid ();
	return kMouseEventHandled;
}

void ClipCurveView::endDrag ()
{
	if (!dragging)
		return;
	dragging = false;
	ctrl->endEdit (kInput);
	ctrl->endEdit (kSoftness);
}

CMouseEventResult ClipCurveView::onMouseUp (CPoint&, const CButtonState&)
{
	endDrag ();
	return kMouseEventHandled;
}

CMouseEventResult ClipCurveView::onMouseCancel ()
{
	endDrag ();
	return kMouseEventHandled;
}

void ClipCurveView::onMouseWheelEvent (MouseWheelEvent& event)
{
	if (event.deltaY == 0.0)
		return;
	const double step = event.modifiers.has (ModifierKey::Shift) ? 0.1 : 0.5;
	const double v = ctrl->getPlain (kCeiling) + (event.deltaY > 0 ? step : -step);
	ctrl->editPlain (kCeiling, std::clamp (v, -24.0, 0.0));
	invalid ();
	event.consumed = true;
}

} // namespace clip
