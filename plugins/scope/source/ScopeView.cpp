#include "ScopeView.h"

#include "suite/ui/Theme.h"

#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgraphicspath.h"
#include "vstgui/lib/events.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>

using namespace VSTGUI;
using namespace suite::ui;

namespace scope {

namespace {

const CColor kRightColor {151, 117, 250, 255};

void performEdit (ScopeController* ctrl, uint32_t id, double plain)
{
	if (auto s = ctrl->spec (id))
	{
		const double n = s->toNormalized (plain);
		ctrl->setParamNormalized (id, n);
		ctrl->performEdit (id, n);
	}
}

std::string timeLabel (double seconds)
{
	char buf[24];
	if (seconds >= 1.0)
		std::snprintf (buf, sizeof (buf), "%.2f s", seconds);
	else if (seconds >= 0.01)
		std::snprintf (buf, sizeof (buf), "%.0f ms", seconds * 1000.0);
	else
		std::snprintf (buf, sizeof (buf), "%.2f ms", seconds * 1000.0);
	return buf;
}

} // namespace

ScopeView::ScopeView (const CRect& r, ScopeController* c) : CView (r), ctrl (c) {}

CRect ScopeView::plotRect () const
{
	CRect p = getViewSize ();
	p.left += 48;
	p.right -= 14;
	p.top += 32;
	p.bottom -= 24;
	return p;
}

double ScopeView::gainLinear () const { return std::pow (10.0, ctrl->getPlain (kGain) / 20.0); }

float ScopeView::source (size_t i) const
{
	switch (static_cast<Channels> (static_cast<int> (ctrl->getPlain (kChannels))))
	{
		case Channels::Side: return 0.5f * (l[i] - r[i]);
		case Channels::Left: return l[i];
		case Channels::Right: return r[i];
		default: return 0.5f * (l[i] + r[i]);
	}
}

void ScopeView::addSample (Trace& t, double x, float v)
{
	const int cols = static_cast<int> (t.lo.size ());
	const int c = std::clamp (static_cast<int> (x * cols), 0, cols - 1);
	t.lo[c] = std::min (t.lo[c], v);
	t.hi[c] = std::max (t.hi[c], v);
	t.valid[c] = true;
}

void ScopeView::refresh (bool frozen)
{
	if (!frozen)
		buildTraces ();
	invalid ();
}

void ScopeView::buildTraces ()
{
	const double sr = ctrl->sampleRate ();
	const auto mode = static_cast<Trigger> (static_cast<int> (ctrl->getPlain (kTrigger)));
	const auto chans = static_cast<Channels> (static_cast<int> (ctrl->getPlain (kChannels)));
	const int cols = std::max (2, static_cast<int> (plotRect ().getWidth ()));
	for (Trace* t : {&traceA, &traceB, &prevA, &prevB})
	{
		t->lo.assign (cols, std::numeric_limits<float>::max ());
		t->hi.assign (cols, std::numeric_limits<float>::lowest ());
		t->valid.assign (cols, false);
	}
	twoTraces = chans == Channels::Both;
	cursor = -1.0;

	auto plot = [&] (size_t i, double x, Trace& a, Trace& b) {
		if (twoTraces)
		{
			addSample (a, x, l[i]);
			addSample (b, x, r[i]);
		}
		else
			addSample (a, x, source (i));
	};

	const double timeSec = ctrl->getPlain (kTime) * 0.001;
	const size_t n = std::max<size_t> (2, static_cast<size_t> (timeSec * sr));
	displaySeconds = timeSec;

	if (mode == Trigger::BeatSync)
	{
		// Estimate tempo from the song position over the last 100 ms.
		ctrl->copyRecent (static_cast<size_t> (0.1 * sr), l, r, ppq);
		const double pNow = ppq.empty () ? -1.0 : ppq.back ();
		const double p0 = ppq.empty () ? -1.0 : ppq.front ();
		if (pNow >= 0.0 && p0 >= 0.0 && pNow > p0)
		{
			const double quartersPerSec = (pNow - p0) / ((ppq.size () - 1) / sr);
			const double len = kBeatLengthQuarters[std::clamp (static_cast<int> (ctrl->getPlain (kBeatLength)), 0, kNumBeatLengths - 1)];
			displaySeconds = len / quartersPerSec;
			ctrl->copyRecent (static_cast<size_t> (2.05 * displaySeconds * sr), l, r, ppq);
			const double cycleStart = std::floor (pNow / len) * len;
			cursor = (pNow - cycleStart) / len;
			for (size_t i = 0; i < l.size (); ++i)
			{
				const double p = ppq[i];
				if (p < 0.0 || p > pNow)
					continue; // stopped, or from before a loop jump
				if (p >= cycleStart)
					plot (i, (p - cycleStart) / len, traceA, traceB);
				else if (p >= cycleStart - len)
				{
					const double x = (p - (cycleStart - len)) / len;
					if (x > cursor)
						plot (i, x, prevA, prevB);
				}
			}
			char buf[64];
			std::snprintf (buf, sizeof (buf), "Beat Sync  %.1f BPM", quartersPerSec * 60.0);
			status = buf;
			return;
		}
		status = "Beat Sync: transport stopped (free run)";
	}

	if (mode == Trigger::RisingEdge)
	{
		const size_t extra = std::max (n, static_cast<size_t> (0.05 * sr));
		ctrl->copyRecent (n + extra, l, r, ppq);
		const size_t len = l.size ();
		float peak = 0.0f;
		for (size_t i = 0; i < len; ++i)
			peak = std::max (peak, std::abs (source (i)));
		const float level = static_cast<float> (ctrl->getPlain (kTriggerLevel) / 100.0);
		const float hysteresis = std::max (0.02f * peak, 1e-4f);
		bool armed = false;
		long found = -1;
		for (size_t i = 1; i + n <= len; ++i)
		{
			const float s = source (i);
			if (s < level - hysteresis)
				armed = true;
			if (armed && source (i - 1) < level && s >= level)
			{
				found = static_cast<long> (i);
				armed = false;
			}
		}
		if (found >= 0)
		{
			for (size_t i = 0; i < n; ++i)
				plot (found + i, double (i) / n, traceA, traceB);
			status = "Triggered";
			return;
		}
		// No trigger: show the newest window untriggered
		for (size_t i = 0; i < n && len >= n; ++i)
			plot (len - n + i, double (i) / n, traceA, traceB);
		status = "Waiting for trigger";
		return;
	}

	ctrl->copyRecent (n, l, r, ppq);
	for (size_t i = 0; i < l.size (); ++i)
		plot (i, double (i) / l.size (), traceA, traceB);
	if (mode == Trigger::Free)
		status = "Free run";
}

void ScopeView::draw (CDrawContext* ctx)
{
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	const CRect v = getViewSize ();
	fillRoundRect (ctx, v, 8, colors::panel);
	const CRect plot = plotRect ();
	const double g = gainLinear ();
	const double cy = plot.getCenter ().y;
	const double half = plot.getHeight () * 0.5;
	auto toY = [&] (double s) { return std::clamp (cy - s * g * half, plot.top, plot.bottom); };
	const auto mode = static_cast<Trigger> (static_cast<int> (ctrl->getPlain (kTrigger)));

	// Header: measurements and status
	char buf[96];
	const double f = ctrl->frequency ();
	if (f > 0.0)
		std::snprintf (buf, sizeof (buf), "%.1f Hz   period %s", f, timeLabel (1.0 / f).c_str ());
	else
		std::snprintf (buf, sizeof (buf), "-- Hz");
	drawText (ctx, "SCOPE", CRect (v.left + 12, v.top + 9, v.left + 80, v.top + 23), 10, colors::textFaint, 0, true);
	drawText (ctx, buf, CRect (v.left + 90, v.top + 6, v.left + 420, v.top + 26), 13, colors::text, 0, true);
	drawText (ctx, status, CRect (v.right - 340, v.top + 9, v.right - 14, v.top + 23), 11, colors::textDim, 2);

	// Grid: 10 x 8 divisions
	ctx->setLineWidth (1.0);
	for (int i = 0; i <= 10; ++i)
	{
		const double x = std::round (plot.left + plot.getWidth () * i / 10.0) + 0.5;
		ctx->setFrameColor (i == 0 || i == 10 ? colors::gridStrong : colors::grid);
		ctx->drawLine (CPoint (x, plot.top), CPoint (x, plot.bottom));
		if (i % 2 == 0)
			drawText (ctx, timeLabel (displaySeconds * i / 10.0), CRect (x - 30, plot.bottom + 5, x + 30, plot.bottom + 19), 10,
			          colors::textFaint);
	}
	for (int j = 0; j <= 8; ++j)
	{
		const double y = std::round (plot.top + plot.getHeight () * j / 8.0) + 0.5;
		ctx->setFrameColor (j == 4 ? colors::gridStrong : colors::grid);
		ctx->drawLine (CPoint (plot.left, y), CPoint (plot.right, y));
		const double amp = (1.0 - j / 4.0) / g;
		std::snprintf (buf, sizeof (buf), "%.2f", amp);
		drawText (ctx, j % 2 == 0 ? buf : "", CRect (plot.left - 46, y - 7, plot.left - 6, y + 7), 10, colors::textFaint, 2);
	}

	ConcatClip clip (*ctx, plot);

	// Beat lines in Beat Sync
	if (mode == Trigger::BeatSync && cursor >= 0.0)
	{
		const double len = kBeatLengthQuarters[std::clamp (static_cast<int> (ctrl->getPlain (kBeatLength)), 0, kNumBeatLengths - 1)];
		if (len > 1.0)
		{
			ctx->setFrameColor (withAlpha (colors::accent, 50));
			for (double q = 1.0; q < len; q += 1.0)
			{
				const double x = plot.left + plot.getWidth () * q / len;
				ctx->drawLine (CPoint (x, plot.top), CPoint (x, plot.bottom));
			}
		}
	}

	auto drawTrace = [&] (const Trace& t, CColor color, uint8_t alpha) {
		const int cols = static_cast<int> (t.lo.size ());
		auto hiPath = owned (ctx->createGraphicsPath ());
		auto loPath = owned (ctx->createGraphicsPath ());
		auto band = owned (ctx->createGraphicsPath ());
		if (!hiPath || !loPath || !band)
			return;
		bool open = false;
		std::vector<CPoint> lows;
		for (int c = 0; c < cols; ++c)
		{
			if (!t.valid[c])
				continue;
			const double x = plot.left + c + 0.5;
			const CPoint hp (x, toY (t.hi[c])), lp (x, toY (t.lo[c]));
			if (!open)
			{
				hiPath->beginSubpath (hp);
				loPath->beginSubpath (lp);
				band->beginSubpath (hp);
				open = true;
			}
			else
			{
				hiPath->addLine (hp);
				loPath->addLine (lp);
				band->addLine (hp);
			}
			lows.push_back (lp);
		}
		if (!open)
			return;
		for (auto it = lows.rbegin (); it != lows.rend (); ++it)
			band->addLine (*it);
		band->closeSubpath ();
		ctx->setFillColor (withAlpha (color, static_cast<uint8_t> (alpha / 3)));
		ctx->drawGraphicsPath (band, CDrawContext::kPathFilled);
		ctx->setFrameColor (withAlpha (color, alpha));
		ctx->setLineWidth (1.5);
		ctx->drawGraphicsPath (hiPath, CDrawContext::kPathStroked);
		ctx->drawGraphicsPath (loPath, CDrawContext::kPathStroked);
	};

	if (cursor >= 0.0)
	{
		drawTrace (prevA, colors::accent, 70);
		if (twoTraces)
			drawTrace (prevB, kRightColor, 60);
	}
	if (twoTraces)
		drawTrace (traceB, kRightColor, 220);
	drawTrace (traceA, colors::accent, 255);

	// Beat Sync cursor
	if (cursor >= 0.0)
	{
		const double x = plot.left + plot.getWidth () * cursor;
		ctx->setFrameColor (withAlpha (colors::text, 140));
		ctx->setLineWidth (1.0);
		ctx->drawLine (CPoint (x, plot.top), CPoint (x, plot.bottom));
	}

	// Trigger level and handle
	if (mode == Trigger::RisingEdge)
	{
		const double y = toY (ctrl->getPlain (kTriggerLevel) / 100.0);
		static const CCoord dashes[] = {4.0, 4.0};
		ctx->setLineStyle (CLineStyle (CLineStyle::kLineCapButt, CLineStyle::kLineJoinMiter, 0.0, 2, dashes));
		ctx->setFrameColor (withAlpha (colors::warning, 160));
		ctx->drawLine (CPoint (plot.left, y), CPoint (plot.right, y));
		ctx->setLineStyle (kLineSolid);
		CRect h (plot.left + 2, y - 6, plot.left + 14, y + 6);
		ctx->setFillColor (colors::warning);
		if (auto tri = owned (ctx->createGraphicsPath ()))
		{
			tri->beginSubpath (h.left, h.top);
			tri->addLine (h.right, y);
			tri->addLine (h.left, h.bottom);
			tri->closeSubpath ();
			ctx->drawGraphicsPath (tri, CDrawContext::kPathFilled);
		}
	}
	setDirty (false);
}

CMouseEventResult ScopeView::onMouseDown (CPoint& where, const CButtonState& buttons)
{
	if (!buttons.isLeftButton ())
		return kMouseEventNotHandled;
	if (buttons.isDoubleClick ())
	{
		ctrl->editNormalized (kTime, ctrl->spec (kTime)->defaultNormalized ());
		ctrl->editNormalized (kGain, ctrl->spec (kGain)->defaultNormalized ());
		return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
	}
	const CRect plot = plotRect ();
	const double g = gainLinear ();
	const double levelY = plot.getCenter ().y - ctrl->getPlain (kTriggerLevel) / 100.0 * g * plot.getHeight () * 0.5;
	const bool edge = static_cast<Trigger> (static_cast<int> (ctrl->getPlain (kTrigger))) == Trigger::RisingEdge;
	lastMouse = where;
	if (edge && where.x < plot.left + 22 && std::abs (where.y - levelY) < 10)
	{
		drag = Drag::Level;
		ctrl->beginEdit (kTriggerLevel);
	}
	else
	{
		drag = Drag::View;
		dragTime = std::log (ctrl->getPlain (kTime));
		dragLevel = ctrl->getPlain (kBeatLength); // reused as fractional beat-length index
		dragGain = ctrl->getPlain (kGain);
		ctrl->beginEdit (kTime);
		ctrl->beginEdit (kBeatLength);
		ctrl->beginEdit (kGain);
	}
	return kMouseEventHandled;
}

CMouseEventResult ScopeView::onMouseMoved (CPoint& where, const CButtonState& buttons)
{
	if (drag == Drag::None)
		return kMouseEventNotHandled;
	const double fine = buttons.isShiftSet () ? 0.15 : 1.0;
	const double dx = (where.x - lastMouse.x) * fine;
	const double dy = (where.y - lastMouse.y) * fine;
	lastMouse = where;
	const CRect plot = plotRect ();
	if (drag == Drag::Level)
	{
		const double level = (plot.getCenter ().y - where.y) / (gainLinear () * plot.getHeight () * 0.5);
		performEdit (ctrl, kTriggerLevel, std::clamp (level, -1.0, 1.0) * 100.0);
	}
	else
	{
		if (static_cast<Trigger> (static_cast<int> (ctrl->getPlain (kTrigger))) == Trigger::BeatSync)
		{
			dragLevel = std::clamp (dragLevel - dx / 40.0, 0.0, double (kNumBeatLengths - 1));
			performEdit (ctrl, kBeatLength, std::round (dragLevel));
		}
		else
		{
			dragTime = std::clamp (dragTime - dx * 0.01, std::log (1.0), std::log (2000.0)); // drag left = zoom out
			performEdit (ctrl, kTime, std::exp (dragTime));
		}
		dragGain = std::clamp (dragGain - dy * 0.1, -12.0, 36.0);
		performEdit (ctrl, kGain, dragGain);
	}
	invalid ();
	return kMouseEventHandled;
}

CMouseEventResult ScopeView::onMouseUp (CPoint&, const CButtonState&)
{
	return onMouseCancel ();
}

CMouseEventResult ScopeView::onMouseCancel ()
{
	if (drag == Drag::Level)
		ctrl->endEdit (kTriggerLevel);
	else if (drag == Drag::View)
	{
		ctrl->endEdit (kTime);
		ctrl->endEdit (kBeatLength);
		ctrl->endEdit (kGain);
	}
	drag = Drag::None;
	return kMouseEventHandled;
}

void ScopeView::onMouseWheelEvent (MouseWheelEvent& event)
{
	if (event.deltaY == 0.0)
		return;
	const double factor = event.modifiers.has (ModifierKey::Shift) ? 1.03 : 1.15;
	const double t = ctrl->getPlain (kTime) * (event.deltaY > 0 ? 1.0 / factor : factor);
	ctrl->editPlain (kTime, std::clamp (t, 1.0, 2000.0));
	event.consumed = true;
}

} // namespace scope
