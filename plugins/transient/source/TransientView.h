#pragma once

#include "vstgui/lib/cview.h"

#include <vector>

namespace trans {

// Scrolling history: input level (filled) and output level (line) on top,
// and below it the gain the shaper applied, boosts up and cuts down.
class TransientView : public VSTGUI::CView
{
public:
	explicit TransientView (const VSTGUI::CRect& r);

	void push (float inDb, float outDb, float gainMaxDb, float gainMinDb);

	void draw (VSTGUI::CDrawContext* ctx) override;
	void setViewSize (const VSTGUI::CRect& rect, bool invalid = true) override;

private:
	struct Column
	{
		float in, out, gainMax, gainMin;
	};

	VSTGUI::CRect levelRect () const;
	VSTGUI::CRect gainRect () const;

	std::vector<Column> columns; // ring buffer
	size_t head {0};
};

} // namespace trans
