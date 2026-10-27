#include "RevViews.h"

#include "suite/ui/Theme.h"

#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgraphicspath.h"
#include "vstgui/lib/events.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <optional>

using namespace VSTGUI;
using namespace suite::ui;

namespace rev {

namespace {

constexpr double kFloorDb = -72.0;
const CColor kLowColor {255, 169, 77, 255};
const CColor kHighColor {151, 117, 250, 255};

std::string secondsLabel (double s)
{
	char buf[16];
	std::snprintf (buf, sizeof (buf), s >= 10.0 ? "%.0f s" : s >= 1.0 ? "%.1f s" : "%.0f ms",
	               s >= 1.0 ? s : s * 1000.0);
	return buf;
}

void dashed (CDrawContext* ctx)
{
	static const CCoord dashes[] = {3.0, 3.0};
	ctx->setLineStyle (CLineStyle (CLineStyle::kLineCapButt, CLineStyle::kLineJoinMiter, 0.0, 2, dashes));
}

void performEdit (RevController* ctrl, uint32_t id, double plain)
{
	if (auto s = ctrl->spec (id))
	{
		const double n = s->toNormalized (plain);
		ctrl->setParamNormalized (id, n);
		ctrl->performEdit (id, n);
	}
}

void drawHandle (CDrawContext* ctx, CPoint p, CColor color, bool hot)
{
	const double r = hot ? 6.5 : 5.0;
	if (hot)
	{
		ctx->setFillColor (withAlpha (color, 60));
		ctx->drawEllipse (CRect (p.x - 11, p.y - 11, p.x + 11, p.y + 11), kDrawFilled);
	}
	ctx->setFillColor (color);
	ctx->drawEllipse (CRect (p.x - r, p.y - r, p.x + r, p.y + r), kDrawFilled);
	ctx->setFrameColor (colors::background);
	ctx->setLineWidth (1.5);
	ctx->drawEllipse (CRect (p.x - r, p.y - r, p.x + r, p.y + r), kDrawStroked);
}

} // namespace

// --- DecayView ------------------------------------------------------------

DecayView::DecayView (const CRect& r, RevController* c) : CView (r), ctrl (c) {}

CRect DecayView::plotRect () const
{
	CRect p = getViewSize ();
	p.left += 34;
	p.right -= 12;
	p.top += 30;
	p.bottom -= 22;
	return p;
}

double DecayView::viewSeconds () const
{
	if (frozenViewSeconds > 0.0)
		return frozenViewSeconds;
	const double pre = ctrl->getPlain (kPreDelay) * 0.001;
	const double decay = ctrl->getPlain (kDecay) * 0.001 * std::max (1.0, ctrl->getPlain (kBassMult));
	const double need = (pre + decay) * 1.1;
	for (double nice : {0.5, 1.0, 2.0, 3.0, 5.0, 8.0, 12.0, 20.0, 30.0, 45.0})
		if (need <= nice)
			return nice;
	return 45.0;
}

double DecayView::timeToX (const CRect& plot, double sec) const
{
	return plot.left + plot.getWidth () * std::clamp (sec / viewSeconds (), 0.0, 1.0);
}

double DecayView::dbToY (const CRect& plot, double db) const
{
	return plot.top + plot.getHeight () * std::clamp (db / kFloorDb, 0.0, 1.0);
}

void DecayView::draw (CDrawContext* ctx)
{
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	const CRect r = getViewSize ();
	fillRoundRect (ctx, r, 8, colors::panel);
	const CRect plot = plotRect ();

	const double pre = ctrl->getPlain (kPreDelay) * 0.001;
	const double decay = ctrl->getPlain (kDecay) * 0.001;
	const double bass = ctrl->getPlain (kBassMult);
	const double damp = ctrl->getPlain (kDamping);
	const double size = ctrl->getPlain (kSize);
	const double early = ctrl->getPlain (kEarly) / 100.0;
	const bool frozen = ctrl->getPlain (kFreeze) > 0.5;
	const double total = viewSeconds ();

	drawText (ctx, "DECAY", CRect (r.left + 12, r.top + 8, r.left + 100, r.top + 22), 10, colors::textFaint, 0, true);

	// Grid: dB every 12, time at sensible steps
	ctx->setLineWidth (1.0);
	for (double db = 0.0; db >= kFloorDb; db -= 12.0)
	{
		const double y = std::round (dbToY (plot, db)) + 0.5;
		ctx->setFrameColor (db == 0.0 ? colors::gridStrong : colors::grid);
		ctx->drawLine (CPoint (plot.left, y), CPoint (plot.right, y));
		char buf[8];
		std::snprintf (buf, sizeof (buf), "%g", db);
		drawText (ctx, buf, CRect (plot.left - 32, y - 7, plot.left - 5, y + 7), 10, colors::textFaint, 2);
	}
	double step = total / 5.0;
	for (double nice : {0.05, 0.1, 0.2, 0.25, 0.5, 1.0, 2.0, 2.5, 5.0, 10.0})
		if (total / nice <= 6.0)
		{
			step = nice;
			break;
		}
	for (double t = 0.0; t <= total + 1e-9; t += step)
	{
		const double x = std::round (timeToX (plot, t)) + 0.5;
		ctx->setFrameColor (colors::grid);
		ctx->drawLine (CPoint (x, plot.top), CPoint (x, plot.bottom));
		drawText (ctx, secondsLabel (t), CRect (x - 24, plot.bottom + 4, x + 24, plot.bottom + 18), 10,
		          colors::textFaint);
	}

	std::optional<ConcatClip> clip;
	clip.emplace (*ctx, plot);

	// Pre-delay gap
	const double preX = timeToX (plot, pre);
	ctx->setFillColor (withAlpha (colors::background, 120));
	ctx->drawRect (CRect (plot.left, plot.top, preX, plot.bottom), kDrawFilled);

	// Early reflections as thin spikes (they bunch up on long time axes)
	const double s = sizeScale (size);
	ctx->setLineWidth (1.0);
	for (const auto& t : kEarlyTaps)
	{
		const double g = std::max (t.gainL, t.gainR) * early * 0.6;
		if (g <= 1e-4)
			continue;
		const double x = std::round (timeToX (plot, pre + t.ms * s * 0.001)) + 0.5;
		const double y = dbToY (plot, 20.0 * std::log10 (g) - 6.0);
		ctx->setFrameColor (withAlpha (colors::warning, 150));
		ctx->drawLine (CPoint (x, plot.bottom), CPoint (x, y));
	}

	// Tail slopes: -60 dB per RT60, starting just after the pre-delay
	const double startDb = -8.0;
	auto slope = [&] (double freq, CColor color, bool fill) {
		const double rt = frozen ? 1e9 : decayAt (freq, decay, bass, damp, size);
		const double t0 = pre + 0.005;
		const double tEnd = t0 + (kFloorDb - startDb) / -60.0 * rt;
		const CPoint a (timeToX (plot, t0), dbToY (plot, startDb));
		const double xEnd = timeToX (plot, std::min (tEnd, total));
		const double dbEnd = startDb - 60.0 * (std::min (tEnd, total) - t0) / rt;
		const CPoint b (xEnd, dbToY (plot, dbEnd));
		if (fill)
		{
			if (auto path = owned (ctx->createGraphicsPath ()))
			{
				path->beginSubpath (a.x, plot.bottom);
				path->addLine (a);
				path->addLine (b);
				path->addLine (b.x, plot.bottom);
				path->closeSubpath ();
				ctx->setFillColor (withAlpha (color, 45));
				ctx->drawGraphicsPath (path, CDrawContext::kPathFilled);
			}
		}
		ctx->setFrameColor (color);
		ctx->setLineWidth (fill ? 2.2 : 1.5);
		ctx->drawLine (a, b);
		return rt;
	};
	const double rtLow = slope (100.0, kLowColor, false);
	const double rtHigh = slope (8000.0, kHighColor, false);
	const double rtMid = slope (1000.0, colors::accent, true);

	// Pre-delay marker
	ctx->setFrameColor (drag == Drag::PreDelay ? colors::text : colors::textDim);
	ctx->setLineWidth (1.0);
	dashed (ctx);
	ctx->drawLine (CPoint (preX, plot.top), CPoint (preX, plot.bottom));
	ctx->setLineStyle (kLineSolid);
	drawHandle (ctx, CPoint (preX, plot.top + 10), colors::textDim, drag == Drag::PreDelay);
	drawText (ctx, "PRE", CRect (preX + 8, plot.top + 3, preX + 60, plot.top + 17), 9, colors::textDim, 0, true);

	// Legend
	char buf[96];
	if (frozen)
		std::snprintf (buf, sizeof (buf), "FROZEN");
	else
		std::snprintf (buf, sizeof (buf), "Low %s   Mid %s   High %s", secondsLabel (rtLow).c_str (),
		               secondsLabel (rtMid).c_str (), secondsLabel (rtHigh).c_str ());
	clip.reset ();
	drawText (ctx, buf, CRect (r.right - 300, r.top + 8, r.right - 12, r.top + 22), 11,
	          frozen ? colors::accent : colors::text, 2);
	setDirty (false);
}

void DecayView::perform (uint32_t id, double plain) { performEdit (ctrl, id, plain); }

CMouseEventResult DecayView::onMouseDown (CPoint& where, const CButtonState& buttons)
{
	if (!buttons.isLeftButton ())
		return kMouseEventNotHandled;
	if (buttons.isDoubleClick ())
	{
		ctrl->editNormalized (kDecay, ctrl->spec (kDecay)->defaultNormalized ());
		invalid ();
		return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
	}
	const CRect plot = plotRect ();
	const double preX = timeToX (plot, ctrl->getPlain (kPreDelay) * 0.001);
	frozenViewSeconds = viewSeconds ();
	lastMouse = where;
	if (std::abs (where.x - preX) < 8.0)
	{
		drag = Drag::PreDelay;
		dragValue = ctrl->getPlain (kPreDelay);
		ctrl->beginEdit (kPreDelay);
	}
	else
	{
		drag = Drag::Decay;
		dragValue = std::log (ctrl->getPlain (kDecay));
		ctrl->beginEdit (kDecay);
	}
	invalid ();
	return kMouseEventHandled;
}

CMouseEventResult DecayView::onMouseMoved (CPoint& where, const CButtonState& buttons)
{
	if (drag == Drag::None)
		return kMouseEventNotHandled;
	const double fine = buttons.isShiftSet () ? 0.15 : 1.0;
	const double dx = (where.x - lastMouse.x) * fine;
	lastMouse = where;
	const CRect plot = plotRect ();
	if (drag == Drag::PreDelay)
	{
		dragValue = std::clamp (dragValue + dx * viewSeconds () * 1000.0 / plot.getWidth (), 0.0, kMaxPreDelayMs);
		perform (kPreDelay, dragValue);
	}
	else
	{
		dragValue = std::clamp (dragValue + dx * 0.008, std::log (200.0), std::log (20000.0));
		perform (kDecay, std::exp (dragValue));
	}
	invalid ();
	return kMouseEventHandled;
}

void DecayView::endDrag ()
{
	if (drag == Drag::PreDelay)
		ctrl->endEdit (kPreDelay);
	else if (drag == Drag::Decay)
		ctrl->endEdit (kDecay);
	drag = Drag::None;
	frozenViewSeconds = 0.0;
	invalid ();
}

CMouseEventResult DecayView::onMouseUp (CPoint&, const CButtonState&)
{
	endDrag ();
	return kMouseEventHandled;
}

CMouseEventResult DecayView::onMouseCancel ()
{
	endDrag ();
	return kMouseEventHandled;
}

void DecayView::onMouseWheelEvent (MouseWheelEvent& event)
{
	if (event.deltaY == 0.0)
		return;
	const double step = event.modifiers.has (ModifierKey::Shift) ? 0.5 : 2.0;
	ctrl->editPlain (kSize, std::clamp (ctrl->getPlain (kSize) + (event.deltaY > 0 ? step : -step), 0.0, 100.0));
	invalid ();
	event.consumed = true;
}

// --- ToneView ---------------------------------------------------------------

namespace {
constexpr double kLoHz = 20.0;
constexpr double kHiHz = 20000.0;
constexpr double kMinSec = 0.1;
constexpr double kMaxSec = 40.0;
constexpr double kSpectrumTopDb = 0.0;
constexpr double kSpectrumBottomDb = -90.0;
} // namespace

ToneView::ToneView (const CRect& r, RevController* c) : CView (r), ctrl (c)
{
	const CRect plot = plotRect ();
	const int n = std::max (2, static_cast<int> (plot.getWidth () / 2));
	columnFreqs.resize (n);
	for (int i = 0; i < n; ++i)
		columnFreqs[i] = static_cast<float> (xToFreq (plot, plot.left + plot.getWidth () * i / (n - 1)));
}

CRect ToneView::plotRect () const
{
	CRect p = getViewSize ();
	p.left += 40;
	p.right -= 12;
	p.top += 30;
	p.bottom -= 22;
	return p;
}

double ToneView::freqToX (const CRect& plot, double f) const
{
	return plot.left + plot.getWidth () * std::log (f / kLoHz) / std::log (kHiHz / kLoHz);
}

double ToneView::xToFreq (const CRect& plot, double x) const
{
	return kLoHz * std::pow (kHiHz / kLoHz, (x - plot.left) / plot.getWidth ());
}

double ToneView::secToY (const CRect& plot, double s) const
{
	const double t = std::log (std::clamp (s, kMinSec, kMaxSec) / kMinSec) / std::log (kMaxSec / kMinSec);
	return plot.bottom - plot.getHeight () * t;
}

void ToneView::refresh (float dt)
{
	ctrl->wetAnalyzer ().compute (columnFreqs, spectrumDb, dt);
	invalid ();
}

CPoint ToneView::handlePos (const CRect& plot, int h) const
{
	const double decay = ctrl->getPlain (kDecay) * 0.001;
	const double bass = ctrl->getPlain (kBassMult);
	const double damp = ctrl->getPlain (kDamping);
	const double size = ctrl->getPlain (kSize);
	auto onCurve = [&] (double f) { return CPoint (freqToX (plot, f), secToY (plot, decayAt (f, decay, bass, damp, size))); };
	switch (h)
	{
		case kLowHandle: return onCurve (100.0);
		case kMidHandle: return onCurve (1000.0);
		case kDampHandle: return onCurve (damp);
		case kLowCutHandle: return CPoint (freqToX (plot, ctrl->getPlain (kLowCut)), plot.bottom - 14);
		case kHighCutHandle: return CPoint (freqToX (plot, ctrl->getPlain (kHighCut)), plot.bottom - 14);
	}
	return {};
}

int ToneView::hitHandle (const CPoint& p) const
{
	const CRect plot = plotRect ();
	int best = kNone;
	double bestDist = 12.0;
	for (int h = 0; h < kNumHandles; ++h)
	{
		const CPoint c = handlePos (plot, h);
		const double d = std::hypot (c.x - p.x, c.y - p.y);
		if (d < bestDist)
		{
			bestDist = d;
			best = h;
		}
	}
	return best;
}

uint32_t ToneView::handleParam (int h) const
{
	switch (h)
	{
		case kLowHandle: return kBassMult;
		case kMidHandle: return kDecay;
		case kDampHandle: return kDamping;
		case kLowCutHandle: return kLowCut;
		case kHighCutHandle: return kHighCut;
	}
	return kDecay;
}

void ToneView::draw (CDrawContext* ctx)
{
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	const CRect r = getViewSize ();
	fillRoundRect (ctx, r, 8, colors::panel);
	const CRect plot = plotRect ();
	drawText (ctx, "TONE", CRect (r.left + 12, r.top + 8, r.left + 100, r.top + 22), 10, colors::textFaint, 0, true);
	drawText (ctx, "decay time vs frequency", CRect (r.right - 220, r.top + 8, r.right - 12, r.top + 22), 10,
	          colors::textFaint, 2);

	ctx->setLineWidth (1.0);
	for (double s : {0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0, 20.0})
	{
		const double y = std::round (secToY (plot, s)) + 0.5;
		ctx->setFrameColor (colors::grid);
		ctx->drawLine (CPoint (plot.left, y), CPoint (plot.right, y));
		drawText (ctx, secondsLabel (s), CRect (plot.left - 40, y - 7, plot.left - 4, y + 7), 10, colors::textFaint, 2);
	}
	static const std::pair<double, const char*> fl[] = {{50, "50"},   {100, "100"}, {200, "200"}, {500, "500"},
	                                                     {1000, "1k"}, {2000, "2k"}, {5000, "5k"}, {10000, "10k"}};
	for (auto& [f, text] : fl)
	{
		const double x = std::round (freqToX (plot, f)) + 0.5;
		ctx->setFrameColor (colors::grid);
		ctx->drawLine (CPoint (x, plot.top), CPoint (x, plot.bottom));
		drawText (ctx, text, CRect (x - 20, plot.bottom + 4, x + 20, plot.bottom + 18), 10, colors::textFaint);
	}

	ConcatClip clip (*ctx, plot);

	// Live wet spectrum (its own dB scale, background only)
	if (spectrumDb.size () == columnFreqs.size ())
	{
		if (auto path = owned (ctx->createGraphicsPath ()))
		{
			const size_t n = spectrumDb.size ();
			path->beginSubpath (plot.left, plot.bottom);
			for (size_t i = 0; i < n; ++i)
			{
				const double t = (kSpectrumTopDb - spectrumDb[i]) / (kSpectrumTopDb - kSpectrumBottomDb);
				path->addLine (plot.left + plot.getWidth () * i / (n - 1),
				               plot.top + plot.getHeight () * std::clamp (t, 0.0, 1.0));
			}
			path->addLine (plot.right, plot.bottom);
			path->closeSubpath ();
			ctx->setFillColor (withAlpha (colors::textDim, 45));
			ctx->drawGraphicsPath (path, CDrawContext::kPathFilled);
		}
	}

	// Cut regions
	const double lcX = freqToX (plot, ctrl->getPlain (kLowCut));
	const double hcX = freqToX (plot, ctrl->getPlain (kHighCut));
	ctx->setFillColor (withAlpha (colors::background, 140));
	ctx->drawRect (CRect (plot.left, plot.top, lcX, plot.bottom), kDrawFilled);
	ctx->drawRect (CRect (hcX, plot.top, plot.right, plot.bottom), kDrawFilled);
	ctx->setFrameColor (colors::textDim);
	dashed (ctx);
	ctx->drawLine (CPoint (lcX, plot.top), CPoint (lcX, plot.bottom));
	ctx->drawLine (CPoint (hcX, plot.top), CPoint (hcX, plot.bottom));
	ctx->setLineStyle (kLineSolid);

	// Damping frequency marker
	const double dampX = freqToX (plot, ctrl->getPlain (kDamping));
	ctx->setFrameColor (withAlpha (kHighColor, 120));
	dashed (ctx);
	ctx->drawLine (CPoint (dampX, plot.top), CPoint (dampX, plot.bottom));
	ctx->setLineStyle (kLineSolid);

	// Decay curve
	const bool frozen = ctrl->getPlain (kFreeze) > 0.5;
	const double decay = ctrl->getPlain (kDecay) * 0.001;
	const double bass = ctrl->getPlain (kBassMult);
	const double damp = ctrl->getPlain (kDamping);
	const double size = ctrl->getPlain (kSize);
	if (auto curve = owned (ctx->createGraphicsPath ()))
	{
		const int n = static_cast<int> (plot.getWidth ());
		for (int i = 0; i <= n; ++i)
		{
			const double x = plot.left + i;
			const double sec = frozen ? kMaxSec : decayAt (xToFreq (plot, x), decay, bass, damp, size);
			if (i == 0)
				curve->beginSubpath (x, secToY (plot, sec));
			else
				curve->addLine (x, secToY (plot, sec));
		}
		ctx->setFrameColor (colors::accent);
		ctx->setLineWidth (2.5);
		ctx->setLineStyle (CLineStyle (CLineStyle::kLineCapRound, CLineStyle::kLineJoinRound));
		ctx->drawGraphicsPath (curve, CDrawContext::kPathStroked);
		ctx->setLineStyle (kLineSolid);
	}

	// Handles
	const CColor handleColors[kNumHandles] = {kLowColor, colors::accent, kHighColor, colors::textDim, colors::textDim};
	const char* names[kNumHandles] = {"LOW", "MID", "DAMP", "LO CUT", "HI CUT"};
	for (int h = 0; h < kNumHandles; ++h)
	{
		if (frozen && h <= kDampHandle)
			continue;
		const CPoint p = handlePos (plot, h);
		const bool hot = h == hover || h == dragging;
		drawHandle (ctx, p, handleColors[h], hot);
		if (hot)
		{
			const double tx = h == kHighCutHandle ? p.x - 64 : p.x + 10;
			drawText (ctx, names[h], CRect (tx, p.y - 20, tx + 56, p.y - 8), 9, colors::text, h == kHighCutHandle ? 2 : 0, true);
		}
	}
	setDirty (false);
}

void ToneView::perform (uint32_t id, double plain) { performEdit (ctrl, id, plain); }

CMouseEventResult ToneView::onMouseDown (CPoint& where, const CButtonState& buttons)
{
	if (!buttons.isLeftButton ())
		return kMouseEventNotHandled;
	const int h = hitHandle (where);
	if (h == kNone)
		return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
	const uint32_t id = handleParam (h);
	if (buttons.isDoubleClick ())
	{
		ctrl->editNormalized (id, ctrl->spec (id)->defaultNormalized ());
		invalid ();
		return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
	}
	dragging = h;
	lastMouse = where;
	dragValue = std::log (ctrl->getPlain (id));
	ctrl->beginEdit (id);
	invalid ();
	return kMouseEventHandled;
}

CMouseEventResult ToneView::onMouseMoved (CPoint& where, const CButtonState& buttons)
{
	if (dragging == kNone)
	{
		const int h = hitHandle (where);
		if (h != hover)
		{
			hover = h;
			invalid ();
		}
		return kMouseEventHandled;
	}
	const double fine = buttons.isShiftSet () ? 0.15 : 1.0;
	const double dx = (where.x - lastMouse.x) * fine;
	const double dy = (where.y - lastMouse.y) * fine;
	lastMouse = where;
	const CRect plot = plotRect ();
	const uint32_t id = handleParam (dragging);
	const auto* s = ctrl->spec (id);

	if (dragging == kLowHandle || dragging == kMidHandle)
		dragValue -= dy * std::log (kMaxSec / kMinSec) / plot.getHeight (); // vertical: time axis is log
	else
		dragValue += dx * std::log (kHiHz / kLoHz) / plot.getWidth (); // horizontal: frequency axis is log
	dragValue = std::clamp (dragValue, std::log (s->min), std::log (s->max));
	perform (id, std::exp (dragValue));
	invalid ();
	return kMouseEventHandled;
}

void ToneView::endDrag ()
{
	if (dragging != kNone)
		ctrl->endEdit (handleParam (dragging));
	dragging = kNone;
	invalid ();
}

CMouseEventResult ToneView::onMouseUp (CPoint&, const CButtonState&)
{
	endDrag ();
	return kMouseEventHandled;
}

CMouseEventResult ToneView::onMouseCancel ()
{
	endDrag ();
	return kMouseEventHandled;
}

} // namespace rev
