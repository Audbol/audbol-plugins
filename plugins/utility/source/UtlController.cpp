#include "UtlController.h"

#include "UtlEditor.h"

#include <algorithm>
#include <cmath>

using namespace Steinberg;

namespace utl {

UtlController::UtlController () : ControllerBase (params ()), scopeL (kScopeSize, 0.0f), scopeR (kScopeSize, 0.0f) {}

FUnknown* UtlController::create (void*)
{
	return static_cast<Vst::IEditController*> (new UtlController);
}

IPlugView* UtlController::createEditor () { return new UtlEditor (this); }

void UtlController::onTapBlock (const suite::TapBlockView& block)
{
	if (block.streams () < kNumTapStreams)
		return;
	const float* l = block.stream (kTapLeft);
	const float* r = block.stream (kTapRight);
	for (uint32_t i = 0; i < block.frames (); ++i)
	{
		const float a = l[i], b = r[i];
		stats.peakL = std::max (stats.peakL, std::abs (a));
		stats.peakR = std::max (stats.peakR, std::abs (b));
		stats.peakM = std::max (stats.peakM, std::abs (0.5f * (a + b)));
		stats.peakS = std::max (stats.peakS, std::abs (0.5f * (a - b)));
		stats.sumLR += double (a) * b;
		stats.sumLL += double (a) * a;
		stats.sumRR += double (b) * b;
		scopeL[scopePos] = a;
		scopeR[scopePos] = b;
		scopePos = (scopePos + 1) % kScopeSize;
	}
	stats.fresh = true;
}

void UtlController::copyScope (std::vector<float>& l, std::vector<float>& r) const
{
	l.resize (kScopeSize);
	r.resize (kScopeSize);
	for (int i = 0; i < kScopeSize; ++i)
	{
		l[i] = scopeL[(scopePos + i) % kScopeSize];
		r[i] = scopeR[(scopePos + i) % kScopeSize];
	}
}

} // namespace utl
