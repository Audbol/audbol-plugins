#include "DlyController.h"

#include "DlyEditor.h"

#include <algorithm>

using namespace Steinberg;

namespace dly {

DlyController::DlyController () : ControllerBase (params ()) {}

FUnknown* DlyController::create (void*)
{
	return static_cast<Vst::IEditController*> (new DlyController);
}

IPlugView* DlyController::createEditor () { return new DlyEditor (this); }

void DlyController::onTapBlock (const suite::TapBlockView& block)
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
	if (block.info () > 0.0f)
		hostTempo = block.info ();
}

void DlyController::currentTimesMs (double& left, double& right)
{
	const bool sync = getPlain (kSync) > 0.5;
	const bool linked = getPlain (kLink) > 0.5 ||
	                    static_cast<Mode> (static_cast<int> (getPlain (kMode))) == Mode::PingPong;
	left = delayMs (sync, static_cast<int> (getPlain (kNoteL)), getPlain (kTimeL), hostTempo);
	right = linked ? left : delayMs (sync, static_cast<int> (getPlain (kNoteR)), getPlain (kTimeR), hostTempo);
}

} // namespace dly
