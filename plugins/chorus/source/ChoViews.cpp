#include "ChoViews.h"

#include "suite/dsp/Svf.h"
#include "suite/ui/Theme.h"

#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgraphicspath.h"
#include "vstgui/lib/events.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdio>

using namespace VSTGUI;
using namespace suite::ui;

namespace cho {

namespace {

const CColor kRightColor {151, 117, 250, 255};
constexpr double kPi = 3.14159265358979323846;

void performEdit (ChoController* ctrl, uint32_t id, double plain)
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
	Shape shape;
	double delay, depth, spread, feedback;
	int voices;
};

Settings currentSettings (ChoController* ctrl)
{
	return {static_cast<Shape> (static_cast<int> (ctrl->getPlain (kShape))), ctrl->getPlain (kDelay),
	        ctrl->getPlain (kDepth) / 100.0, ctrl->getPlain (kSpread), ctrl->getPlain (kFeedback) / 100.0,
	        static_cast<int> (ctrl->getPlain (kVoices)) + 1};
}

void dashed (CDrawContext* ctx)
{
	static const CCoord dashes[] = {4.0, 3.0};
	ctx->setLineStyle (CLineStyle (CLineStyle::kLineCapButt, CLineStyle::kLineJoinMiter, 0.0, 2, dashes));
}

} // namespace

// --- LfoView ----------------------------------------------------------------

LfoView::LfoView (const CRect& r, ChoController* c) : CView (r), ctrl (c) {}

void LfoView::advance (float dt)
{
	const double p = ctrl->lfoPhase ();
	if (p >= 0.0 && p != lastProcessorPhase)
	{
		lastProcessorPhase = p; // audio is running: follow the processor
		playhead = p;
	}
	else
	{
		playhead += ctrl->getPlain (kRate) * dt; // editor open without audio
		playhead -= std::floor (playhead);
	}
	invalid ();
}

CRect LfoView::plotRect () const
{
	CRect p = getViewSize ();
	p.left += 46;
	p.right -= 14;
	p.top += 30;
	p.bottom -= 22;
	return p;
}

void LfoView::draw (CDrawContext* ctx)
{
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	const CRect r = getViewSize ();
	fillRoundRect (ctx, r, 8, colors::panel);
	const CRect plot = plotRect ();
	const auto s = currentSettings (ctrl);
	const double rate = ctrl->getPlain (kRate);

	char buf[96];
	std::snprintf (buf, sizeof (buf), "%d voice%s   %.2f Hz   period %.0f ms", s.voices, s.voices > 1 ? "s" : "",
	               rate, 1000.0 / rate);
	drawText (ctx, "LFO", CRect (r.left + 12, r.top + 8, r.left + 100, r.top + 22), 10, colors::textFaint, 0, true);
	drawText (ctx, buf, CRect (r.right - 320, r.top + 8, r.right - 12, r.top + 22), 11, colors::text, 2);

	// Vertical range: the longest any voice can reach
	const double maxMs = std::max (1.0, voiceCenterMs (s.delay, s.voices - 1, s.voices) * (1.0 + 0.9 * s.depth) * 1.1);
	auto toY = [&] (double ms) { return plot.bottom - plot.getHeight () * std::clamp (ms / maxMs, 0.0, 1.0); };
	auto toX = [&] (double phase) { return plot.left + plot.getWidth () * phase; };

	ctx->setLineWidth (1.0);
	for (int i = 0; i <= 4; ++i)
	{
		const double ms = maxMs * i / 4.0;
		const double y = std::round (toY (ms)) + 0.5;
		ctx->setFrameColor (i == 0 ? colors::gridStrong : colors::grid);
		ctx->drawLine (CPoint (plot.left, y), CPoint (plot.right, y));
		std::snprintf (buf, sizeof (buf), ms >= 10.0 ? "%.0f ms" : "%.1f ms", ms);
		drawText (ctx, buf, CRect (plot.left - 46, y - 7, plot.left - 4, y + 7), 10, colors::textFaint, 2);
	}
	for (int i = 0; i <= 4; ++i)
	{
		const double x = std::round (toX (i / 4.0)) + 0.5;
		ctx->setFrameColor (colors::grid);
		ctx->drawLine (CPoint (x, plot.top), CPoint (x, plot.bottom));
		static const char* labels[] = {"0", "1/4", "1/2", "3/4", "1 cycle"};
		drawText (ctx, labels[i], CRect (x - 30, plot.bottom + 4, x + 30, plot.bottom + 18), 10, colors::textFaint);
	}

	ConcatClip clip (*ctx, plot);
	const int steps = static_cast<int> (plot.getWidth () / 2);
	for (int c = 1; c >= 0; --c)
	{
		for (int v = 0; v < s.voices; ++v)
		{
			auto path = owned (ctx->createGraphicsPath ());
			if (!path)
				continue;
			for (int i = 0; i <= steps; ++i)
			{
				const double m = double (i) / steps;
				const double ms = voiceDelayMs (s.shape, s.delay, s.depth, v, s.voices, voicePhase (m, v, s.voices, c, s.spread));
				if (i == 0)
					path->beginSubpath (toX (m), toY (ms));
				else
					path->addLine (toX (m), toY (ms));
			}
			const CColor base = c == 0 ? colors::accent : kRightColor;
			ctx->setFrameColor (withAlpha (base, static_cast<uint8_t> (230 - 35 * v)));
			ctx->setLineWidth (c == 0 ? 2.0 : 1.5);
			if (c == 1)
				dashed (ctx);
			ctx->drawGraphicsPath (path, CDrawContext::kPathStroked);
			ctx->setLineStyle (kLineSolid);
		}
	}

	// Playhead and the voices' current positions
	const double px = toX (playhead);
	ctx->setFrameColor (withAlpha (colors::text, 120));
	ctx->setLineWidth (1.0);
	ctx->drawLine (CPoint (px, plot.top), CPoint (px, plot.bottom));
	for (int c = 0; c < 2; ++c)
		for (int v = 0; v < s.voices; ++v)
		{
			const double ms = voiceDelayMs (s.shape, s.delay, s.depth, v, s.voices,
			                                voicePhase (playhead, v, s.voices, c, s.spread));
			const double y = toY (ms);
			ctx->setFillColor (c == 0 ? colors::accent : kRightColor);
			ctx->drawEllipse (CRect (px - 4, y - 4, px + 4, y + 4), kDrawFilled);
		}
	setDirty (false);
}

CMouseEventResult LfoView::onMouseDown (CPoint& where, const CButtonState& buttons)
{
	if (!buttons.isLeftButton ())
		return kMouseEventNotHandled;
	if (buttons.isDoubleClick ())
	{
		ctrl->editNormalized (kRate, ctrl->spec (kRate)->defaultNormalized ());
		ctrl->editNormalized (kDepth, ctrl->spec (kDepth)->defaultNormalized ());
		return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
	}
	dragging = true;
	lastMouse = where;
	dragDepth = ctrl->getPlain (kDepth);
	dragLogRate = std::log (ctrl->getPlain (kRate));
	ctrl->beginEdit (kDepth);
	ctrl->beginEdit (kRate);
	return kMouseEventHandled;
}

CMouseEventResult LfoView::onMouseMoved (CPoint& where, const CButtonState& buttons)
{
	if (!dragging)
		return kMouseEventNotHandled;
	const double fine = buttons.isShiftSet () ? 0.15 : 1.0;
	dragDepth = std::clamp (dragDepth - (where.y - lastMouse.y) * fine * 0.4, 0.0, 100.0);
	dragLogRate = std::clamp (dragLogRate + (where.x - lastMouse.x) * fine * 0.01, std::log (0.05), std::log (10.0));
	lastMouse = where;
	performEdit (ctrl, kDepth, dragDepth);
	performEdit (ctrl, kRate, std::exp (dragLogRate));
	return kMouseEventHandled;
}

void LfoView::endDrag ()
{
	if (!dragging)
		return;
	dragging = false;
	ctrl->endEdit (kDepth);
	ctrl->endEdit (kRate);
}

CMouseEventResult LfoView::onMouseUp (CPoint&, const CButtonState&)
{
	endDrag ();
	return kMouseEventHandled;
}

CMouseEventResult LfoView::onMouseCancel ()
{
	endDrag ();
	return kMouseEventHandled;
}

void LfoView::onMouseWheelEvent (MouseWheelEvent& event)
{
	if (event.deltaY == 0.0)
		return;
	const double factor = event.modifiers.has (ModifierKey::Shift) ? 1.01 : 1.05;
	const double d = ctrl->getPlain (kDelay) * (event.deltaY > 0 ? factor : 1.0 / factor);
	ctrl->editPlain (kDelay, std::clamp (d, kMinDelayMs, kMaxDelayMs));
	event.consumed = true;
}

// --- ResponseView -----------------------------------------------------------

namespace {
constexpr double kLoHz = 20.0;
constexpr double kHiHz = 20000.0;
constexpr double kTopDb = 12.0;
constexpr double kBottomDb = -30.0;
constexpr double kSampleRate = 48000.0;

std::complex<double> bandResponse (const suite::dsp::BandDesign& d, double freq)
{
	const double w = std::tan (kPi * std::min (freq, kSampleRate * 0.4999) / kSampleRate);
	std::complex<double> h (d.outputScale, 0.0);
	for (int i = 0; i < d.numSections; ++i)
		h *= d.sections[i].response (w);
	if (d.hasOnePole)
		h *= d.onePole.response (w);
	return h;
}
} // namespace

ResponseView::ResponseView (const CRect& r, ChoController* c) : CView (r), ctrl (c) {}

void ResponseView::draw (CDrawContext* ctx)
{
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	const CRect r = getViewSize ();
	fillRoundRect (ctx, r, 8, colors::panel);
	drawText (ctx, "RESPONSE NOW", CRect (r.left + 12, r.top + 8, r.left + 160, r.top + 22), 10, colors::textFaint, 0, true);
	drawText (ctx, "dry + wet, L solid / R dashed", CRect (r.right - 240, r.top + 8, r.right - 12, r.top + 22), 10,
	          colors::textFaint, 2);

	CRect plot = r;
	plot.left += 34;
	plot.right -= 12;
	plot.top += 30;
	plot.bottom -= 22;
	auto toX = [&] (double f) { return plot.left + plot.getWidth () * std::log (f / kLoHz) / std::log (kHiHz / kLoHz); };
	auto toY = [&] (double db) {
		return plot.top + plot.getHeight () * std::clamp ((kTopDb - db) / (kTopDb - kBottomDb), 0.0, 1.0);
	};

	ctx->setLineWidth (1.0);
	char buf[16];
	for (double db = 12.0; db >= kBottomDb; db -= 6.0)
	{
		const double y = std::round (toY (db)) + 0.5;
		ctx->setFrameColor (db == 0.0 ? colors::gridStrong : colors::grid);
		ctx->drawLine (CPoint (plot.left, y), CPoint (plot.right, y));
		if (std::fmod (std::abs (db), 12.0) < 1e-9)
		{
			std::snprintf (buf, sizeof (buf), "%+g", db);
			drawText (ctx, db == 0.0 ? "0" : buf, CRect (plot.left - 34, y - 7, plot.left - 4, y + 7), 10,
			          colors::textFaint, 2);
		}
	}
	for (auto [f, t] : {std::pair<double, const char*> {100, "100"}, {1000, "1k"}, {10000, "10k"}})
	{
		const double x = std::round (toX (f)) + 0.5;
		ctx->setFrameColor (colors::grid);
		ctx->drawLine (CPoint (x, plot.top), CPoint (x, plot.bottom));
		drawText (ctx, t, CRect (x - 16, plot.bottom + 4, x + 16, plot.bottom + 18), 10, colors::textFaint);
	}

	const auto s = currentSettings (ctrl);
	const double mix = ctrl->getPlain (kMix) / 100.0;
	const double dryGain = 1.0 - mix; // linear mix, as in the processor
	const double wetGain = mix;
	const double lc = ctrl->getPlain (kLowCut), hc = ctrl->getPlain (kHighCut);
	const auto lo = suite::dsp::designBand (suite::dsp::FilterType::LowCut, lc, 0.707, 0, 1, kSampleRate);
	const auto hi = suite::dsp::designBand (suite::dsp::FilterType::HighCut, hc, 0.707, 0, 1, kSampleRate);

	ConcatClip clip (*ctx, plot);
	for (int c = 1; c >= 0; --c)
	{
		double delays[kMaxVoices];
		for (int v = 0; v < s.voices; ++v)
			delays[v] = voiceDelayMs (s.shape, s.delay, s.depth, v, s.voices, voicePhase (phase, v, s.voices, c, s.spread)) * 0.001;
		auto path = owned (ctx->createGraphicsPath ());
		if (!path)
			continue;
		auto response = [&] (double f) {
			// A = mean of the voice delays; line = 1 / (1 - fb A); wet = A * line
			std::complex<double> a (0.0, 0.0);
			for (int v = 0; v < s.voices; ++v)
				a += std::polar (1.0, -2.0 * kPi * f * delays[v]);
			a /= double (s.voices);
			std::complex<double> wet = a / (1.0 - s.feedback * a);
			if (lc > kLowCutOffHz)
				wet *= bandResponse (lo, f);
			if (hc < kHighCutOffHz)
				wet *= bandResponse (hi, f);
			return std::norm (dryGain + wetGain * wet);
		};
		// Power-average across each pixel's frequency span, so combs denser than
		// the pixels show their average level instead of aliasing into noise.
		constexpr int kSub = 8;
		const int steps = static_cast<int> (plot.getWidth ());
		const double ratio = std::pow (kHiHz / kLoHz, 1.0 / steps);
		for (int i = 0; i <= steps; ++i)
		{
			const double f = kLoHz * std::pow (kHiHz / kLoHz, double (i) / steps);
			double power = 0.0;
			for (int k = 0; k < kSub; ++k)
				power += response (f * std::pow (ratio, (k + 0.5) / kSub - 0.5));
			const double db = 10.0 * std::log10 (std::max (power / kSub, 1e-12));
			if (i == 0)
				path->beginSubpath (plot.left + i, toY (db));
			else
				path->addLine (plot.left + i, toY (db));
		}
		ctx->setFrameColor (c == 0 ? colors::accent : kRightColor);
		ctx->setLineWidth (c == 0 ? 2.0 : 1.5);
		if (c == 1)
			dashed (ctx);
		ctx->drawGraphicsPath (path, CDrawContext::kPathStroked);
		ctx->setLineStyle (kLineSolid);
	}
	setDirty (false);
}

} // namespace cho
