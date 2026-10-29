#include "TransientView.h"

#include "TransParams.h"

#include "suite/ui/Theme.h"

#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgraphicspath.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace VSTGUI;
using namespace suite::ui;

namespace trans {

namespace {
constexpr CCoord kColumnWidth = 2.0;
constexpr double kLevelRangeDb = 48.0;
constexpr float kFloorDb = -120.0f;
} // namespace

TransientView::TransientView (const CRect& r) : CView (r) { setViewSize (r, false); }

void TransientView::setViewSize (const CRect& rect, bool inv)
{
	CView::setViewSize (rect, inv);
	const auto n = static_cast<size_t> (std::max (2.0, levelRect ().getWidth () / kColumnWidth));
	columns.assign (n, {kFloorDb, kFloorDb, 0.0f, 0.0f});
	head = 0;
}

CRect TransientView::levelRect () const
{
	CRect p = getViewSize ();
	p.inset (10, 10);
	p.left += 24;
	p.top += 18;
	p.bottom = p.top + (p.bottom - p.top) * 0.62;
	return p;
}

CRect TransientView::gainRect () const
{
	CRect p = getViewSize ();
	p.inset (10, 10);
	p.left += 24;
	p.top = levelRect ().bottom + 22;
	return p;
}

void TransientView::push (float inDb, float outDb, float gainMaxDb, float gainMinDb)
{
	columns[head] = {inDb, outDb, gainMaxDb, gainMinDb};
	head = (head + 1) % columns.size ();
	invalid ();
}

void TransientView::draw (CDrawContext* ctx)
{
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	drawPanel (ctx, getViewSize ());
	const CRect lv = levelRect ();
	const CRect gr = gainRect ();
	const CRect box = getViewSize ();
	char buf[16];

	drawText (ctx, "Level", CRect (box.left + 12, box.top + 8, box.left + 120, box.top + 22), 11,
	          colors::textDim, 0, true);
	drawText (ctx, "Shaping", CRect (box.left + 12, lv.bottom + 6, box.left + 120, lv.bottom + 20), 11,
	          colors::textDim, 0, true);

	auto levelY = [&] (double db) { return lv.top + lv.getHeight () * std::clamp (-db / kLevelRangeDb, 0.0, 1.0); };
	const double gainMid = std::round ((gr.top + gr.bottom) * 0.5) + 0.5;
	auto gainY = [&] (double db) {
		return gainMid - gr.getHeight () * 0.5 * std::clamp (db / kMaxShapeDb, -1.0, 1.0);
	};

	// Grids
	ctx->setLineWidth (1.0);
	for (double db = 0.0; db >= -kLevelRangeDb; db -= 12.0)
	{
		const double y = std::round (levelY (db)) + 0.5;
		ctx->setFrameColor (db == 0.0 ? colors::gridStrong : colors::grid);
		ctx->drawLine (CPoint (lv.left, y), CPoint (lv.right, y));
		std::snprintf (buf, sizeof (buf), "%g", db);
		drawText (ctx, buf, CRect (lv.left - 30, y - 7, lv.left - 4, y + 7), 10, colors::textFaint, 2);
	}
	for (double db : {kMaxShapeDb, 0.0, -kMaxShapeDb})
	{
		const double y = db == 0.0 ? gainMid : std::round (gainY (db)) + 0.5;
		ctx->setFrameColor (db == 0.0 ? colors::gridStrong : colors::grid);
		ctx->drawLine (CPoint (gr.left, y), CPoint (gr.right, y));
		std::snprintf (buf, sizeof (buf), "%+g", db);
		drawText (ctx, db == 0.0 ? "0" : buf, CRect (gr.left - 30, y - 7, gr.left - 4, y + 7), 10,
		          colors::textFaint, 2);
	}

	const size_t n = columns.size ();
	auto colX = [&] (size_t i) { return lv.left + i * kColumnWidth; };
	auto at = [&] (size_t i) -> const Column& { return columns[(head + i) % n]; };

	{
		ConcatClip clip (*ctx, lv);
		if (auto area = owned (ctx->createGraphicsPath ()))
		{
			area->beginSubpath (colX (0), lv.bottom);
			for (size_t i = 0; i < n; ++i)
			{
				area->addLine (colX (i), levelY (at (i).in));
				area->addLine (colX (i) + kColumnWidth, levelY (at (i).in));
			}
			area->addLine (colX (n), lv.bottom);
			area->closeSubpath ();
			ctx->setFillColor (withAlpha (colors::textDim, 60));
			ctx->drawGraphicsPath (area, CDrawContext::kPathFilled);
		}
		if (auto line = owned (ctx->createGraphicsPath ()))
		{
			line->beginSubpath (colX (0), levelY (at (0).out));
			for (size_t i = 1; i < n; ++i)
				line->addLine (colX (i), levelY (at (i).out));
			ctx->setFrameColor (withAlpha (colors::accent, 220));
			ctx->setLineWidth (1.25);
			ctx->drawGraphicsPath (line, CDrawContext::kPathStroked);
		}
	}

	{
		ConcatClip clip (*ctx, gr);
		// One bar per column: boost above the centre line, cut below it
		for (size_t i = 0; i < n; ++i)
		{
			const auto& c = at (i);
			const CCoord x = colX (i);
			if (c.gainMax > 0.05f)
			{
				ctx->setFillColor (withAlpha (colors::accent, 200));
				ctx->drawRect (CRect (x, gainY (c.gainMax), x + kColumnWidth, gainMid), kDrawFilled);
			}
			if (c.gainMin < -0.05f)
			{
				ctx->setFillColor (withAlpha (colors::textDim, 170));
				ctx->drawRect (CRect (x, gainMid, x + kColumnWidth, gainY (c.gainMin)), kDrawFilled);
			}
		}
	}
	setDirty (false);
}

} // namespace trans
