#include "ClipProcessor.h"

#include <algorithm>
#include <cmath>

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace suite::dsp;

namespace clip {

// Per channel: input gain -> 4x upsample -> clip curve -> 4x downsample ->
// a final hard limit at the ceiling at the base rate (the downsampling
// filter rings a little above a clipped peak; this catches it) -> output.
// Delta downsamples what the curve removed through a second filter, so it is
// exactly the difference between the driven input and the clipped output.

namespace {
constexpr int kOs = Oversampler4x::kFactor;
constexpr int kLatency = Oversampler4x::kLatency;
constexpr double kFloorDb = -150.0;
} // namespace

ClipProcessor::ClipProcessor () : ProcessorBase (params (), kControllerUID) {}

FUnknown* ClipProcessor::create (void*) { return static_cast<IAudioProcessor*> (new ClipProcessor); }

tresult PLUGIN_API ClipProcessor::connect (IConnectionPoint* other)
{
	auto result = ProcessorBase::connect (other);
	tap.onConnect (other, getHostContext ());
	return result;
}

tresult PLUGIN_API ClipProcessor::disconnect (IConnectionPoint* other)
{
	tap.onDisconnect (other);
	return ProcessorBase::disconnect (other);
}

tresult PLUGIN_API ClipProcessor::setActive (TBool state)
{
	if (state)
		tap.onActivate (processSetup);
	else
		tap.onDeactivate ();
	return ProcessorBase::setActive (state);
}

void ClipProcessor::prepare (double sr, int32_t maxBlock)
{
	for (auto& d : driven)
		d.init (kLatency + 2);
	for (auto* s : {&inGain, &ceiling, &knee, &outGain})
		s->setup (sr, 30.0);
	const auto n = static_cast<size_t> (std::max (maxBlock, 1));
	tapIn.assign (n, 0.0f);
	tapOut.assign (n, 0.0f);
	tapGr.assign (n, 0.0f);
}

void ClipProcessor::reset ()
{
	for (int c = 0; c < 2; ++c)
	{
		oversampler[c].reset ();
		deltaDown[c].reset ();
		driven[c].clear ();
	}
	for (auto* s : {&inGain, &ceiling, &knee, &outGain})
		s->reset (s->getTarget ());
}

void ClipProcessor::updateOutput ()
{
	outGain.setTarget (dbToGain (outputDb - (match ? inputDb : 0.0)));
}

void ClipProcessor::paramChanged (uint32_t id, double v)
{
	switch (id)
	{
		case kInput:
			inputDb = v;
			inGain.setTarget (dbToGain (v));
			updateOutput ();
			break;
		case kCeiling: ceiling.setTarget (dbToGain (v)); break;
		case kSoftness: knee.setTarget (v / 100.0 * kMaxKnee); break;
		case kOutput:
			outputDb = v;
			updateOutput ();
			break;
		case kMatch:
			match = v > 0.5;
			updateOutput ();
			break;
		case kDelta:
			if (v > 0.5 && !delta)
				for (auto& o : deltaDown)
					o.reset (); // no leftovers from the last time it was on
			delta = v > 0.5;
			break;
		default: break;
	}
}

void ClipProcessor::processAudio (const float* const* in, float* const* out, int32_t numSamples)
{
	const bool tapOn = static_cast<size_t> (numSamples) <= tapIn.size ();

	for (int32_t i = 0; i < numSamples; ++i)
	{
		const double g = inGain.next ();
		const double c = ceiling.next ();
		const double k = knee.next ();
		const double post = outGain.next ();
		double y[2], d[2];

		for (int ch = 0; ch < 2; ++ch)
		{
			const double s = in[ch][i] * g;
			double os[kOs], removed[kOs];
			oversampler[ch].up (s, os);
			for (int j = 0; j < kOs; ++j)
			{
				const double clipped = clipSample (os[j], c, k);
				removed[j] = os[j] - clipped;
				os[j] = clipped;
			}
			const double down = oversampler[ch].down (os);
			const double limited = std::clamp (down, -c, c);
			// The delta filter only runs while Delta is on (it settles within 32 samples).
			const double diff = delta ? deltaDown[ch].down (removed) + (down - limited) : 0.0;

			driven[ch].push (static_cast<float> (s));
			d[ch] = driven[ch].at (kLatency + 1);
			y[ch] = (delta ? diff : limited) * post;
			out[ch][i] = static_cast<float> (y[ch]);
		}

		if (tapOn)
		{
			// Clipping amount: how far the aligned driven peak was pulled down
			const double dPeak = std::max (std::abs (d[0]), std::abs (d[1]));
			const double yPeak = std::max (std::abs (y[0]), std::abs (y[1])) / post;
			double gr = 0.0;
			if (!delta && dPeak > c * (1.0 - kMaxKnee))
				gr = std::max (0.0, gainToDb (dPeak, kFloorDb) - gainToDb (yPeak, kFloorDb));
			tapIn[i] = static_cast<float> (dPeak);
			tapOut[i] = static_cast<float> (std::max (std::abs (y[0]), std::abs (y[1])));
			tapGr[i] = static_cast<float> (gr);
		}
	}

	if (tapOn)
	{
		const float* streams[suite::kNumDynStreams] = {tapIn.data (), tapOut.data (), tapGr.data ()};
		tap.push (numSamples, streams, nullptr);
	}
}

} // namespace clip
