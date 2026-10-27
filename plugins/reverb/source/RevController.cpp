#include "RevController.h"

#include "RevEditor.h"

#include <algorithm>

using namespace Steinberg;

namespace rev {

RevController::RevController () : ControllerBase (params (), {}, &rev::presets ()) {}

FUnknown* RevController::create (void*)
{
	return static_cast<Vst::IEditController*> (new RevController);
}

IPlugView* RevController::createEditor () { return new RevEditor (this); }

void RevController::onTapBlock (const suite::TapBlockView& block)
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
	wet.setSampleRate (block.sampleRate ());
	wet.push (block.stream (kTapWet), static_cast<int> (block.frames ()));
}

} // namespace rev
