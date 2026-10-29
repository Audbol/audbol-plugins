#include "GateController.h"

#include "GateEditor.h"

#include <algorithm>

using namespace Steinberg;

namespace gate {

GateController::GateController () : ControllerBase (params (), {}, &gate::presets ()) {}

FUnknown* GateController::create (void*)
{
	return static_cast<Vst::IEditController*> (new GateController);
}

IPlugView* GateController::createEditor () { return new GateEditor (this); }

void GateController::onTapBlock (const suite::TapBlockView& block)
{
	if (block.streams () < kNumTapStreams)
		return;
	meters.accumulate (block);
	const float* key = block.stream (kTapKey);
	for (uint32_t i = 0; i < block.frames (); ++i)
		keyPeak = std::max (keyPeak, key[i]);
}

GateMeters GateController::takeMeters ()
{
	GateMeters m {meters.take (), keyPeak};
	keyPeak = 0.0f;
	return m;
}

} // namespace gate
