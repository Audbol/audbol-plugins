#include "TransController.h"

#include "TransEditor.h"

#include <algorithm>

using namespace Steinberg;

namespace trans {

TransController::TransController () : ControllerBase (params (), {}, &trans::presets ()) {}

FUnknown* TransController::create (void*)
{
	return static_cast<Vst::IEditController*> (new TransController);
}

IPlugView* TransController::createEditor () { return new TransEditor (this); }

void TransController::onTapBlock (const suite::TapBlockView& block)
{
	if (block.streams () < kNumTapStreams)
		return;
	const float* in = block.stream (kTapInput);
	const float* out = block.stream (kTapOutput);
	const float* gain = block.stream (kTapGain);
	for (uint32_t i = 0; i < block.frames (); ++i)
	{
		acc.inPeak = std::max (acc.inPeak, in[i]);
		acc.outPeak = std::max (acc.outPeak, out[i]);
		acc.gainMax = std::max (acc.gainMax, gain[i]);
		acc.gainMin = std::min (acc.gainMin, gain[i]);
	}
	acc.fresh = true;
}

} // namespace trans
