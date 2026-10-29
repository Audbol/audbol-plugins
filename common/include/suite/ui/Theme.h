#pragma once

#include "vstgui/lib/ccolor.h"
#include "vstgui/lib/cfont.h"
#include "vstgui/lib/crect.h"
#include "vstgui/lib/vstguifwd.h"

#include <string>

namespace suite::ui {

// Dark theme shared by every plug-in in the suite: neutral graphite surfaces
// with soft depth, one accent colour per plug-in.
namespace colors {
inline const VSTGUI::CColor background {15, 16, 19, 255};
inline const VSTGUI::CColor panel {23, 25, 29, 255};
inline const VSTGUI::CColor panelRaised {33, 36, 42, 255};
inline const VSTGUI::CColor control {37, 40, 47, 255};
inline const VSTGUI::CColor controlHover {47, 51, 60, 255};
inline const VSTGUI::CColor border {41, 44, 52, 255};
inline const VSTGUI::CColor grid {32, 35, 41, 255};
inline const VSTGUI::CColor gridStrong {52, 56, 65, 255};
inline const VSTGUI::CColor text {234, 236, 240, 255};
inline const VSTGUI::CColor textDim {146, 151, 162, 255};
inline const VSTGUI::CColor textFaint {94, 99, 110, 255};
// The plug-in's own colour; each plug-in sets it once (see setAccent).
inline VSTGUI::CColor accent {108, 170, 255, 255};
inline const VSTGUI::CColor warning {255, 196, 0, 255};
inline const VSTGUI::CColor danger {255, 92, 92, 255};
} // namespace colors

// Per-plug-in accent colours (each plug-in is its own module, so setting the
// accent affects only that plug-in).
namespace accents {
inline const VSTGUI::CColor eq {62, 207, 190, 255};
inline const VSTGUI::CColor compressor {108, 170, 255, 255};
inline const VSTGUI::CColor deesser {240, 122, 170, 255};
inline const VSTGUI::CColor limiter {255, 112, 92, 255};
inline const VSTGUI::CColor reverb {162, 142, 250, 255};
inline const VSTGUI::CColor delay {86, 200, 240, 255};
inline const VSTGUI::CColor saturation {255, 150, 72, 255};
inline const VSTGUI::CColor chorus {122, 218, 150, 255};
inline const VSTGUI::CColor utility {172, 186, 206, 255};
inline const VSTGUI::CColor loudness {192, 228, 92, 255};
inline const VSTGUI::CColor scope {250, 208, 96, 255};
inline const VSTGUI::CColor gate {200, 130, 245, 255};
inline const VSTGUI::CColor transient {250, 180, 130, 255};
inline const VSTGUI::CColor clipper {236, 72, 112, 255};
} // namespace accents

// Call before any widget is created (e.g. in the editor's constructor).
void setAccent (const VSTGUI::CColor& c);

VSTGUI::CColor bandColor (int index);
VSTGUI::CColor withAlpha (const VSTGUI::CColor& c, uint8_t alpha);
VSTGUI::CColor mix (const VSTGUI::CColor& a, const VSTGUI::CColor& b, float t);

// Bahnschrift (DIN-style, ships with Windows 10+), falling back to Segoe UI.
VSTGUI::SharedPointer<VSTGUI::CFontDesc> font (VSTGUI::CCoord size, bool bold = false);

void fillRoundRect (VSTGUI::CDrawContext* ctx, const VSTGUI::CRect& r, VSTGUI::CCoord radius,
                    const VSTGUI::CColor& fill);
// Vertical gradient fill, top colour to bottom colour.
void fillRoundRectGradient (VSTGUI::CDrawContext* ctx, const VSTGUI::CRect& r, VSTGUI::CCoord radius,
                            const VSTGUI::CColor& top, const VSTGUI::CColor& bottom);
// A panel surface with soft depth: gradient, hairline edge, top highlight.
void drawPanel (VSTGUI::CDrawContext* ctx, const VSTGUI::CRect& r, VSTGUI::CCoord radius = 8,
                const VSTGUI::CColor& fill = colors::panel);
void strokeRoundRect (VSTGUI::CDrawContext* ctx, const VSTGUI::CRect& r, VSTGUI::CCoord radius,
                      const VSTGUI::CColor& stroke, VSTGUI::CCoord width = 1.0);
void drawText (VSTGUI::CDrawContext* ctx, const std::string& text, const VSTGUI::CRect& r,
               VSTGUI::CCoord size, const VSTGUI::CColor& color, int align = 1 /*0 L,1 C,2 R*/,
               bool bold = false);

} // namespace suite::ui
