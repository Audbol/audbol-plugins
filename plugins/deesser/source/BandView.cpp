#include "BandView.h"

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

namespace dess {

namespace {
constexpr double kLoHz = 500.0;
constexpr double kHiHz = 20000.0;
constexpr double kGainTopDb = 6.0; // reduction curve scale
constexpr double kGainBottomDb = -24.0;
constexpr double kSpectrumTopDb = 0.0;
constexpr double kSpectrumBottomDb = -96.0;
constexpr double kDisplayRate = 48000.0; // filter shape is drawn at a fixed rate
constexpr int kPixelsPerColumn = 2;
} // namespace

BandView::BandView (const CRect& r, DessController* c) : CView (r), ctrl (c) { layout (); }

void BandView::setViewSize (const CRect& rect, bool inv)
{
	CView::setViewSize (rect, inv);
	layout ();
}

void BandView::layout ()
{
	plot = getViewSize ();
	plot.left += 10;
	plot.right -= 34;
	plot.top += 30;
	plot.bottom -= 22;
	const int columns = std::max (2, static_cast<int> (plot.getWidth () / kPixelsPerColumn));
	columnFreqs.resize (columns);
	for (int i = 0; i < columns; ++i)
		columnFreqs[i] = static_cast<float> (kLoHz * std::pow (kHiHz / kLoHz, i / double (columns - 1)));
	spectrumDb.clear ();
}

double BandView::freqToX (double f) const
{
	return plot.left + plot.getWidth () * std::log (f / kLoHz) / std::log (kHiHz / kLoHz);
}

double BandView::gainToY (double db) const
{
	const double t = (kGainTopDb - db) / (kGainTopDb - kGainBottomDb);
	return plot.top + plot.getHeight () * std::clamp (t, 0.0, 1.0);
}

double BandView::spectrumToY (double db) const
{
	const double t = (kSpectrumTopDb - db) / (kSpectrumTopDb - kSpectrumBottomDb);
	return plot.top + plot.getHeight () * std::clamp (t, 0.0, 1.0);
}

double BandView::responseDb (double freq, double gr) const
{
	const auto filter = static_cast<Filter> (static_cast<int> (ctrl->getPlain (kFilter)));
	const auto d = designDetector (filter, ctrl->getPlain (kFrequency), ctrl->getPlain (kQ), kDisplayRate);
	const double w = std::tan (suite::dsp::kPi * std::min (freq, kDisplayRate * 0.4999) / kDisplayRate);
	std::complex<double> h (d.outputScale, 0.0);
	for (int i = 0; i < d.numSections; ++i)
		h *= d.sections[i].response (w);
	const double g = std::pow (10.0, -gr / 20.0);
	const bool split = static_cast<Mode> (static_cast<int> (ctrl->getPlain (kMode))) == Mode::Split;
	const double mag = split ? std::abs (1.0 + (g - 1.0) * h) : g;
	return 20.0 * std::log10 (std::max (mag, 1e-6));
}

void BandView::update (float gr, float dt)
{
	if (std::abs (gr - grDb) > 0.02f)
		grDb = gr;
	ctrl->analyzer ().compute (columnFreqs, spectrumDb, dt);
	invalid ();
}

void BandView::draw (CDrawContext* ctx)
{
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	fillRoundRect (ctx, getViewSize (), 8, colors::panel);

	const double freq = ctrl->getPlain (kFrequency);
	const double q = ctrl->getPlain (kQ);
	const double range = ctrl->getPlain (kRange);
	const auto filter = static_cast<Filter> (static_cast<int> (ctrl->getPlain (kFilter)));
	const bool listen = ctrl->getPlain (kListen) > 0.5;

	// Header and readout
	char buf[64];
	const CRect head = getViewSize ();
	drawText (ctx, "DETECTION", CRect (head.left + 12, head.top + 8, head.left + 120, head.top + 22), 10,
	          colors::textFaint, 0, true);
	if (filter == Filter::Band)
		std::snprintf (buf, sizeof (buf), "%.2f kHz   Q %.2f", freq / 1000.0, q);
	else
		std::snprintf (buf, sizeof (buf), "above %.2f kHz", freq / 1000.0);
	drawText (ctx, buf, CRect (head.right - 200, head.top + 8, head.right - 12, head.top + 22), 11, colors::text,
	          2);

	// Grid
	ctx->setLineWidth (1.0);
	for (double decade = 100.0; decade <= 10000.0; decade *= 10.0)
		for (int m = 1; m <= 9; ++m)
		{
			const double f = decade * m;
			if (f < kLoHz || f > kHiHz)
				continue;
			const double x = std::round (freqToX (f)) + 0.5;
			ctx->setFrameColor (m == 1 ? colors::gridStrong : colors::grid);
			ctx->drawLine (CPoint (x, plot.top), CPoint (x, plot.bottom));
		}
	static const std::pair<double, const char*> labels[] = {
	    {500, "500"}, {1000, "1k"}, {2000, "2k"}, {5000, "5k"}, {10000, "10k"}, {20000, "20k"}};
	for (auto& [f, text] : labels)
	{
		const double x = freqToX (f);
		drawText (ctx, text, CRect (x - 20, plot.bottom + 4, x + 20, plot.bottom + 18), 10, colors::textFaint);
	}
	for (double db = 0.0; db >= kGainBottomDb; db -= 6.0)
	{
		const double y = std::round (gainToY (db)) + 0.5;
		ctx->setFrameColor (db == 0.0 ? colors::gridStrong : colors::grid);
		ctx->drawLine (CPoint (plot.left, y), CPoint (plot.right, y));
		std::snprintf (buf, sizeof (buf), "%g", db);
		drawText (ctx, buf, CRect (plot.right + 4, y - 7, plot.right + 32, y + 7), 10,
		          db == 0.0 ? colors::textDim : colors::textFaint, 0);
	}

	ConcatClip clip (*ctx, plot);
	const size_t n = columnFreqs.size ();
	const double w = plot.getWidth ();

	// Input spectrum
	if (spectrumDb.size () == n)
	{
		auto area = owned (ctx->createGraphicsPath ());
		if (area)
		{
			area->beginSubpath (plot.left, plot.bottom);
			for (size_t i = 0; i < n; ++i)
				area->addLine (plot.left + w * i / (n - 1), spectrumToY (spectrumDb[i]));
			area->addLine (plot.right, plot.bottom);
			area->closeSubpath ();
			ctx->setFillColor (withAlpha (colors::textDim, 34));
			ctx->drawGraphicsPath (area, CDrawContext::kPathFilled);
		}
	}

	// Detection filter shape, hanging from 0 dB: the part of the signal the
	// sidechain listens to (and, in Split mode, the part that is turned down).
	const double zeroY = gainToY (0.0);
	if (auto shape = owned (ctx->createGraphicsPath ()))
	{
		const auto d = designDetector (filter, freq, q, kDisplayRate);
		shape->beginSubpath (plot.left, zeroY);
		for (size_t i = 0; i < n; ++i)
		{
			const double mag = d.magnitude (columnFreqs[i], kDisplayRate);
			// 0 at the band's edges, full depth (range) at its peak
			shape->addLine (plot.left + w * i / (n - 1), gainToY (-range * std::min (mag, 1.0)));
		}
		shape->addLine (plot.right, zeroY);
		shape->closeSubpath ();
		ctx->setFillColor (withAlpha (listen ? colors::warning : colors::accent, 26));
		ctx->drawGraphicsPath (shape, CDrawContext::kPathFilled);
	}

	// Response at full range (dashed) and right now (solid)
	auto curve = [&] (double gr) {
		auto path = owned (ctx->createGraphicsPath ());
		if (path)
			for (size_t i = 0; i < n; ++i)
			{
				const CPoint p (plot.left + w * i / (n - 1), gainToY (responseDb (columnFreqs[i], gr)));
				if (i == 0)
					path->beginSubpath (p);
				else
					path->addLine (p);
			}
		return path;
	};
	if (!listen)
	{
		if (auto full = curve (range))
		{
			const CCoord dashes[] = {4.0, 4.0};
			ctx->setLineStyle (CLineStyle (CLineStyle::kLineCapButt, CLineStyle::kLineJoinMiter, 0.0, 2, dashes));
			ctx->setFrameColor (withAlpha (colors::accent, 110));
			ctx->setLineWidth (1.0);
			ctx->drawGraphicsPath (full, CDrawContext::kPathStroked);
		}
		if (auto live = curve (grDb))
		{
			ctx->setLineStyle (CLineStyle (CLineStyle::kLineCapRound, CLineStyle::kLineJoinRound));
			ctx->setFrameColor (grDb > 0.05f ? colors::warning : colors::accent);
			ctx->setLineWidth (2.5);
			ctx->drawGraphicsPath (live, CDrawContext::kPathStroked);
		}
		ctx->setLineStyle (kLineSolid);
	}

	// Frequency marker and handle
	const double fx = freqToX (freq);
	ctx->setFrameColor (withAlpha (colors::text, 70));
	ctx->setLineWidth (1.0);
	ctx->drawLine (CPoint (fx, plot.top), CPoint (fx, plot.bottom));
	{
		CRect h (fx - 6, zeroY - 6, fx + 6, zeroY + 6);
		ctx->setFillColor (dragging ? colors::text : colors::panelRaised);
		ctx->drawEllipse (h, kDrawFilled);
		ctx->setFrameColor (listen ? colors::warning : colors::accent);
		ctx->setLineWidth (2.0);
		ctx->drawEllipse (h, kDrawStroked);
	}
	if (listen)
		drawText (ctx, "LISTENING TO SIDECHAIN", CRect (plot.left, plot.bottom - 22, plot.right, plot.bottom - 6), 10,
		          colors::warning, 1, true);
	setDirty (false);
}

void BandView::perform (uint32_t id, double plain)
{
	if (auto s = ctrl->spec (id))
	{
		const double v = s->toNormalized (plain);
		ctrl->setParamNormalized (id, v);
		ctrl->performEdit (id, v);
	}
}

CMouseEventResult BandView::onMouseDown (CPoint& where, const CButtonState& buttons)
{
	if (!buttons.isLeftButton ())
		return kMouseEventNotHandled;
	if (buttons.isDoubleClick ())
	{
		ctrl->editNormalized (kFrequency, ctrl->spec (kFrequency)->defaultNormalized ());
		ctrl->editNormalized (kQ, ctrl->spec (kQ)->defaultNormalized ());
		invalid ();
		return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
	}
	dragging = true;
	lastMouse = where;
	dragLogFreq = std::log (ctrl->getPlain (kFrequency));
	dragLogQ = std::log (ctrl->getPlain (kQ));
	ctrl->beginEdit (kFrequency);
	ctrl->beginEdit (kQ);
	invalid ();
	return kMouseEventHandled;
}

CMouseEventResult BandView::onMouseMoved (CPoint& where, const CButtonState& buttons)
{
	if (!dragging)
		return kMouseEventNotHandled;
	const double fine = buttons.isShiftSet () ? 0.15 : 1.0;
	const double dx = (where.x - lastMouse.x) * fine;
	const double dy = (where.y - lastMouse.y) * fine;
	lastMouse = where;
	dragLogFreq += dx * std::log (kHiHz / kLoHz) / plot.getWidth ();
	dragLogFreq = std::clamp (dragLogFreq, std::log (2000.0), std::log (16000.0));
	dragLogQ = std::clamp (dragLogQ - dy * 0.01, std::log (0.5), std::log (6.0));
	perform (kFrequency, std::exp (dragLogFreq));
	perform (kQ, std::exp (dragLogQ));
	invalid ();
	return kMouseEventHandled;
}

void BandView::endDrag ()
{
	if (!dragging)
		return;
	dragging = false;
	ctrl->endEdit (kFrequency);
	ctrl->endEdit (kQ);
	invalid ();
}

CMouseEventResult BandView::onMouseUp (CPoint&, const CButtonState&)
{
	endDrag ();
	return kMouseEventHandled;
}

CMouseEventResult BandView::onMouseCancel ()
{
	endDrag ();
	return kMouseEventHandled;
}

void BandView::onMouseWheelEvent (MouseWheelEvent& event)
{
	if (event.deltaY == 0.0)
		return;
	const double step = event.modifiers.has (ModifierKey::Shift) ? 1.02 : 1.1;
	const double q = ctrl->getPlain (kQ) * (event.deltaY > 0 ? step : 1.0 / step);
	ctrl->editPlain (kQ, std::clamp (q, 0.5, 6.0));
	invalid ();
	event.consumed = true;
}

} // namespace dess
