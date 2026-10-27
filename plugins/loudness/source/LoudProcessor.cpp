#include "LoudProcessor.h"

#include <algorithm>
#include <cstring>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace loud {

LoudProcessor::LoudProcessor () : ProcessorBase (params (), kControllerUID) {}

FUnknown* LoudProcessor::create (void*) { return static_cast<IAudioProcessor*> (new LoudProcessor); }

tresult PLUGIN_API LoudProcessor::connect (IConnectionPoint* other)
{
	auto result = ProcessorBase::connect (other);
	tap.onConnect (other, getHostContext ());
	return result;
}

tresult PLUGIN_API LoudProcessor::disconnect (IConnectionPoint* other)
{
	tap.onDisconnect (other);
	return ProcessorBase::disconnect (other);
}

tresult PLUGIN_API LoudProcessor::setActive (TBool state)
{
	if (state)
		tap.onActivate (processSetup);
	else
		tap.onDeactivate ();
	return ProcessorBase::setActive (state);
}

void LoudProcessor::prepare (double sr, int32_t) { meter.prepare (sr); }

void LoudProcessor::reset () { meter.reset (); }

void LoudProcessor::paramChanged (uint32_t id, double v)
{
	if (id != kReset)
		return;
	// Reset on the rising edge (the editor's button sends 1 then 0).
	const bool held = v > 0.5;
	if (held && !resetHeld)
		resetRequested = true;
	resetHeld = held;
}

void LoudProcessor::publish ()
{
	const double m = meter.momentary (), s = meter.shortTerm (), i = meter.integrated ();
	const double lra = meter.range (), tpMax = meter.truePeakMax (), tpRecent = meter.truePeakRecent ();
	setOutputParam (kMomentary, m);
	setOutputParam (kShortTerm, s);
	setOutputParam (kIntegrated, i);
	setOutputParam (kRange, lra);
	setOutputParam (kTruePeakMax, std::max (tpMax, -70.0));

	const float values[kNumTapStreams] = {float (m), float (s), float (i), float (lra), float (tpMax), float (tpRecent)};
	const float* streams[kNumTapStreams];
	for (uint32_t k = 0; k < kNumTapStreams; ++k)
		streams[k] = &values[k];
	tap.push (1, streams, nullptr);
	tap.flush ();
}

void LoudProcessor::processAudio (const float* const* in, float* const* out, int32_t numSamples)
{
	if (resetRequested)
	{
		meter.resetIntegrated ();
		resetRequested = false;
		publish ();
	}
	for (int32_t i = 0; i < numSamples; ++i)
	{
		if (meter.push (in[0][i], in[1][i]))
			publish ();
	}
	for (int c = 0; c < 2; ++c)
		if (out[c] != in[c])
			std::memcpy (out[c], in[c], sizeof (float) * numSamples);
}

} // namespace loud
