#include "EqGraph.h"

#include "suite/ui/Theme.h"

#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cframe.h"
#include "vstgui/lib/cgraphicspath.h"
#include "vstgui/lib/controls/coptionmenu.h"
#include "vstgui/lib/events.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace VSTGUI;
using namespace suite::ui;
using suite::dsp::FilterType;

namespace eq {

namespace {

constexpr double kLoHz = 20.0;
constexpr double kHiHz = 20000.0;
constexpr double kAnalyzerTopDb = 6.0;
constexpr double kAnalyzerBottomDb = -96.0;
constexpr double kNodeRadius = 7.0;
constexpr int kPixelsPerColumn = 2;

std::string freqLabel (double f)
{
	char buf[32];
	if (f >= 1000.0)
		std::snprintf (buf, sizeof (buf), f >= 10000.0 ? "%.1f kHz" : "%.2f kHz", f / 1000.0);
	else
		std::snprintf (buf, sizeof (buf), "%.0f Hz", f);
	return buf;
}

} // namespace

EqGraph::EqGraph (const CRect& r, EqController* c) : CView (r), ctrl (c)
{
	layout ();
}

void EqGraph::setViewSize (const CRect& rect, bool inv)
{
	CView::setViewSize (rect, inv);
	layout ();
}

void EqGraph::layout ()
{
	plot = getViewSize ();
	plot.left += 10;
	plot.right -= 34; // dB labels
	plot.top += 10;
	plot.bottom -= 22; // frequency labels

	const int columns = std::max (2, static_cast<int> (plot.getWidth () / kPixelsPerColumn));
	columnFreqs.resize (columns);
	for (int i = 0; i < columns; ++i)
		columnFreqs[i] = static_cast<float> (xToFreq (plot.left + plot.getWidth () * i / (columns - 1)));
	for (auto& v : bandDb)
		v.assign (columns, 0.0);
	sumDb.assign (columns, 0.0);
	preDb.clear ();
	postDb.clear ();
	lastSnapshot.clear (); // force curve rebuild
}

void EqGraph::setSelected (int band)
{
	band = std::clamp (band, 0, kNumBands - 1);
	if (band != selected)
	{
		selected = band;
		invalid ();
	}
}

// --- Coordinate mapping ---------------------------------------------------

double EqGraph::freqToX (double f) const
{
	return plot.left + plot.getWidth () * std::log (f / kLoHz) / std::log (kHiHz / kLoHz);
}

double EqGraph::xToFreq (double x) const
{
	return kLoHz * std::pow (kHiHz / kLoHz, (x - plot.left) / plot.getWidth ());
}

double EqGraph::dbToY (double db) const
{
	return plot.top + plot.getHeight () * (0.5 - db / (2.0 * rangeDb));
}

double EqGraph::yToDb (double y) const
{
	return (0.5 - (y - plot.top) / plot.getHeight ()) * 2.0 * rangeDb;
}

double EqGraph::analyzerDbToY (double db) const
{
	double t = (kAnalyzerTopDb - db) / (kAnalyzerTopDb - kAnalyzerBottomDb);
	return plot.top + plot.getHeight () * std::clamp (t, 0.0, 1.0);
}

// --- Model ------------------------------------------------------------------

bool EqGraph::readBands ()
{
	// Cheap change detection over every parameter that affects the curves.
	std::vector<double> snap;
	snap.reserve (kNumBands * 7 + 3);
	snap.push_back (ctrl->getParamNormalized (kGainScale));
	snap.push_back (ctrl->getParamNormalized (kDisplayRange));
	snap.push_back (ctrl->sampleRate ());
	for (int b = 0; b < kNumBands; ++b)
		for (uint32_t p = kBandOn; p <= kBandPlacement; ++p)
			snap.push_back (ctrl->getParamNormalized (bandParam (b, static_cast<BandParam> (p))));
	if (snap == lastSnapshot)
		return false;
	lastSnapshot = std::move (snap);

	rangeDb = kDisplayRanges[std::clamp (static_cast<int> (ctrl->getPlain (kDisplayRange)), 0, 3)];
	const double scale = ctrl->getPlain (kGainScale) / 100.0;
	curveSampleRate = ctrl->sampleRate ();
	for (int b = 0; b < kNumBands; ++b)
	{
		auto& v = bands[b];
		v.on = ctrl->getPlain (bandParam (b, kBandOn)) > 0.5;
		v.type = static_cast<FilterType> (static_cast<int> (ctrl->getPlain (bandParam (b, kBandType))));
		v.freq = ctrl->getPlain (bandParam (b, kBandFreq));
		v.gain = ctrl->getPlain (bandParam (b, kBandGain));
		v.q = ctrl->getPlain (bandParam (b, kBandQ));
		v.slope = static_cast<int> (ctrl->getPlain (bandParam (b, kBandSlope)));
		v.design = suite::dsp::designBand (v.type, v.freq, v.q, v.gain * scale, v.slope, curveSampleRate);
	}
	return true;
}

void EqGraph::rebuildCurves ()
{
	std::fill (sumDb.begin (), sumDb.end (), 0.0);
	for (int b = 0; b < kNumBands; ++b)
	{
		if (!bands[b].on)
			continue;
		for (size_t i = 0; i < columnFreqs.size (); ++i)
		{
			const double mag = bands[b].design.magnitude (columnFreqs[i], curveSampleRate);
			const double db = 20.0 * std::log10 (std::max (mag, 1e-6));
			bandDb[b][i] = db;
			sumDb[i] += db;
		}
	}
}

void EqGraph::refresh (float dt)
{
	bool dirty = false;
	if (readBands ())
	{
		rebuildCurves ();
		dirty = true;
	}

	auto mode = static_cast<AnalyzerMode> (static_cast<int> (ctrl->getPlain (kAnalyzerMode)));
	if (mode != analyzer)
	{
		analyzer = mode;
		preDb.clear ();
		postDb.clear ();
		dirty = true;
	}
	if (analyzer == AnalyzerMode::Pre || analyzer == AnalyzerMode::Both)
	{
		ctrl->preAnalyzer ().compute (columnFreqs, preDb, dt);
		dirty = true;
	}
	if (analyzer == AnalyzerMode::Post || analyzer == AnalyzerMode::Both)
	{
		ctrl->postAnalyzer ().compute (columnFreqs, postDb, dt);
		dirty = true;
	}
	if (dirty)
		invalid ();
}

CPoint EqGraph::nodePos (int b) const
{
	const auto& v = bands[b];
	double db = 0.0;
	if (suite::dsp::filterTypeHasGain (v.type))
		db = v.type == FilterType::TiltShelf ? v.gain * 0.5 : v.gain;
	else if (suite::dsp::filterTypeHasSlope (v.type))
		db = 20.0 * std::log10 (std::max (v.design.magnitude (v.freq, curveSampleRate), 1e-6));
	db = std::clamp (db, -rangeDb, rangeDb);
	return CPoint (freqToX (std::clamp (v.freq, kLoHz, kHiHz)), dbToY (db));
}

int EqGraph::hitNode (const CPoint& p) const
{
	int best = -1;
	double bestDist = kNodeRadius + 5.0;
	// Selected band wins ties so overlapping nodes stay reachable.
	for (int i = 0; i < kNumBands; ++i)
	{
		const int b = (selected + i) % kNumBands;
		if (!bands[b].on)
			continue;
		auto n = nodePos (b);
		const double d = std::hypot (n.x - p.x, n.y - p.y);
		if (d < bestDist)
		{
			bestDist = d;
			best = b;
		}
	}
	return best;
}

// --- Drawing ----------------------------------------------------------------

void EqGraph::draw (CDrawContext* ctx)
{
	if (lastSnapshot.empty ())
	{
		readBands ();
		rebuildCurves ();
	}
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	fillRoundRect (ctx, getViewSize (), 8, colors::panel);

	drawGrid (ctx);

	CRect clip = plot;
	ConcatClip cc (*ctx, clip);
	if (analyzer == AnalyzerMode::Pre || analyzer == AnalyzerMode::Both)
		drawAnalyzer (ctx, preDb, withAlpha (colors::textDim, 38), withAlpha (colors::textDim, 90));
	if (analyzer == AnalyzerMode::Post || analyzer == AnalyzerMode::Both)
		drawAnalyzer (ctx, postDb, withAlpha (colors::accent, 40), withAlpha (colors::accent, 120));
	drawCurves (ctx);
	drawNodes (ctx);

	const int tip = dragging >= 0 ? dragging : hover;
	if (tip >= 0 && bands[tip].on)
		drawTooltip (ctx, tip);

	bool any = false;
	for (auto& b : bands)
		any = any || b.on;
	if (!any)
	{
		const double y = dbToY (0.0) - 26.0;
		drawText (ctx, "Double-click anywhere to add a band", CRect (plot.left, y, plot.right, y + 18), 13,
		          colors::textDim);
	}

	setDirty (false);
}

void EqGraph::drawGrid (CDrawContext* ctx)
{
	ctx->setLineWidth (1.0);
	ctx->setLineStyle (kLineSolid);

	// Frequency lines: 1-9 x each decade.
	for (double decade = 10.0; decade <= 10000.0; decade *= 10.0)
	{
		for (int m = 1; m <= 9; ++m)
		{
			const double f = decade * m;
			if (f < kLoHz || f > kHiHz)
				continue;
			const double x = std::round (freqToX (f)) + 0.5;
			ctx->setFrameColor (m == 1 ? colors::gridStrong : colors::grid);
			ctx->drawLine (CPoint (x, plot.top), CPoint (x, plot.bottom));
		}
	}
	static const std::pair<double, const char*> labels[] = {
	    {20, "20"},  {50, "50"},  {100, "100"}, {200, "200"}, {500, "500"},
	    {1000, "1k"}, {2000, "2k"}, {5000, "5k"}, {10000, "10k"}, {20000, "20k"}};
	for (auto& [f, text] : labels)
	{
		const double x = freqToX (f);
		CRect r (x - 20, plot.bottom + 4, x + 20, plot.bottom + 18);
		drawText (ctx, text, r, 10, colors::textFaint);
	}

	// Gain lines.
	const double step = rangeDb / 4.0;
	for (int i = -4; i <= 4; ++i)
	{
		const double db = i * step;
		const double y = std::round (dbToY (db)) + 0.5;
		ctx->setFrameColor (i == 0 ? colors::gridStrong : colors::grid);
		ctx->drawLine (CPoint (plot.left, y), CPoint (plot.right, y));
		if (i == -4 || i == 4)
			continue;
		char buf[16];
		std::snprintf (buf, sizeof (buf), i > 0 ? "+%g" : "%g", db);
		CRect r (plot.right + 4, y - 7, plot.right + 32, y + 7);
		drawText (ctx, buf, r, 10, i == 0 ? colors::textDim : colors::textFaint, 0);
	}
}

void EqGraph::drawAnalyzer (CDrawContext* ctx, const std::vector<float>& db, CColor fill, CColor line)
{
	if (db.size () != columnFreqs.size () || db.empty ())
		return;
	auto area = owned (ctx->createGraphicsPath ());
	auto edge = owned (ctx->createGraphicsPath ());
	if (!area || !edge)
		return;
	const double w = plot.getWidth ();
	const size_t n = db.size ();
	area->beginSubpath (plot.left, plot.bottom);
	for (size_t i = 0; i < n; ++i)
	{
		const double x = plot.left + w * i / (n - 1);
		const double y = analyzerDbToY (db[i]);
		area->addLine (x, y);
		if (i == 0)
			edge->beginSubpath (x, y);
		else
			edge->addLine (x, y);
	}
	area->addLine (plot.right, plot.bottom);
	area->closeSubpath ();
	ctx->setFillColor (fill);
	ctx->drawGraphicsPath (area, CDrawContext::kPathFilled);
	ctx->setFrameColor (line);
	ctx->setLineWidth (1.0);
	ctx->drawGraphicsPath (edge, CDrawContext::kPathStroked);
}

void EqGraph::drawCurves (CDrawContext* ctx)
{
	const size_t n = columnFreqs.size ();
	const double w = plot.getWidth ();
	const double zeroY = dbToY (0.0);
	auto xAt = [&] (size_t i) { return plot.left + w * i / (n - 1); };
	auto yAt = [&] (double db) {
		return std::clamp (dbToY (db), plot.top - 2.0, plot.bottom + 2.0);
	};

	for (int b = 0; b < kNumBands; ++b)
	{
		if (!bands[b].on)
			continue;
		const CColor color = bandColor (b);
		const bool focus = b == selected || b == hover || b == dragging;
		if (focus)
		{
			if (auto fill = owned (ctx->createGraphicsPath ()))
			{
				fill->beginSubpath (xAt (0), zeroY);
				for (size_t i = 0; i < n; ++i)
					fill->addLine (xAt (i), yAt (bandDb[b][i]));
				fill->addLine (xAt (n - 1), zeroY);
				fill->closeSubpath ();
				ctx->setFillColor (withAlpha (color, 40));
				ctx->drawGraphicsPath (fill, CDrawContext::kPathFilled);
			}
		}
		if (auto line = owned (ctx->createGraphicsPath ()))
		{
			line->beginSubpath (xAt (0), yAt (bandDb[b][0]));
			for (size_t i = 1; i < n; ++i)
				line->addLine (xAt (i), yAt (bandDb[b][i]));
			ctx->setFrameColor (withAlpha (color, focus ? 220 : 110));
			ctx->setLineWidth (focus ? 1.5 : 1.0);
			ctx->drawGraphicsPath (line, CDrawContext::kPathStroked);
		}
	}

	if (auto sum = owned (ctx->createGraphicsPath ()))
	{
		sum->beginSubpath (xAt (0), yAt (sumDb[0]));
		for (size_t i = 1; i < n; ++i)
			sum->addLine (xAt (i), yAt (sumDb[i]));
		ctx->setFrameColor (colors::text);
		ctx->setLineWidth (2.2);
		ctx->setLineStyle (CLineStyle (CLineStyle::kLineCapRound, CLineStyle::kLineJoinRound));
		ctx->drawGraphicsPath (sum, CDrawContext::kPathStroked);
	}
}

void EqGraph::drawNodes (CDrawContext* ctx)
{
	for (int i = kNumBands - 1; i >= 0; --i)
	{
		// Draw the selected node last so it sits on top.
		const int b = (selected + 1 + i) % kNumBands;
		if (!bands[b].on)
			continue;
		const CPoint p = nodePos (b);
		const bool isSel = b == selected;
		const bool isHot = b == hover || b == dragging;
		const CColor color = bandColor (b);
		const double r = isSel ? kNodeRadius + 1.0 : kNodeRadius;

		if (isHot)
		{
			CRect halo (p.x - r - 5, p.y - r - 5, p.x + r + 5, p.y + r + 5);
			ctx->setFillColor (withAlpha (color, 50));
			ctx->drawEllipse (halo, kDrawFilled);
		}
		CRect node (p.x - r, p.y - r, p.x + r, p.y + r);
		ctx->setFillColor (isSel || isHot ? color : withAlpha (color, 170));
		ctx->drawEllipse (node, kDrawFilled);
		if (isSel)
		{
			ctx->setFrameColor (colors::text);
			ctx->setLineWidth (1.5);
			ctx->drawEllipse (node, kDrawStroked);
		}
		drawText (ctx, std::to_string (b + 1), node, 9, colors::background, 1, true);
	}
}

void EqGraph::drawTooltip (CDrawContext* ctx, int b)
{
	const auto& v = bands[b];
	char buf[96];
	if (suite::dsp::filterTypeHasGain (v.type))
		std::snprintf (buf, sizeof (buf), "%s   %+.1f dB   Q %.2f", freqLabel (v.freq).c_str (), v.gain, v.q);
	else if (suite::dsp::filterTypeHasSlope (v.type))
		std::snprintf (buf, sizeof (buf), "%s   %d dB/oct   Q %.2f", freqLabel (v.freq).c_str (),
		               suite::dsp::kSlopeOrders[std::clamp (v.slope, 0, 7)] * 6, v.q);
	else
		std::snprintf (buf, sizeof (buf), "%s   Q %.2f", freqLabel (v.freq).c_str (), v.q);

	ctx->setFont (font (11));
	const double w = ctx->getStringWidth (buf) + 18;
	const CPoint p = nodePos (b);
	double x = std::clamp (p.x - w * 0.5, plot.left + 4, plot.right - w - 4);
	double y = p.y - kNodeRadius - 30;
	if (y < plot.top + 4)
		y = p.y + kNodeRadius + 8;
	CRect box (x, y, x + w, y + 22);
	fillRoundRect (ctx, box, 5, withAlpha (colors::background, 235));
	strokeRoundRect (ctx, box, 5, withAlpha (bandColor (b), 160));
	drawText (ctx, buf, box, 11, colors::text);
}

// --- Editing ----------------------------------------------------------------

void EqGraph::perform (uint32_t id, double plain)
{
	if (auto s = ctrl->spec (id))
	{
		const double n = s->toNormalized (plain);
		ctrl->setParamNormalized (id, n);
		ctrl->performEdit (id, n);
	}
}

void EqGraph::addBandAt (const CPoint& p)
{
	int free = -1;
	for (int b = 0; b < kNumBands && free < 0; ++b)
		if (!bands[b].on)
			free = b;
	if (free < 0)
		return;
	const double f = std::clamp (xToFreq (p.x), kMinFreq, kMaxFreq);
	const double g = std::clamp (yToDb (p.y), -kMaxGain, kMaxGain);
	ctrl->editPlain (bandParam (free, kBandType), static_cast<double> (FilterType::Bell));
	ctrl->editPlain (bandParam (free, kBandFreq), f);
	ctrl->editPlain (bandParam (free, kBandGain), std::round (g * 10.0) / 10.0);
	ctrl->editPlain (bandParam (free, kBandQ), 1.0);
	ctrl->editPlain (bandParam (free, kBandOn), 1.0);
	selected = free;
	if (onSelect)
		onSelect (free);
	lastSnapshot.clear ();
	refresh (0.0f);
}

CMouseEventResult EqGraph::onMouseDown (CPoint& where, const CButtonState& buttons)
{
	const int hit = hitNode (where);
	if (buttons.isRightButton ())
	{
		if (hit >= 0)
		{
			setSelected (hit);
			if (onSelect)
				onSelect (hit);
			showMenu (hit, where);
		}
		return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
	}
	if (!buttons.isLeftButton ())
		return kMouseEventNotHandled;

	if (hit < 0)
	{
		if (buttons.isDoubleClick ())
			addBandAt (where);
		return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
	}

	setSelected (hit);
	if (onSelect)
		onSelect (hit);
	if (buttons.isDoubleClick ())
	{
		ctrl->editPlain (bandParam (hit, kBandOn), 0.0);
		hover = -1;
		return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
	}

	dragging = hit;
	lastMouse = where;
	dragLogFreq = std::log (bands[hit].freq);
	dragGain = bands[hit].gain;
	dragLogQ = std::log (bands[hit].q);
	ctrl->beginEdit (bandParam (hit, kBandFreq));
	ctrl->beginEdit (bandParam (hit, kBandGain));
	ctrl->beginEdit (bandParam (hit, kBandQ));
	invalid ();
	return kMouseEventHandled;
}

CMouseEventResult EqGraph::onMouseMoved (CPoint& where, const CButtonState& buttons)
{
	if (dragging < 0)
	{
		const int h = hitNode (where);
		if (h != hover)
		{
			hover = h;
			invalid ();
		}
		return kMouseEventHandled;
	}

	const double fine = buttons.isShiftSet () ? 0.12 : 1.0;
	const double dx = (where.x - lastMouse.x) * fine;
	const double dy = (where.y - lastMouse.y) * fine;
	lastMouse = where;

	const auto& v = bands[dragging];
	dragLogFreq += dx * std::log (kHiHz / kLoHz) / plot.getWidth ();
	dragLogFreq = std::clamp (dragLogFreq, std::log (kMinFreq), std::log (kMaxFreq));
	perform (bandParam (dragging, kBandFreq), std::exp (dragLogFreq));

	if (suite::dsp::filterTypeHasGain (v.type))
	{
		double gainPerPixel = 2.0 * rangeDb / plot.getHeight ();
		if (v.type == FilterType::TiltShelf)
			gainPerPixel *= 2.0; // node sits at half the tilt
		dragGain = std::clamp (dragGain - dy * gainPerPixel, -kMaxGain, kMaxGain);
		perform (bandParam (dragging, kBandGain), dragGain);
	}
	else if (suite::dsp::filterTypeHasSlope (v.type))
	{
		// Vertical drag on cut filters sets the resonance.
		dragLogQ = std::clamp (dragLogQ - dy * 0.02, std::log (kMinQ), std::log (kMaxQ));
		perform (bandParam (dragging, kBandQ), std::exp (dragLogQ));
	}
	refresh (0.0f);
	return kMouseEventHandled;
}

void EqGraph::endDrag ()
{
	if (dragging < 0)
		return;
	ctrl->endEdit (bandParam (dragging, kBandFreq));
	ctrl->endEdit (bandParam (dragging, kBandGain));
	ctrl->endEdit (bandParam (dragging, kBandQ));
	dragging = -1;
	invalid ();
}

CMouseEventResult EqGraph::onMouseUp (CPoint&, const CButtonState&)
{
	endDrag ();
	return kMouseEventHandled;
}

CMouseEventResult EqGraph::onMouseCancel ()
{
	endDrag ();
	return kMouseEventHandled;
}

CMouseEventResult EqGraph::onMouseExited (CPoint&, const CButtonState&)
{
	if (hover >= 0)
	{
		hover = -1;
		invalid ();
	}
	return kMouseEventHandled;
}

void EqGraph::onMouseWheelEvent (MouseWheelEvent& event)
{
	int b = hitNode (event.mousePosition);
	if (b < 0)
		b = bands[selected].on ? selected : -1;
	if (b < 0 || event.deltaY == 0.0)
		return;
	const double factor = event.modifiers.has (ModifierKey::Shift) ? 1.02 : 1.12;
	const double q = bands[b].q * (event.deltaY > 0 ? factor : 1.0 / factor);
	ctrl->editPlain (bandParam (b, kBandQ), std::clamp (q, kMinQ, kMaxQ));
	refresh (0.0f);
	event.consumed = true;
}

void EqGraph::showMenu (int b, const CPoint& where)
{
	auto frame = getFrame ();
	if (!frame)
		return;
	SharedPointer<EqGraph> self (this);
	auto menu = makeOwned<COptionMenu> ();

	auto addItems = [&] (COptionMenu* m, uint32_t id) {
		auto s = ctrl->spec (id);
		const int current = static_cast<int> (ctrl->getPlain (id));
		for (size_t i = 0; i < s->choices.size (); ++i)
		{
			auto item = new CCommandMenuItem (CCommandMenuItem::Desc (s->choices[i].c_str ()));
			item->setChecked (static_cast<int> (i) == current);
			item->setActions ([self, id, i] (CCommandMenuItem*) {
				self->ctrl->editPlain (id, static_cast<double> (i));
				self->refresh (0.0f);
			});
			m->addEntry (item);
		}
	};

	auto typeMenu = makeOwned<COptionMenu> (CRect (), nullptr, -1, nullptr, nullptr, COptionMenu::kCheckStyle);
	addItems (typeMenu, bandParam (b, kBandType));
	menu->addEntry (typeMenu, "Type");

	if (suite::dsp::filterTypeHasSlope (bands[b].type))
	{
		auto slopeMenu = makeOwned<COptionMenu> (CRect (), nullptr, -1, nullptr, nullptr, COptionMenu::kCheckStyle);
		addItems (slopeMenu, bandParam (b, kBandSlope));
		menu->addEntry (slopeMenu, "Slope");
	}

	auto placeMenu = makeOwned<COptionMenu> (CRect (), nullptr, -1, nullptr, nullptr, COptionMenu::kCheckStyle);
	addItems (placeMenu, bandParam (b, kBandPlacement));
	menu->addEntry (placeMenu, "Stereo Placement");

	menu->addSeparator ();
	auto reset = new CCommandMenuItem (CCommandMenuItem::Desc ("Reset Gain"));
	reset->setActions ([self, b] (CCommandMenuItem*) {
		self->ctrl->editPlain (bandParam (b, kBandGain), 0.0);
		self->refresh (0.0f);
	});
	menu->addEntry (reset);
	auto remove = new CCommandMenuItem (CCommandMenuItem::Desc ("Remove Band"));
	remove->setActions ([self, b] (CCommandMenuItem*) {
		self->ctrl->editPlain (bandParam (b, kBandOn), 0.0);
		self->refresh (0.0f);
	});
	menu->addEntry (remove);

	CPoint p = where;
	localToFrame (p);
	menu->popup (frame, p);
}

} // namespace eq
