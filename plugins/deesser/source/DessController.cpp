#include "DessController.h"

#include "DessEditor.h"
#include "DessProcessor.h"

#include <algorithm>

using namespace Steinberg;

namespace dess {

DessController::DessController () : ControllerBase (params (), {}, &dess::presets ()) {}

FUnknown* DessController::create (void*)
{
	return static_cast<Vst::IEditController*> (new DessController);
}

IPlugView* DessController::createEditor () { return new DessEditor (this); }

void DessController::onTapBlock (const suite::TapBlockView& block)
{
	if (block.streams () < kNumTapStreams)
		return;
	meters.accumulate (block);
	const float* sc = block.stream (kTapSidechain);
	for (uint32_t i = 0; i < block.frames (); ++i)
		scPeak = std::max (scPeak, sc[i]);
	spectrum.setSampleRate (block.sampleRate ());
	spectrum.push (block.stream (kTapAudio), static_cast<int> (block.frames ()));
}

DessMeters DessController::takeMeters ()
{
	DessMeters m {meters.take (), scPeak};
	scPeak = 0.0f;
	return m;
}

} // namespace dess
