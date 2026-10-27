#include "LoudController.h"

#include "LoudEditor.h"

using namespace Steinberg;

namespace loud {

LoudController::LoudController () : ControllerBase (params ())
{
	histM.fill (-70.0f);
	histS.fill (-70.0f);
}

FUnknown* LoudController::create (void*)
{
	return static_cast<Vst::IEditController*> (new LoudController);
}

IPlugView* LoudController::createEditor () { return new LoudEditor (this); }

void LoudController::onTapBlock (const suite::TapBlockView& block)
{
	if (block.streams () < kNumTapStreams)
		return;
	for (uint32_t i = 0; i < block.frames (); ++i)
	{
		values.momentary = block.stream (kTapMomentary)[i];
		values.shortTerm = block.stream (kTapShortTerm)[i];
		values.integrated = block.stream (kTapIntegrated)[i];
		values.range = block.stream (kTapRange)[i];
		values.truePeakMax = block.stream (kTapTruePeakMax)[i];
		values.truePeakRecent = block.stream (kTapTruePeakRecent)[i];
		histM[histPos] = values.momentary;
		histS[histPos] = values.shortTerm;
		histPos = (histPos + 1) % kHistory;
	}
	received = true;
}

void LoudController::copyHistory (std::vector<float>& m, std::vector<float>& s) const
{
	m.resize (kHistory);
	s.resize (kHistory);
	for (int i = 0; i < kHistory; ++i)
	{
		m[i] = histM[(histPos + i) % kHistory];
		s[i] = histS[(histPos + i) % kHistory];
	}
}

void LoudController::requestReset ()
{
	editNormalized (kReset, 1.0);
	editNormalized (kReset, 0.0);
}

} // namespace loud
