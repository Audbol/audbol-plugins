#include "GateCurveView.h"

#include "suite/ui/Theme.h"

#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgraphicspath.h"
#include "vstgui/lib/events.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace VSTGUI;
using namespace suite::ui;

namespace gate {

namespace {
constexpr double kAxisDb = 80.0; // both axes span -80..0 dBFS
}

GateCurveView::GateCurveView (const CRect& r, GateController* c) : CView (r), ctrl (c) {}

void GateCurveView::setKeyLevel (float db)
{
	if (std::abs (db - keyDb) > 0.05f)
	{
		keyDb = db;
		invalid ();
	}
}

CRect GateCurveView::plotRect () const
{
	CRect p = getViewSize ();
	p.left += 34;
	p.top += 30;
	p.right -= 12;
	p.bottom -= 24;
	return p;
}

CPoint GateCurveView::toPoint (const CRect& plot, double inDb, double outDb) const
{
	const double fx = std::clamp ((inDb + kAxisDb) / kAxisDb, 0.0, 1.0);
	const double fy = std::clamp ((outDb + kAxisDb) / kAxisDb, 0.0, 1.0);
	return CPoint (plot.left + plot.getWidth () * fx, plot.bottom - plot.getHeight () * fy);
}

void GateCurveView::draw (CDrawContext* ctx)
{
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	drawPanel (ctx, getViewSize ());
	const CRect plot = plotRect ();

	const double thr = ctrl->getPlain (kThreshold);
	const double range = ctrl->getPlain (kRange);
	const double ratio = ctrl->getPlain (kRatio);
	const double hyst = ctrl->getPlain (kHysteresis);
	const auto mode = static_cast<Mode> (static_cast<int> (ctrl->getPlain (kMode)));

	// Readout
	char buf[64];
	if (mode == Mode::Gate)
		std::snprintf (buf, sizeof (buf), "%.1f dB   floor -%.0f dB", thr, range);
	else
		std::snprintf (buf, sizeof (buf), "%.1f dB   1 : %.1f", thr, ratio);
	const CRect head = getViewSize ();
	drawText (ctx, "Transfer", CRect (head.left + 12, head.top + 8, head.left + 120, head.top + 22), 11,
	          colors::textDim, 0, true);
	drawText (ctx, buf, CRect (head.right - 170, head.top + 8, head.right - 12, head.top + 22), 11,
	          colors::text, 2);

	// Grid
	ctx->setLineWidth (1.0);
	for (double db = 0.0; db >= -kAxisDb; db -= 20.0)
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

	// Hysteresis zone: between the closing and opening level the gate keeps its state
	if (mode == Mode::Gate && hyst > 0.0)
	{
		const double x0 = toPoint (plot, thr - hyst, 0).x;
		const double x1 = toPoint (plot, thr, 0).x;
		ctx->setFillColor (withAlpha (colors::accent, 16));
		ctx->drawRect (CRect (x0, plot.top, x1, plot.bottom), kDrawFilled);
	}

	// Unity line (dashed) and threshold marker
	const CCoord dashes[] = {4.0, 4.0};
	ctx->setLineStyle (CLineStyle (CLineStyle::kLineCapButt, CLineStyle::kLineJoinMiter, 0.0, 2, dashes));
	ctx->setFrameColor (colors::gridStrong);
	ctx->drawLine (toPoint (plot, -kAxisDb, -kAxisDb), toPoint (plot, 0, 0));
	const double tx = toPoint (plot, thr, 0).x;
	ctx->setFrameColor (withAlpha (colors::text, 90));
	ctx->drawLine (CPoint (tx, plot.top), CPoint (tx, plot.bottom));
	ctx->setLineStyle (kLineSolid);

	// Curve plus the attenuation wedge between it and unity
	auto curve = owned (ctx->createGraphicsPath ());
	auto wedge = owned (ctx->createGraphicsPath ());
	if (curve && wedge)
	{
		auto add = [&] (double in, double out, bool first) {
			const CPoint p = toPoint (plot, in, out);
			if (first)
			{
				curve->beginSubpath (p);
				wedge->beginSubpath (toPoint (plot, -kAxisDb, -kAxisDb));
				wedge->addLine (p);
			}
			else
			{
				curve->addLine (p);
				wedge->addLine (p);
			}
		};
		if (mode == Mode::Gate)
		{
			// Exact corners: a straight line at the floor, a step at the threshold
			add (-kAxisDb, -kAxisDb - range, true);
			add (thr, thr - range, false);
			add (thr, thr, false);
			add (0.0, 0.0, false);
		}
		else
		{
			const int steps = static_cast<int> (plot.getWidth ());
			for (int i = 0; i <= steps; ++i)
			{
				const double in = -kAxisDb + kAxisDb * i / steps;
				add (in, in - staticReductionDb (mode, in, thr, range, ratio), i == 0);
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

	// Threshold handle at the top of the step
	{
		const CPoint p = toPoint (plot, thr, thr);
		CRect h (p.x - 5, p.y - 5, p.x + 5, p.y + 5);
		ctx->setFillColor (dragging ? colors::text : colors::panelRaised);
		ctx->drawEllipse (h, kDrawFilled);
		ctx->setFrameColor (colors::text);
		ctx->setLineWidth (1.5);
		ctx->drawEllipse (h, kDrawStroked);
	}

	// Live key level
	if (keyDb > -kAxisDb)
	{
		const double outDb = keyDb - staticReductionDb (mode, keyDb, thr, range, ratio);
		const CPoint p = toPoint (plot, keyDb, outDb);
		CRect glow (p.x - 9, p.y - 9, p.x + 9, p.y + 9);
		ctx->setFillColor (withAlpha (colors::warning, 60));
		ctx->drawEllipse (glow, kDrawFilled);
		CRect dot (p.x - 4.5, p.y - 4.5, p.x + 4.5, p.y + 4.5);
		ctx->setFillColor (colors::warning);
		ctx->drawEllipse (dot, kDrawFilled);
	}
	setDirty (false);
}

CMouseEventResult GateCurveView::onMouseDown (CPoint& where, const CButtonState& buttons)
{
	if (!buttons.isLeftButton ())
		return kMouseEventNotHandled;
	if (buttons.isDoubleClick ())
	{
		ctrl->editNormalized (kThreshold, ctrl->spec (kThreshold)->defaultNormalized ());
		ctrl->editNormalized (kRange, ctrl->spec (kRange)->defaultNormalized ());
		invalid ();
		return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
	}
	dragging = true;
	lastMouse = where;
	dragThreshold = ctrl->getPlain (kThreshold);
	dragRange = ctrl->getPlain (kRange);
	ctrl->beginEdit (kThreshold);
	ctrl->beginEdit (kRange);
	invalid ();
	return kMouseEventHandled;
}

CMouseEventResult GateCurveView::onMouseMoved (CPoint& where, const CButtonState& buttons)
{
	if (!dragging)
		return kMouseEventNotHandled;
	const double fine = buttons.isShiftSet () ? 0.15 : 1.0;
	const double dx = (where.x - lastMouse.x) * fine;
	const double dy = (where.y - lastMouse.y) * fine;
	lastMouse = where;

	const CRect plot = plotRect ();
	dragThreshold = std::clamp (dragThreshold + dx * kAxisDb / plot.getWidth (), -80.0, 0.0);
	dragRange = std::clamp (dragRange + dy * kAxisDb / plot.getHeight (), 0.0, kMaxRangeDb);
	for (auto [id, v] : {std::pair<uint32_t, double> {kThreshold, dragThreshold}, {kRange, dragRange}})
	{
		const double n = ctrl->spec (id)->toNormalized (v);
		ctrl->setParamNormalized (id, n);
		ctrl->performEdit (id, n);
	}
	invalid ();
	return kMouseEventHandled;
}

void GateCurveView::endDrag ()
{
	if (!dragging)
		return;
	dragging = false;
	ctrl->endEdit (kThreshold);
	ctrl->endEdit (kRange);
	invalid ();
}

CMouseEventResult GateCurveView::onMouseUp (CPoint&, const CButtonState&)
{
	endDrag ();
	return kMouseEventHandled;
}

CMouseEventResult GateCurveView::onMouseCancel ()
{
	endDrag ();
	return kMouseEventHandled;
}

void GateCurveView::onMouseWheelEvent (MouseWheelEvent& event)
{
	if (event.deltaY == 0.0)
		return;
	const bool fine = event.modifiers.has (ModifierKey::Shift);
	const int dir = event.deltaY > 0 ? 1 : -1;
	if (static_cast<Mode> (static_cast<int> (ctrl->getPlain (kMode))) == Mode::Gate)
	{
		const double h = ctrl->getPlain (kHysteresis) + dir * (fine ? 0.1 : 0.5);
		ctrl->editPlain (kHysteresis, std::clamp (h, 0.0, 12.0));
	}
	else
	{
		const double r = ctrl->getPlain (kRatio) * std::exp (dir * (fine ? 0.01 : 0.05));
		ctrl->editPlain (kRatio, std::clamp (r, 1.2, 10.0));
	}
	invalid ();
	event.consumed = true;
}

} // namespace gate
