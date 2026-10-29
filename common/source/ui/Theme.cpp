#include "suite/ui/Theme.h"

#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgradient.h"
#include "vstgui/lib/cgraphicspath.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <map>

using namespace VSTGUI;

namespace suite::ui {

CColor bandColor (int index)
{
	static const CColor palette[] = {
	    {255, 107, 107, 255}, {255, 169, 77, 255}, {255, 212, 59, 255},  {105, 219, 124, 255},
	    {56, 217, 169, 255},  {77, 171, 247, 255}, {151, 117, 250, 255}, {247, 131, 172, 255},
	};
	return palette[((index % 8) + 8) % 8];
}

CColor withAlpha (const CColor& c, uint8_t alpha) { return {c.red, c.green, c.blue, alpha}; }

CColor mix (const CColor& a, const CColor& b, float t)
{
	auto lerp = [t] (uint8_t x, uint8_t y) {
		return static_cast<uint8_t> (std::clamp (x + (y - x) * t, 0.0f, 255.0f));
	};
	return {lerp (a.red, b.red), lerp (a.green, b.green), lerp (a.blue, b.blue),
	        lerp (a.alpha, b.alpha)};
}

SharedPointer<CFontDesc> font (CCoord size, bool bold)
{
	// Deliberately leaked: releasing platform fonts during DLL unload (after
	// VSTGUI has shut down) can crash some hosts.
	static auto& cache = *new std::map<std::pair<int, bool>, SharedPointer<CFontDesc>> ();
	auto key = std::make_pair (static_cast<int> (size * 10), bold);
	auto it = cache.find (key);
	if (it != cache.end ())
		return it->second;
#if WINDOWS
	// Bahnschrift's SemiBold instance is a family of its own in DirectWrite.
	static const bool bahnschrift = [] {
		const char* windir = std::getenv ("WINDIR");
		return std::filesystem::exists (std::filesystem::path (windir ? windir : "C:\\Windows") / "Fonts" /
		                                "bahnschrift.ttf");
	}();
	const char* family = bahnschrift ? (bold ? "Bahnschrift SemiBold" : "Bahnschrift") : "Segoe UI";
	const int32_t style = bahnschrift ? kNormalFace : (bold ? kBoldFace : kNormalFace);
#elif MAC
	const char* family = "Helvetica Neue";
	const int32_t style = bold ? kBoldFace : kNormalFace;
#else
	// Pango takes a fallback list: the first family installed wins.
	const char* family = "Bahnschrift,Noto Sans,Cantarell,Ubuntu,DejaVu Sans,sans-serif";
	const int32_t style = bold ? kBoldFace : kNormalFace;
#endif
	auto f = makeOwned<CFontDesc> (family, size, style);
	cache[key] = f;
	return f;
}

void setAccent (const CColor& c) { colors::accent = c; }

void fillRoundRectGradient (CDrawContext* ctx, const CRect& r, CCoord radius, const CColor& top,
                            const CColor& bottom)
{
	auto path = owned (ctx->createRoundRectGraphicsPath (r, radius));
	auto gradient = owned (CGradient::create (0.0, 1.0, top, bottom));
	if (path && gradient)
		ctx->fillLinearGradient (path, *gradient, CPoint (r.left, r.top), CPoint (r.left, r.bottom));
}

void fillRoundRect (CDrawContext* ctx, const CRect& r, CCoord radius, const CColor& fill)
{
	if (auto path = owned (ctx->createRoundRectGraphicsPath (r, radius)))
	{
		ctx->setFillColor (fill);
		ctx->drawGraphicsPath (path, CDrawContext::kPathFilled);
	}
}

void drawPanel (CDrawContext* ctx, const CRect& r, CCoord radius, const CColor& fill)
{
	fillRoundRectGradient (ctx, r, radius, mix (fill, colors::text, 0.025f), fill);
	strokeRoundRect (ctx, r, radius, withAlpha (colors::border, 200));
	ctx->setFrameColor (CColor (255, 255, 255, 10));
	ctx->setLineWidth (1.0);
	ctx->setLineStyle (kLineSolid);
	ctx->drawLine (CPoint (r.left + radius, r.top + 1.5), CPoint (r.right - radius, r.top + 1.5));
}

void strokeRoundRect (CDrawContext* ctx, const CRect& r, CCoord radius, const CColor& stroke,
                      CCoord width)
{
	CRect inset = r;
	inset.inset (width * 0.5, width * 0.5);
	if (auto path = owned (ctx->createRoundRectGraphicsPath (inset, radius)))
	{
		ctx->setFrameColor (stroke);
		ctx->setLineWidth (width);
		ctx->setLineStyle (kLineSolid);
		ctx->drawGraphicsPath (path, CDrawContext::kPathStroked);
	}
}

void drawText (CDrawContext* ctx, const std::string& text, const CRect& r, CCoord size,
               const CColor& color, int align, bool bold)
{
	ctx->setFont (font (size, bold));
	ctx->setFontColor (color);
	ctx->drawString (text.c_str (), r,
	                 align == 0 ? kLeftText : align == 2 ? kRightText : kCenterText, true);
}

} // namespace suite::ui
