#include "ScopeController.h"

#include "ScopeEditor.h"

#include <algorithm>

using namespace Steinberg;

namespace scope {

ScopeController::ScopeController () : ControllerBase (params ())
{
	const auto n = static_cast<size_t> (kHistorySeconds * rate);
	left.assign (n, 0.0f);
	right.assign (n, 0.0f);
	songPos.assign (n, -1.0f);
}

FUnknown* ScopeController::create (void*)
{
	return static_cast<Vst::IEditController*> (new ScopeController);
}

IPlugView* ScopeController::createEditor () { return new ScopeEditor (this); }

void ScopeController::onTapBlock (const suite::TapBlockView& block)
{
	if (block.streams () < kNumTapStreams)
		return;
	if (block.sampleRate () > 0.0 && block.sampleRate () != rate)
	{
		rate = block.sampleRate ();
		const auto n = static_cast<size_t> (kHistorySeconds * rate);
		left.assign (n, 0.0f);
		right.assign (n, 0.0f);
		songPos.assign (n, -1.0f);
		writePos = 0;
	}
	const float* l = block.stream (kTapLeft);
	const float* r = block.stream (kTapRight);
	const float* p = block.stream (kTapPpq);
	for (uint32_t i = 0; i < block.frames (); ++i)
	{
		left[writePos] = l[i];
		right[writePos] = r[i];
		songPos[writePos] = p[i];
		writePos = (writePos + 1) % left.size ();
	}
	freq = block.info ();
}

void ScopeController::copyRecent (size_t count, std::vector<float>& l, std::vector<float>& r,
                                  std::vector<float>& ppq) const
{
	const size_t cap = left.size ();
	count = std::min (count, cap);
	l.resize (count);
	r.resize (count);
	ppq.resize (count);
	const size_t start = (writePos + cap - count) % cap;
	for (size_t i = 0; i < count; ++i)
	{
		const size_t k = (start + i) % cap;
		l[i] = left[k];
		r[i] = right[k];
		ppq[i] = songPos[k];
	}
}

} // namespace scope
