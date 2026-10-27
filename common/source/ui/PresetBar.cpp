#include "suite/ui/PresetBar.h"

#include "suite/ui/Theme.h"

#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cframe.h"
#include "vstgui/lib/cgraphicspath.h"
#include "vstgui/lib/controls/coptionmenu.h"

#include <map>
#include <vector>

using namespace VSTGUI;

namespace suite::ui {

namespace {
constexpr CCoord kArrowWidth = 26;

void drawArrow (CDrawContext* ctx, CPoint c, bool left, CColor color)
{
	if (auto path = owned (ctx->createGraphicsPath ()))
	{
		const double d = left ? 1.0 : -1.0;
		path->beginSubpath (c.x + 2.5 * d, c.y - 4.5);
		path->addLine (c.x - 2.5 * d, c.y);
		path->addLine (c.x + 2.5 * d, c.y + 4.5);
		ctx->setFrameColor (color);
		ctx->setLineWidth (1.6);
		ctx->setLineStyle (CLineStyle (CLineStyle::kLineCapRound, CLineStyle::kLineJoinRound));
		ctx->drawGraphicsPath (path, CDrawContext::kPathStroked);
		ctx->setLineStyle (kLineSolid);
	}
}
} // namespace

PresetBar::PresetBar (const CRect& r, ControllerBase* c) : CView (r), ctrl (c) {}

void PresetBar::refresh ()
{
	const int p = ctrl->currentPreset ();
	const bool m = ctrl->presetModified ();
	if (p != shownPreset || m != shownModified)
		invalid ();
}

PresetBar::Zone PresetBar::zoneAt (const CPoint& p) const
{
	const CRect r = getViewSize ();
	if (!r.pointInside (p))
		return kNone;
	if (p.x < r.left + kArrowWidth)
		return kPrev;
	if (p.x > r.right - kArrowWidth)
		return kNext;
	return kName;
}

void PresetBar::draw (CDrawContext* ctx)
{
	ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
	const CRect r = getViewSize ();
	shownPreset = ctrl->currentPreset ();
	shownModified = ctrl->presetModified ();

	fillRoundRect (ctx, r, 6, colors::control);
	if (hover != kNone)
	{
		CRect h = r;
		if (hover == kPrev)
			h.right = h.left + kArrowWidth;
		else if (hover == kNext)
			h.left = h.right - kArrowWidth;
		else
		{
			h.left += kArrowWidth;
			h.right -= kArrowWidth;
		}
		fillRoundRect (ctx, h, 6, colors::controlHover);
	}
	ctx->setFrameColor (withAlpha (colors::background, 200));
	ctx->setLineWidth (1.0);
	ctx->drawLine (CPoint (r.left + kArrowWidth + 0.5, r.top + 5), CPoint (r.left + kArrowWidth + 0.5, r.bottom - 5));
	ctx->drawLine (CPoint (r.right - kArrowWidth - 0.5, r.top + 5), CPoint (r.right - kArrowWidth - 0.5, r.bottom - 5));
	drawArrow (ctx, CPoint (r.left + kArrowWidth / 2, r.getCenter ().y), true, colors::textDim);
	drawArrow (ctx, CPoint (r.right - kArrowWidth / 2, r.getCenter ().y), false, colors::textDim);

	CRect nameRect (r.left + kArrowWidth + 6, r.top, r.right - kArrowWidth - 6, r.bottom);
	const auto& list = ctrl->presets ();
	std::string name = shownPreset >= 0 ? list[shownPreset].name : "Default";
	if (shownModified)
		name += " *";
	drawText (ctx, name, nameRect, 12, colors::text, 1, shownPreset >= 0);
	setDirty (false);
}

void PresetBar::step (int delta)
{
	const int n = static_cast<int> (ctrl->presets ().size ());
	if (n == 0)
		return;
	const int current = ctrl->currentPreset ();
	const int next = current < 0 ? (delta > 0 ? 0 : n - 1) : (current + delta + n) % n;
	ctrl->applyPreset (next);
	invalid ();
}

void PresetBar::showMenu ()
{
	auto frame = getFrame ();
	if (!frame)
		return;
	const auto& list = ctrl->presets ();
	const int current = ctrl->currentPreset ();

	// One submenu per category, in the order categories first appear
	auto menu = makeOwned<COptionMenu> ();
	std::vector<std::string> order;
	std::map<std::string, SharedPointer<COptionMenu>> subs;
	for (auto& p : list)
		if (!subs.count (p.category))
		{
			order.push_back (p.category);
			subs[p.category] = makeOwned<COptionMenu> (CRect (), nullptr, -1, nullptr, nullptr, COptionMenu::kCheckStyle);
		}
	SharedPointer<PresetBar> self (this);
	for (int i = 0; i < static_cast<int> (list.size ()); ++i)
	{
		auto item = new CCommandMenuItem (CCommandMenuItem::Desc (list[i].name.c_str ()));
		item->setChecked (i == current);
		item->setActions ([self, i] (CCommandMenuItem*) {
			self->ctrl->applyPreset (i);
			self->invalid ();
		});
		subs[list[i].category]->addEntry (item);
	}
	for (auto& c : order)
		menu->addEntry (subs[c], c.c_str ());

	CPoint where (getViewSize ().left + kArrowWidth, getViewSize ().bottom);
	localToFrame (where);
	menu->popup (frame, where);
}

CMouseEventResult PresetBar::onMouseDown (CPoint& where, const CButtonState& buttons)
{
	if (!buttons.isLeftButton () || ctrl->presets ().empty ())
		return kMouseEventNotHandled;
	switch (zoneAt (where))
	{
		case kPrev: step (-1); break;
		case kNext: step (1); break;
		case kName: showMenu (); break;
		case kNone: break;
	}
	return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
}

CMouseEventResult PresetBar::onMouseMoved (CPoint& where, const CButtonState&)
{
	const Zone z = zoneAt (where);
	if (z != hover)
	{
		hover = z;
		invalid ();
	}
	return kMouseEventHandled;
}

CMouseEventResult PresetBar::onMouseExited (CPoint&, const CButtonState&)
{
	hover = kNone;
	invalid ();
	return kMouseEventHandled;
}

} // namespace suite::ui
