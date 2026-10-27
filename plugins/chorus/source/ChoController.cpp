#include "ChoController.h"

#include "ChoEditor.h"

#include <algorithm>

using namespace Steinberg;

namespace cho {

ChoController::ChoController () : ControllerBase (params ()) {}

FUnknown* ChoController::create (void*)
{
	return static_cast<Vst::IEditController*> (new ChoController);
}

IPlugView* ChoController::createEditor () { return new ChoEditor (this); }

void ChoController::onTapBlock (const suite::TapBlockView& block)
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
	phase = block.info ();
}

} // namespace cho
