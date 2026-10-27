#include "SatController.h"

#include "SatEditor.h"

#include <algorithm>

using namespace Steinberg;

namespace sat {

SatController::SatController () : ControllerBase (params (), {}, &sat::presets ()) {}

FUnknown* SatController::create (void*)
{
	return static_cast<Vst::IEditController*> (new SatController);
}

IPlugView* SatController::createEditor () { return new SatEditor (this); }

void SatController::onTapBlock (const suite::TapBlockView& block)
{
	if (block.streams () < kNumTapStreams)
		return;
	const float* in = block.stream (kTapInput);
	const float* out = block.stream (kTapOutput);
	for (uint32_t i = 0; i < block.frames (); ++i)
	{
		levels.inPeak = std::max (levels.inPeak, in[i]);
		levels.outPeak = std::max (levels.outPeak, out[i]);
	}
	levels.fresh = true;
}

} // namespace sat
