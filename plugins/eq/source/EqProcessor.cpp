#include "EqProcessor.h"

#include <algorithm>
#include <cmath>
#include <cstring>

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace suite::dsp;

namespace eq {

namespace {
constexpr int32_t kControlBlock = 32; // samples between coefficient updates
}

EqProcessor::EqProcessor () : ProcessorBase (params (), kControllerUID) {}

FUnknown* EqProcessor::create (void*) { return static_cast<IAudioProcessor*> (new EqProcessor); }

tresult PLUGIN_API EqProcessor::connect (IConnectionPoint* other)
{
	auto result = ProcessorBase::connect (other);
	tap.onConnect (other, getHostContext ());
	return result;
}

tresult PLUGIN_API EqProcessor::disconnect (IConnectionPoint* other)
{
	tap.onDisconnect (other);
	return ProcessorBase::disconnect (other);
}

tresult PLUGIN_API EqProcessor::setActive (TBool state)
{
	if (state)
		tap.onActivate (processSetup);
	else
		tap.onDeactivate ();
	return ProcessorBase::setActive (state);
}

void EqProcessor::prepare (double sr, int32_t maxBlock)
{
	for (auto& b : bands)
	{
		b.logFreq.setup (sr, 40.0);
		b.gainDb.setup (sr, 40.0);
		b.logQ.setup (sr, 40.0);
		b.mix.setup (sr, 25.0);
		b.dirty = true;
	}
	outputGain.setup (sr, 30.0);
	preL.assign (static_cast<size_t> (std::max (maxBlock, 1)), 0.0f);
	preR.assign (preL.size (), 0.0f);
}

void EqProcessor::reset ()
{
	for (auto& b : bands)
	{
		b.state[0].reset ();
		b.state[1].reset ();
		b.logFreq.reset (b.logFreq.getTarget ());
		b.gainDb.reset (b.gainDb.getTarget ());
		b.logQ.reset (b.logQ.getTarget ());
		b.mix.reset (b.on ? 1.0 : 0.0);
		b.dirty = true;
	}
	outputGain.reset (outputGain.getTarget ());
}

void EqProcessor::paramChanged (uint32_t id, double v)
{
	if (isBandParam (id))
	{
		auto& b = bands[bandOf (id)];
		switch (bandParamOf (id))
		{
			case kBandOn:
				b.on = v > 0.5;
				b.mix.setTarget (b.on ? 1.0 : 0.0);
				break;
			case kBandType: b.type = static_cast<FilterType> (static_cast<int> (v)); break;
			case kBandFreq: b.logFreq.setTarget (std::log (v)); break;
			case kBandGain: b.gainDb.setTarget (v); break;
			case kBandQ: b.logQ.setTarget (std::log (v)); break;
			case kBandSlope: b.slope = static_cast<int> (v); break;
			case kBandPlacement: b.placement = static_cast<Placement> (static_cast<int> (v)); break;
		}
		b.dirty = true;
		return;
	}
	switch (id)
	{
		case kOutputGain: outputGain.setTarget (dbToGain (v)); break;
		case kGainScale:
			gainScale = v / 100.0;
			for (auto& b : bands)
				b.dirty = true;
			break;
		case kAnalyzerMode: analyzerMode = static_cast<AnalyzerMode> (static_cast<int> (v)); break;
		default: break;
	}
}

void EqProcessor::processBand (Band& b, float* l, float* r, int32_t n, double mixStart, double mixEnd)
{
	const auto& d = b.design;
	auto& s0 = b.state[0];
	auto& s1 = b.state[1];
	const double mixStep = (mixEnd - mixStart) / n;
	double mix = mixStart;

	for (int32_t i = 0; i < n; ++i)
	{
		mix += mixStep;
		const double inL = l[i];
		const double inR = r[i];
		double yl = inL, yr = inR;
		switch (b.placement)
		{
			case Placement::Stereo:
				yl = s0.process (inL, d);
				yr = s1.process (inR, d);
				break;
			case Placement::Left: yl = s0.process (inL, d); break;
			case Placement::Right: yr = s1.process (inR, d); break;
			case Placement::Mid:
			{
				const double m = s0.process ((inL + inR) * 0.5, d);
				const double s = (inL - inR) * 0.5;
				yl = m + s;
				yr = m - s;
				break;
			}
			case Placement::Side:
			{
				const double m = (inL + inR) * 0.5;
				const double s = s1.process ((inL - inR) * 0.5, d);
				yl = m + s;
				yr = m - s;
				break;
			}
		}
		l[i] = static_cast<float> (inL + (yl - inL) * mix);
		r[i] = static_cast<float> (inR + (yr - inR) * mix);
	}
}

void EqProcessor::processAudio (const float* const* in, float* const* out, int32_t numSamples)
{
	float* l = out[0];
	float* r = out[1];
	if (l != in[0])
		std::memcpy (l, in[0], sizeof (float) * numSamples);
	if (r != in[1])
		std::memcpy (r, in[1], sizeof (float) * numSamples);

	const bool tapOn = analyzerMode != AnalyzerMode::Off && static_cast<size_t> (numSamples) <= preL.size ();
	if (tapOn)
	{
		std::memcpy (preL.data (), l, sizeof (float) * numSamples);
		std::memcpy (preR.data (), r, sizeof (float) * numSamples);
	}

	const double sr = sampleRate ();
	for (int32_t offset = 0; offset < numSamples; offset += kControlBlock)
	{
		const int32_t n = std::min (kControlBlock, numSamples - offset);
		for (auto& b : bands)
		{
			const double mixStart = b.mix.getCurrent ();
			if (!b.on && mixStart <= 0.0)
				continue;
			if (mixStart <= 0.0)
			{
				// Waking up from fully off: start from clean filter state.
				b.state[0].reset ();
				b.state[1].reset ();
			}
			const bool moving = b.logFreq.isSmoothing () || b.gainDb.isSmoothing () || b.logQ.isSmoothing ();
			const double f = std::exp (b.logFreq.skip (n));
			const double g = b.gainDb.skip (n) * gainScale;
			const double q = std::exp (b.logQ.skip (n));
			const double mixEnd = b.mix.skip (n);
			if (moving || b.dirty)
			{
				b.design = designBand (b.type, f, q, g, b.slope, sr);
				b.dirty = false;
			}
			processBand (b, l + offset, r + offset, n, mixStart, mixEnd);
		}

		for (int32_t i = offset; i < offset + n; ++i)
		{
			const float gain = static_cast<float> (outputGain.next ());
			l[i] *= gain;
			r[i] *= gain;
		}
	}

	if (tapOn)
	{
		const float* lefts[2] = {preL.data (), l};
		const float* rights[2] = {preR.data (), r};
		tap.push (numSamples, lefts, rights);
	}
}

} // namespace eq
