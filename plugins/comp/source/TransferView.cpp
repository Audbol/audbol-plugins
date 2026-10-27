#include "TransferView.h"

#include "suite/ui/Theme.h"

#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgraphicspath.h"
#include "vstgui/lib/events.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace VSTGUI;
using namespace suite::ui;

namespace comp {

namespace {
constexpr double kRangeDb = 60.0; // both axes span -60..0 dBFS
}

TransferView::TransferView (const CRect& r, CompController* c) : CView (r), ctrl (c) {}

void TransferView::setInputLevel (float db)
{
	if (std::abs (db - inputDb) > 0.05f)
	{
		inputDb = db;
		invalid ();
	}
}

CRect TransferView::plotRect () const
{
	CRect p = getViewSize ();
	p.left += 34;
	p.top += 30;
	p.right -= 12;
	p.bottom -= 24;
	return p;
}

CPoint TransferView::toPoint (const CRect& plot, double inDb, double outDb) const
{
	const double fx = std::clamp ((inDb + kRangeDb) / kRangeDb, 0.0, 1.0);
	const double fy = std::clamp ((outDb + kRangeDb) / kRangeDb, 0.0, 1.0);
	return CPoint (plot.left + plot.getWidth () * fx, plot.bottom - plot.getHeight () * fy);
}

void TransferView::draw (CDrawContext* ctx)
{
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	fillRoundRect (ctx, getViewSize (), 8, colors::panel);
	const CRect plot = plotRect ();

	const double thr = ctrl->getPlain (kThreshold);
	const double ratio = ctrl->getPlain (kRatio);
	const double knee = ctrl->getPlain (kKnee);

	// Readout
	char buf[64];
	if (ratio >= kMaxRatio - 0.5)
		std::snprintf (buf, sizeof (buf), "%.1f dB   inf : 1", thr);
	else
		std::snprintf (buf, sizeof (buf), "%.1f dB   %.1f : 1", thr, ratio);
	const CRect head = getViewSize ();
	drawText (ctx, "TRANSFER", CRect (head.left + 12, head.top + 8, head.left + 120, head.top + 22), 10,
	          colors::textFaint, 0, true);
	drawText (ctx, buf, CRect (head.right - 160, head.top + 8, head.right - 12, head.top + 22), 11,
	          colors::text, 2);

	// Grid
	ctx->setLineWidth (1.0);
	for (double db = 0.0; db >= -kRangeDb; db -= 12.0)
	{
		const CPoint p = toPoint (plot, db, db);
		const double x = std::round (p.x) + 0.5, y = std::round (p.y) + 0.5;
		ctx->setFrameColor (db == 0.0 ? colors::gridStrong : colors::grid);
		ctx->drawLine (CPoint (x, plot.top), CPoint (x, plot.bottom));
		ctx->drawLine (CPoint (plot.left, y), CPoint (plot.right, y));
		std::snprintf (buf, sizeof (buf), "%g", db);
		drawText (ctx, buf, CRect (plot.left - 32, y - 7, plot.left - 5, y + 7), 10, colors::textFaint, 2);
		drawText (ctx, buf, CRect (x - 16, plot.bottom + 4, x + 16, plot.bottom + 18), 10, colors::textFaint);
	}

	ConcatClip clip (*ctx, plot);

	// Knee zone
	if (knee > 0.0)
	{
		const double x0 = toPoint (plot, thr - knee * 0.5, 0).x;
		const double x1 = toPoint (plot, thr + knee * 0.5, 0).x;
		ctx->setFillColor (withAlpha (colors::accent, 14));
		ctx->drawRect (CRect (x0, plot.top, x1, plot.bottom), kDrawFilled);
	}

	// Unity line (dashed)
	const CCoord dashes[] = {4.0, 4.0};
	ctx->setLineStyle (CLineStyle (CLineStyle::kLineCapButt, CLineStyle::kLineJoinMiter, 0.0, 2, dashes));
	ctx->setFrameColor (colors::gridStrong);
	ctx->drawLine (toPoint (plot, -kRangeDb, -kRangeDb), toPoint (plot, 0, 0));
	// Threshold marker
	const double tx = toPoint (plot, thr, 0).x;
	ctx->setFrameColor (withAlpha (colors::text, 90));
	ctx->drawLine (CPoint (tx, plot.top), CPoint (tx, plot.bottom));
	ctx->setLineStyle (kLineSolid);

	// Curve and the gain-reduction wedge between it and unity
	const int steps = static_cast<int> (plot.getWidth ());
	auto curve = owned (ctx->createGraphicsPath ());
	auto wedge = owned (ctx->createGraphicsPath ());
	if (curve && wedge)
	{
		for (int i = 0; i <= steps; ++i)
		{
			const double in = -kRangeDb + kRangeDb * i / steps;
			const CPoint p = toPoint (plot, in, compressDb (in, thr, ratio, knee));
			if (i == 0)
			{
				curve->beginSubpath (p);
				wedge->beginSubpath (p);
			}
			else
			{
				curve->addLine (p);
				wedge->addLine (p);
			}
		}
		wedge->addLine (toPoint (plot, 0, 0));
		wedge->closeSubpath ();
		ctx->setFillColor (withAlpha (colors::accent, 30));
		ctx->drawGraphicsPath (wedge, CDrawContext::kPathFilled);
		ctx->setFrameColor (colors::accent);
		ctx->setLineWidth (2.5);
		ctx->setLineStyle (CLineStyle (CLineStyle::kLineCapRound, CLineStyle::kLineJoinRound));
		ctx->drawGraphicsPath (curve, CDrawContext::kPathStroked);
	}

	// Threshold handle
	{
		const CPoint p = toPoint (plot, thr, compressDb (thr, thr, ratio, knee));
		CRect h (p.x - 5, p.y - 5, p.x + 5, p.y + 5);
		ctx->setFillColor (dragging ? colors::text : colors::panelRaised);
		ctx->drawEllipse (h, kDrawFilled);
		ctx->setFrameColor (colors::text);
		ctx->setLineWidth (1.5);
		ctx->drawEllipse (h, kDrawStroked);
	}

	// Live signal dot
	if (inputDb > -kRangeDb)
	{
		const CPoint p = toPoint (plot, inputDb, compressDb (inputDb, thr, ratio, knee));
		CRect glow (p.x - 9, p.y - 9, p.x + 9, p.y + 9);
		ctx->setFillColor (withAlpha (colors::warning, 60));
		ctx->drawEllipse (glow, kDrawFilled);
		CRect dot (p.x - 4.5, p.y - 4.5, p.x + 4.5, p.y + 4.5);
		ctx->setFillColor (colors::warning);
		ctx->drawEllipse (dot, kDrawFilled);
	}
	setDirty (false);
}

void TransferView::perform (uint32_t id, double plain)
{
	if (auto s = ctrl->spec (id))
	{
		const double n = s->toNormalized (plain);
		ctrl->setParamNormalized (id, n);
		ctrl->performEdit (id, n);
	}
}

CMouseEventResult TransferView::onMouseDown (CPoint& where, const CButtonState& buttons)
{
	if (!buttons.isLeftButton ())
		return kMouseEventNotHandled;
	if (buttons.isDoubleClick ())
	{
		ctrl->editNormalized (kThreshold, ctrl->spec (kThreshold)->defaultNormalized ());
		ctrl->editNormalized (kRatio, ctrl->spec (kRatio)->defaultNormalized ());
		invalid ();
		return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
	}
	dragging = true;
	lastMouse = where;
	dragThreshold = ctrl->getPlain (kThreshold);
	dragLogRatio = std::log (ctrl->getPlain (kRatio));
	ctrl->beginEdit (kThreshold);
	ctrl->beginEdit (kRatio);
	invalid ();
	return kMouseEventHandled;
}

CMouseEventResult TransferView::onMouseMoved (CPoint& where, const CButtonState& buttons)
{
	if (!dragging)
		return kMouseEventNotHandled;
	const double fine = buttons.isShiftSet () ? 0.15 : 1.0;
	const double dx = (where.x - lastMouse.x) * fine;
	const double dy = (where.y - lastMouse.y) * fine;
	lastMouse = where;

	const CRect plot = plotRect ();
	dragThreshold = std::clamp (dragThreshold + dx * kRangeDb / plot.getWidth (), -60.0, 0.0);
	dragLogRatio = std::clamp (dragLogRatio + dy * 0.015, 0.0, std::log (kMaxRatio));
	perform (kThreshold, dragThreshold);
	perform (kRatio, std::exp (dragLogRatio));
	invalid ();
	return kMouseEventHandled;
}

void TransferView::endDrag ()
{
	if (!dragging)
		return;
	dragging = false;
	ctrl->endEdit (kThreshold);
	ctrl->endEdit (kRatio);
	invalid ();
}

CMouseEventResult TransferView::onMouseUp (CPoint&, const CButtonState&)
{
	endDrag ();
	return kMouseEventHandled;
}

CMouseEventResult TransferView::onMouseCancel ()
{
	endDrag ();
	return kMouseEventHandled;
}

void TransferView::onMouseWheelEvent (MouseWheelEvent& event)
{
	if (event.deltaY == 0.0)
		return;
	const double step = event.modifiers.has (ModifierKey::Shift) ? 0.25 : 1.0;
	const double knee = ctrl->getPlain (kKnee) + (event.deltaY > 0 ? step : -step);
	ctrl->editPlain (kKnee, std::clamp (knee, 0.0, 24.0));
	invalid ();
	event.consumed = true;
}

} // namespace comp
