#include "ScopeProcessor.h"

#include <algorithm>
#include <cmath>
#include <cstring>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace scope {

namespace {
constexpr double kHysteresis = 0.05; // fraction of the recent peak
constexpr double kNoSignalSeconds = 0.5;
constexpr double kMaxSpread = 0.03; // period standard deviation / mean
} // namespace

ScopeProcessor::ScopeProcessor () : ProcessorBase (params (), kControllerUID)
{
	processContextRequirements.needTempo ().needProjectTimeMusic ();
}

FUnknown* ScopeProcessor::create (void*) { return static_cast<IAudioProcessor*> (new ScopeProcessor); }

tresult PLUGIN_API ScopeProcessor::connect (IConnectionPoint* other)
{
	auto result = ProcessorBase::connect (other);
	tap.onConnect (other, getHostContext ());
	return result;
}

tresult PLUGIN_API ScopeProcessor::disconnect (IConnectionPoint* other)
{
	tap.onDisconnect (other);
	return ProcessorBase::disconnect (other);
}

tresult PLUGIN_API ScopeProcessor::setActive (TBool state)
{
	if (state)
		tap.onActivate (processSetup);
	else
		tap.onDeactivate ();
	return ProcessorBase::setActive (state);
}

void ScopeProcessor::prepare (double sr, int32_t maxBlock)
{
	hopSamples = std::max (1, static_cast<int> (sr * 0.1));
	envelopeCoef = std::exp (-1.0 / (0.2 * sr)); // 200 ms peak decay
	ppqBuffer.assign (static_cast<size_t> (std::max (maxBlock, 1)), -1.0f);
}

void ScopeProcessor::reset ()
{
	sampleIndex = 0;
	lastCrossing = -1.0;
	prevMid = 0.0;
	armed = false;
	envelope = 0.0;
	periodSum = 0.0;
	periodSquares = 0.0;
	periodCount = 0;
	lastCrossingSeen = 0;
	frequency = 0.0;
	hopCount = 0;
	hopPeak = 0.0;
}

void ScopeProcessor::publish ()
{
	// Average the periods seen in this hop; hold the last value while a
	// low note's period spans several hops; drop to 0 when the signal stops
	// or isn't periodic (crossing intervals vary by more than kMaxSpread).
	if (periodCount > 0)
	{
		const double mean = periodSum / periodCount;
		const double variance = std::max (0.0, periodSquares / periodCount - mean * mean);
		frequency = std::sqrt (variance) <= kMaxSpread * mean ? sampleRate () / mean : 0.0;
	}
	else if (sampleIndex - lastCrossingSeen > kNoSignalSeconds * sampleRate ())
		frequency = 0.0;
	periodSum = 0.0;
	periodSquares = 0.0;
	periodCount = 0;
	setOutputParam (kFrequency, std::min (frequency, 20000.0));
	setOutputParam (kPeak, hopPeak > 0.0 ? std::max (-70.0, 20.0 * std::log10 (hopPeak)) : -70.0);
	tap.setInfo (static_cast<float> (frequency));
	hopPeak = 0.0;
}

void ScopeProcessor::processAudio (const float* const* in, float* const* out, int32_t numSamples)
{
	const double ppq0 = blockPpq ();
	const double ppqPerSample = hostTempo () / 60.0 / sampleRate ();
	const bool tapOn = static_cast<size_t> (numSamples) <= ppqBuffer.size ();

	for (int32_t i = 0; i < numSamples; ++i)
	{
		const double l = in[0][i], r = in[1][i];
		const double mid = 0.5 * (l + r);
		const double a = std::max (std::abs (l), std::abs (r));
		hopPeak = std::max (hopPeak, a);
		envelope = std::max (std::abs (mid), envelope * envelopeCoef);

		// Rising zero crossing with hysteresis: arm below -h, fire at >= 0.
		const double h = kHysteresis * envelope;
		if (mid < -h)
			armed = true;
		if (armed && prevMid < 0.0 && mid >= 0.0 && envelope > 1e-5)
		{
			const double t = (sampleIndex - 1) + prevMid / (prevMid - mid); // sub-sample position
			if (lastCrossing >= 0.0)
			{
				const double period = t - lastCrossing;
				periodSum += period;
				periodSquares += period * period;
				++periodCount;
			}
			lastCrossing = t;
			lastCrossingSeen = sampleIndex;
			armed = false;
		}
		prevMid = mid;
		++sampleIndex;

		if (tapOn)
			ppqBuffer[i] = ppq0 >= 0.0 ? static_cast<float> (ppq0 + i * ppqPerSample) : -1.0f;
		if (++hopCount >= hopSamples)
		{
			hopCount = 0;
			publish ();
		}
	}

	if (tapOn)
	{
		const float* streams[kNumTapStreams] = {in[0], in[1], ppqBuffer.data ()};
		tap.push (numSamples, streams, nullptr);
	}
	for (int c = 0; c < 2; ++c)
		if (out[c] != in[c])
			std::memcpy (out[c], in[c], sizeof (float) * numSamples);
}

} // namespace scope
