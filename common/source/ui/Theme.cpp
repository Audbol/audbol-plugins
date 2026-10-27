#include "suite/ui/Theme.h"

#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgraphicspath.h"

#include <algorithm>
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
	const char* family = "Segoe UI";
#else
	const char* family = "Helvetica Neue";
#endif
	auto f = makeOwned<CFontDesc> (family, size, bold ? kBoldFace : kNormalFace);
	cache[key] = f;
	return f;
}

void fillRoundRect (CDrawContext* ctx, const CRect& r, CCoord radius, const CColor& fill)
{
	if (auto path = owned (ctx->createRoundRectGraphicsPath (r, radius)))
	{
		ctx->setFillColor (fill);
		ctx->drawGraphicsPath (path, CDrawContext::kPathFilled);
	}
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
