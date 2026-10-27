#pragma once

#include "vstgui/lib/ccolor.h"
#include "vstgui/lib/cfont.h"
#include "vstgui/lib/crect.h"
#include "vstgui/lib/vstguifwd.h"

#include <string>

namespace suite::ui {

// Modern flat dark theme shared by every plug-in in the suite.
namespace colors {
inline const VSTGUI::CColor background {17, 19, 23, 255};
inline const VSTGUI::CColor panel {24, 27, 33, 255};
inline const VSTGUI::CColor panelRaised {33, 37, 45, 255};
inline const VSTGUI::CColor control {42, 47, 57, 255};
inline const VSTGUI::CColor controlHover {52, 58, 70, 255};
inline const VSTGUI::CColor border {48, 54, 64, 255};
inline const VSTGUI::CColor grid {36, 40, 48, 255};
inline const VSTGUI::CColor gridStrong {58, 64, 76, 255};
inline const VSTGUI::CColor text {230, 233, 239, 255};
inline const VSTGUI::CColor textDim {140, 148, 163, 255};
inline const VSTGUI::CColor textFaint {92, 99, 112, 255};
inline const VSTGUI::CColor accent {79, 195, 247, 255};
inline const VSTGUI::CColor warning {255, 196, 0, 255};
inline const VSTGUI::CColor danger {255, 92, 92, 255};
} // namespace colors

VSTGUI::CColor bandColor (int index);
VSTGUI::CColor withAlpha (const VSTGUI::CColor& c, uint8_t alpha);
VSTGUI::CColor mix (const VSTGUI::CColor& a, const VSTGUI::CColor& b, float t);

VSTGUI::SharedPointer<VSTGUI::CFontDesc> font (VSTGUI::CCoord size, bool bold = false);

void fillRoundRect (VSTGUI::CDrawContext* ctx, const VSTGUI::CRect& r, VSTGUI::CCoord radius,
                    const VSTGUI::CColor& fill);
void strokeRoundRect (VSTGUI::CDrawContext* ctx, const VSTGUI::CRect& r, VSTGUI::CCoord radius,
                      const VSTGUI::CColor& stroke, VSTGUI::CCoord width = 1.0);
void drawText (VSTGUI::CDrawContext* ctx, const std::string& text, const VSTGUI::CRect& r,
               VSTGUI::CCoord size, const VSTGUI::CColor& color, int align = 1 /*0 L,1 C,2 R*/,
               bool bold = false);

} // namespace suite::ui
