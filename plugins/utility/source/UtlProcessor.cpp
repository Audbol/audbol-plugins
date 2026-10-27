#include "UtlProcessor.h"

#include <algorithm>
#include <cmath>

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace suite::dsp;

namespace utl {

// channel mode -> polarity -> width (M/S) -> bass mono (24 dB/oct high-pass
// on the side signal) -> balance pan -> gain -> mute -> DC filter

UtlProcessor::UtlProcessor () : ProcessorBase (params (), kControllerUID) {}

FUnknown* UtlProcessor::create (void*) { return static_cast<IAudioProcessor*> (new UtlProcessor); }

tresult PLUGIN_API UtlProcessor::connect (IConnectionPoint* other)
{
	auto result = ProcessorBase::connect (other);
	tap.onConnect (other, getHostContext ());
	return result;
}

tresult PLUGIN_API UtlProcessor::disconnect (IConnectionPoint* other)
{
	tap.onDisconnect (other);
	return ProcessorBase::disconnect (other);
}

tresult PLUGIN_API UtlProcessor::setActive (TBool state)
{
	if (state)
		tap.onActivate (processSetup);
	else
		tap.onDeactivate ();
	return ProcessorBase::setActive (state);
}

void UtlProcessor::updateFilters ()
{
	bassHp = designBand (FilterType::LowCut, bassMonoFreq, 0.707, 0.0, 3, sampleRate ()); // 24 dB/oct
	dcHp = designBand (FilterType::LowCut, 5.0, 0.707, 0.0, 1, sampleRate ());
}

void UtlProcessor::prepare (double sr, int32_t)
{
	for (auto* s : {&gain, &mute, &invL, &invR, &width, &panL, &panR})
		s->setup (sr, 20.0);
	updateFilters ();
}

void UtlProcessor::reset ()
{
	bassState.reset ();
	for (auto& s : dcState)
		s.reset ();
	for (auto* s : {&gain, &mute, &invL, &invR, &width, &panL, &panR})
		s->reset (s->getTarget ());
}

void UtlProcessor::paramChanged (uint32_t id, double v)
{
	switch (id)
	{
		case kGain: gain.setTarget (dbToGain (v)); break;
		case kMute: mute.setTarget (v > 0.5 ? 0.0 : 1.0); break;
		case kInvertL: invL.setTarget (v > 0.5 ? -1.0 : 1.0); break;
		case kInvertR: invR.setTarget (v > 0.5 ? -1.0 : 1.0); break;
		case kChannelMode: mode = static_cast<ChannelMode> (static_cast<int> (v)); break;
		case kWidth: width.setTarget (v / 100.0); break;
		case kPan:
		{
			// Balance law: centre is unity on both sides, the far side fades out.
			const double p = v / 100.0;
			panL.setTarget (p > 0.0 ? 1.0 - p : 1.0);
			panR.setTarget (p < 0.0 ? 1.0 + p : 1.0);
			break;
		}
		case kBassMono:
			bassMono = v > 0.5;
			bassState.reset ();
			break;
		case kBassMonoFreq:
			bassMonoFreq = v;
			if (sampleRate () > 0.0)
				updateFilters ();
			break;
		case kDcFilter: dcFilter = v > 0.5; break;
		default: break;
	}
}

void UtlProcessor::processAudio (const float* const* in, float* const* out, int32_t numSamples)
{
	for (int32_t i = 0; i < numSamples; ++i)
	{
		double l = in[0][i];
		double r = in[1][i];
		switch (mode)
		{
			case ChannelMode::Stereo: break;
			case ChannelMode::Mono: l = r = 0.5 * (l + r); break;
			case ChannelMode::Left: r = l; break;
			case ChannelMode::Right: l = r; break;
			case ChannelMode::Swap: std::swap (l, r); break;
		}
		l *= invL.next ();
		r *= invR.next ();

		const double mid = 0.5 * (l + r);
		double side = 0.5 * (l - r) * width.next ();
		if (bassMono)
			side = bassState.process (side, bassHp);
		l = mid + side;
		r = mid - side;

		const double g = gain.next () * mute.next ();
		l *= panL.next () * g;
		r *= panR.next () * g;
		if (dcFilter)
		{
			l = dcState[0].process (l, dcHp);
			r = dcState[1].process (r, dcHp);
		}
		out[0][i] = static_cast<float> (l);
		out[1][i] = static_cast<float> (r);
	}

	const float* streams[kNumTapStreams] = {out[0], out[1]};
	tap.push (numSamples, streams, nullptr);
}

} // namespace utl
