#include "DlyViews.h"

#include "suite/dsp/Svf.h"
#include "suite/ui/Theme.h"

#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgraphicspath.h"
#include "vstgui/lib/events.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace VSTGUI;
using namespace suite::ui;

namespace dly {

namespace {

const CColor kRightColor {151, 117, 250, 255};
constexpr double kFloorDb = -48.0;

void performEdit (DlyController* ctrl, uint32_t id, double plain)
{
	if (auto s = ctrl->spec (id))
	{
		const double n = s->toNormalized (plain);
		ctrl->setParamNormalized (id, n);
		ctrl->performEdit (id, n);
	}
}

struct Echo
{
	double time;
	int channel; // 0 = left, 1 = right
	double gain;
};

// Echo pattern for a mono-ish input, following the processor's routing.
std::vector<Echo> echoPattern (Mode mode, double tL, double tR, double fb, double maxTime)
{
	std::vector<Echo> out, queue;
	const double t[2] = {tL, tR};
	queue.push_back ({tL, 0, 1.0});
	if (mode != Mode::PingPong)
		queue.push_back ({tR, 1, 1.0});
	const double minGain = std::pow (10.0, kFloorDb / 20.0);
	while (!queue.empty () && out.size () < 64)
	{
		auto e = queue.back ();
		queue.pop_back ();
		if (e.time > maxTime || e.gain < minGain)
			continue;
		out.push_back (e);
		const int next = mode == Mode::Stereo ? e.channel : 1 - e.channel;
		queue.push_back ({e.time + t[next], next, e.gain * fb});
	}
	return out;
}

} // namespace

// --- EchoView ---------------------------------------------------------------

EchoView::EchoView (const CRect& r, DlyController* c) : CView (r), ctrl (c) {}

CRect EchoView::plotRect () const
{
	CRect p = getViewSize ();
	p.left += 12;
	p.right -= 12;
	p.top += 30;
	p.bottom -= 22;
	return p;
}

double EchoView::viewSeconds () const
{
	if (frozenView > 0.0)
		return frozenView;
	double l, r;
	ctrl->currentTimesMs (l, r);
	const double need = std::max (l, r) * 0.001 * 6.0;
	for (double nice : {0.25, 0.5, 1.0, 2.0, 3.0, 4.0, 6.0, 8.0, 12.0, 16.0, 24.0})
		if (need <= nice)
			return nice;
	return 24.0;
}

void EchoView::draw (CDrawContext* ctx)
{
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	const CRect r = getViewSize ();
	drawPanel (ctx, r);
	const CRect plot = plotRect ();
	const double total = viewSeconds ();
	const double midY = plot.getCenter ().y;
	auto tx = [&] (double s) { return plot.left + plot.getWidth () * s / total; };

	double tL, tR;
	ctrl->currentTimesMs (tL, tR);
	const bool sync = ctrl->getPlain (kSync) > 0.5;
	const double fb = ctrl->getPlain (kFeedback) / 100.0;
	const auto mode = static_cast<Mode> (static_cast<int> (ctrl->getPlain (kMode)));

	// Header readout
	char buf[96];
	if (sync)
		std::snprintf (buf, sizeof (buf), "L %s  %.0f ms    R %s  %.0f ms    %.1f BPM",
		               kNoteNames[static_cast<int> (ctrl->getPlain (kNoteL))], tL,
		               tR == tL ? kNoteNames[static_cast<int> (ctrl->getPlain (kNoteL))]
		                        : kNoteNames[static_cast<int> (ctrl->getPlain (kNoteR))],
		               tR, ctrl->tempo ());
	else
		std::snprintf (buf, sizeof (buf), "L %.0f ms    R %.0f ms", tL, tR);
	drawText (ctx, "Echoes", CRect (r.left + 12, r.top + 8, r.left + 100, r.top + 22), 11, colors::textDim, 0, true);
	drawText (ctx, buf, CRect (r.right - 420, r.top + 8, r.right - 12, r.top + 22), 11, colors::text, 2);

	// Grid: beats when synced, otherwise even time steps
	ctx->setLineWidth (1.0);
	const double beat = 60.0 / ctrl->tempo ();
	double step = sync ? beat : total / 8.0;
	while (sync && total / step > 24.0)
		step *= 2.0;
	const bool labelEveryLine = total / step <= 8.0;
	int index = 0;
	for (double s = 0.0; s <= total + 1e-9; s += step, ++index)
	{
		const double x = std::round (tx (s)) + 0.5;
		const bool bar = sync && index % 4 == 0;
		ctx->setFrameColor (bar ? colors::gridStrong : colors::grid);
		ctx->drawLine (CPoint (x, plot.top), CPoint (x, plot.bottom));
		if (!sync || bar || labelEveryLine)
		{
			std::snprintf (buf, sizeof (buf), s >= 1.0 ? "%.2f s" : "%.0f ms", s >= 1.0 ? s : s * 1000.0);
			drawText (ctx, buf, CRect (x - 30, plot.bottom + 4, x + 30, plot.bottom + 18), 10, colors::textFaint);
		}
	}
	ctx->setFrameColor (colors::gridStrong);
	ctx->drawLine (CPoint (plot.left, midY), CPoint (plot.right, midY));
	drawText (ctx, "L", CRect (plot.left + 4, plot.top + 2, plot.left + 20, plot.top + 16), 10, colors::accent, 0, true);
	drawText (ctx, "R", CRect (plot.left + 4, plot.bottom - 16, plot.left + 20, plot.bottom - 2), 10, kRightColor, 0, true);

	ConcatClip clip (*ctx, plot);
	const double half = plot.getHeight () * 0.5 - 6.0;
	auto height = [&] (double gain) {
		const double db = 20.0 * std::log10 (std::max (gain, 1e-6));
		return half * std::clamp (1.0 - db / kFloorDb, 0.0, 1.0);
	};

	// Dry hit at t = 0
	ctx->setFillColor (colors::textDim);
	ctx->drawRect (CRect (tx (0) + 1, midY - half, tx (0) + 5, midY + half), kDrawFilled);

	for (const auto& e : echoPattern (mode, tL * 0.001, tR * 0.001, fb, total))
	{
		const double x = tx (e.time);
		const double h = height (e.gain);
		const CColor color = e.channel == 0 ? colors::accent : kRightColor;
		CRect bar = e.channel == 0 ? CRect (x - 2.5, midY - h, x + 2.5, midY - 1)
		                           : CRect (x - 2.5, midY + 1, x + 2.5, midY + h);
		ctx->setFillColor (withAlpha (color, dragging ? 255 : 220));
		ctx->drawRect (bar, kDrawFilled);
		CRect cap = e.channel == 0 ? CRect (x - 4, midY - h - 2, x + 4, midY - h + 2)
		                           : CRect (x - 4, midY + h - 2, x + 4, midY + h + 2);
		ctx->setFillColor (color);
		ctx->drawRect (cap, kDrawFilled);
	}
	setDirty (false);
}

CMouseEventResult EchoView::onMouseDown (CPoint& where, const CButtonState& buttons)
{
	if (!buttons.isLeftButton ())
		return kMouseEventNotHandled;
	const bool linked = ctrl->getPlain (kLink) > 0.5 ||
	                    static_cast<Mode> (static_cast<int> (ctrl->getPlain (kMode))) == Mode::PingPong;
	const bool sync = ctrl->getPlain (kSync) > 0.5;
	const bool right = !linked && where.y > plotRect ().getCenter ().y;
	timeParam = sync ? (right ? kNoteR : kNoteL) : (right ? kTimeR : kTimeL);
	dragTime = sync ? ctrl->getPlain (timeParam) : std::log (ctrl->getPlain (timeParam));
	dragFeedback = ctrl->getPlain (kFeedback);
	frozenView = viewSeconds ();
	lastMouse = where;
	dragging = true;
	ctrl->beginEdit (timeParam);
	ctrl->beginEdit (kFeedback);
	invalid ();
	return kMouseEventHandled;
}

CMouseEventResult EchoView::onMouseMoved (CPoint& where, const CButtonState& buttons)
{
	if (!dragging)
		return kMouseEventNotHandled;
	const double fine = buttons.isShiftSet () ? 0.15 : 1.0;
	const double dx = (where.x - lastMouse.x) * fine;
	const double dy = (where.y - lastMouse.y) * fine;
	lastMouse = where;
	if (timeParam == kNoteL || timeParam == kNoteR)
	{
		dragTime = std::clamp (dragTime + dx / 28.0, 0.0, double (kNumNotes - 1)); // one note per 28 px
		performEdit (ctrl, timeParam, std::round (dragTime));
	}
	else
	{
		dragTime = std::clamp (dragTime + dx * 0.006, std::log (kMinTimeMs), std::log (kMaxTimeMs));
		performEdit (ctrl, timeParam, std::exp (dragTime));
	}
	dragFeedback = std::clamp (dragFeedback - dy * 0.4, 0.0, 100.0);
	performEdit (ctrl, kFeedback, dragFeedback);
	invalid ();
	return kMouseEventHandled;
}

void EchoView::endDrag ()
{
	if (!dragging)
		return;
	dragging = false;
	ctrl->endEdit (timeParam);
	ctrl->endEdit (kFeedback);
	frozenView = 0.0;
	invalid ();
}

CMouseEventResult EchoView::onMouseUp (CPoint&, const CButtonState&)
{
	endDrag ();
	return kMouseEventHandled;
}

CMouseEventResult EchoView::onMouseCancel ()
{
	endDrag ();
	return kMouseEventHandled;
}

void EchoView::onMouseWheelEvent (MouseWheelEvent& event)
{
	if (event.deltaY == 0.0)
		return;
	const double step = event.modifiers.has (ModifierKey::Shift) ? 0.5 : 2.0;
	ctrl->editPlain (kFeedback, std::clamp (ctrl->getPlain (kFeedback) + (event.deltaY > 0 ? step : -step), 0.0, 100.0));
	invalid ();
	event.consumed = true;
}

// --- FilterView -----------------------------------------------------------

namespace {
constexpr double kLoHz = 20.0;
constexpr double kHiHz = 20000.0;
constexpr double kTopDb = 6.0;
constexpr double kBottomDb = -30.0;
} // namespace

FilterView::FilterView (const CRect& r, DlyController* c) : CView (r), ctrl (c) {}

CRect FilterView::plotRect () const
{
	CRect p = getViewSize ();
	p.left += 30;
	p.right -= 10;
	p.top += 30;
	p.bottom -= 22;
	return p;
}

double FilterView::freqToX (double f) const
{
	const CRect p = plotRect ();
	return p.left + p.getWidth () * std::log (f / kLoHz) / std::log (kHiHz / kLoHz);
}

double FilterView::dbToY (double db) const
{
	const CRect p = plotRect ();
	return p.top + p.getHeight () * std::clamp ((kTopDb - db) / (kTopDb - kBottomDb), 0.0, 1.0);
}

void FilterView::draw (CDrawContext* ctx)
{
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	const CRect r = getViewSize ();
	drawPanel (ctx, r);
	const CRect plot = plotRect ();
	drawText (ctx, "Repeat tone", CRect (r.left + 12, r.top + 8, r.right - 12, r.top + 22), 11, colors::textDim, 0, true);

	ctx->setLineWidth (1.0);
	for (double db = 0.0; db >= kBottomDb; db -= 12.0)
	{
		const double y = std::round (dbToY (db)) + 0.5;
		ctx->setFrameColor (db == 0.0 ? colors::gridStrong : colors::grid);
		ctx->drawLine (CPoint (plot.left, y), CPoint (plot.right, y));
		char buf[8];
		std::snprintf (buf, sizeof (buf), "%g", db);
		drawText (ctx, buf, CRect (plot.left - 28, y - 7, plot.left - 4, y + 7), 10, colors::textFaint, 2);
	}
	for (auto [f, text] : {std::pair<double, const char*> {100, "100"}, {1000, "1k"}, {10000, "10k"}})
	{
		const double x = std::round (freqToX (f)) + 0.5;
		ctx->setFrameColor (colors::grid);
		ctx->drawLine (CPoint (x, plot.top), CPoint (x, plot.bottom));
		drawText (ctx, text, CRect (x - 16, plot.bottom + 4, x + 16, plot.bottom + 18), 10, colors::textFaint);
	}

	ConcatClip clip (*ctx, plot);
	const double sr = 48000.0;
	const auto lo = suite::dsp::designBand (suite::dsp::FilterType::LowCut, ctrl->getPlain (kLowCut), 0.707, 0, 1, sr);
	const auto hi = suite::dsp::designBand (suite::dsp::FilterType::HighCut, ctrl->getPlain (kHighCut), 0.707, 0, 1, sr);

	// Repeat n has passed through the filters n times.
	for (int n = 4; n >= 1; --n)
	{
		auto path = owned (ctx->createGraphicsPath ());
		if (!path)
			continue;
		const int steps = static_cast<int> (plot.getWidth ());
		for (int i = 0; i <= steps; ++i)
		{
			const double x = plot.left + i;
			const double f = kLoHz * std::pow (kHiHz / kLoHz, double (i) / steps);
			const double mag = lo.magnitude (f, sr) * hi.magnitude (f, sr);
			const double db = n * 20.0 * std::log10 (std::max (mag, 1e-6));
			if (i == 0)
				path->beginSubpath (x, dbToY (db));
			else
				path->addLine (x, dbToY (db));
		}
		ctx->setFrameColor (withAlpha (colors::accent, n == 1 ? 255 : static_cast<uint8_t> (180 / n)));
		ctx->setLineWidth (n == 1 ? 2.2 : 1.2);
		ctx->drawGraphicsPath (path, CDrawContext::kPathStroked);
	}

	for (uint32_t id : {static_cast<uint32_t> (kLowCut), static_cast<uint32_t> (kHighCut)})
	{
		const CPoint p (freqToX (ctrl->getPlain (id)), dbToY (-3.0));
		const bool hot = active && dragging == id;
		const double rad = hot ? 6.5 : 5.0;
		ctx->setFillColor (colors::text);
		ctx->drawEllipse (CRect (p.x - rad, p.y - rad, p.x + rad, p.y + rad), kDrawFilled);
	}
	setDirty (false);
}

CMouseEventResult FilterView::onMouseDown (CPoint& where, const CButtonState& buttons)
{
	if (!buttons.isLeftButton ())
		return kMouseEventNotHandled;
	// Grab the nearer of the two handles.
	const double dLow = std::abs (where.x - freqToX (ctrl->getPlain (kLowCut)));
	const double dHigh = std::abs (where.x - freqToX (ctrl->getPlain (kHighCut)));
	dragging = dLow <= dHigh ? kLowCut : kHighCut;
	if (buttons.isDoubleClick ())
	{
		ctrl->editNormalized (dragging, ctrl->spec (dragging)->defaultNormalized ());
		invalid ();
		return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
	}
	active = true;
	lastMouse = where;
	dragValue = std::log (ctrl->getPlain (dragging));
	ctrl->beginEdit (dragging);
	invalid ();
	return kMouseEventHandled;
}

CMouseEventResult FilterView::onMouseMoved (CPoint& where, const CButtonState& buttons)
{
	if (!active)
		return kMouseEventNotHandled;
	const double fine = buttons.isShiftSet () ? 0.15 : 1.0;
	dragValue += (where.x - lastMouse.x) * fine * std::log (kHiHz / kLoHz) / plotRect ().getWidth ();
	lastMouse = where;
	const auto* s = ctrl->spec (dragging);
	dragValue = std::clamp (dragValue, std::log (s->min), std::log (s->max));
	performEdit (ctrl, dragging, std::exp (dragValue));
	invalid ();
	return kMouseEventHandled;
}

void FilterView::endDrag ()
{
	if (!active)
		return;
	active = false;
	ctrl->endEdit (dragging);
	invalid ();
}

CMouseEventResult FilterView::onMouseUp (CPoint&, const CButtonState&)
{
	endDrag ();
	return kMouseEventHandled;
}

CMouseEventResult FilterView::onMouseCancel ()
{
	endDrag ();
	return kMouseEventHandled;
}

} // namespace dly
