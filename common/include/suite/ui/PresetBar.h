#pragma once

#include "suite/vst/ControllerBase.h"

#include "vstgui/lib/cview.h"

#include <string>

namespace suite::ui {

// Header preset selector: < name >. Arrows step through the factory
// presets, clicking the name opens a menu grouped by category. A trailing
// "*" shows the sound has been edited since the preset was loaded.
class PresetBar : public VSTGUI::CView
{
public:
	PresetBar (const VSTGUI::CRect& r, ControllerBase* controller);

	// Redraw if the shown preset or its modified state changed.
	void refresh ();

	void draw (VSTGUI::CDrawContext* ctx) override;
	VSTGUI::CMouseEventResult onMouseDown (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseMoved (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseExited (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;

private:
	enum Zone
	{
		kNone,
		kPrev,
		kName,
		kNext
	};
	Zone zoneAt (const VSTGUI::CPoint& p) const;
	void step (int delta);
	void showMenu ();

	ControllerBase* ctrl;
	Zone hover {kNone};
	int shownPreset {-2};
	bool shownModified {false};
};

} // namespace suite::ui
