#include "SatProcessor.h"

#include <algorithm>
#include <cmath>

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace suite::dsp;

namespace sat {

// Per channel: 4x upsample -> drive + bias + shaper -> 4x downsample ->
// DC blocker -> tone tilt -> auto gain / output -> mix with the dry signal
// (delayed by the oversampler latency so the mix doesn't comb-filter).

namespace {
constexpr int kOs = Oversampler4x::kFactor;
constexpr int kLatency = Oversampler4x::kLatency;
constexpr double kAutoGainWindowMs = 300.0;
constexpr double kMaxCompDb = 24.0;
} // namespace

SatProcessor::SatProcessor () : ProcessorBase (params (), kControllerUID) {}

FUnknown* SatProcessor::create (void*) { return static_cast<IAudioProcessor*> (new SatProcessor); }

tresult PLUGIN_API SatProcessor::connect (IConnectionPoint* other)
{
	auto result = ProcessorBase::connect (other);
	tap.onConnect (other, getHostContext ());
	return result;
}

tresult PLUGIN_API SatProcessor::disconnect (IConnectionPoint* other)
{
	tap.onDisconnect (other);
	return ProcessorBase::disconnect (other);
}

tresult PLUGIN_API SatProcessor::setActive (TBool state)
{
	if (state)
		tap.onActivate (processSetup);
	else
		tap.onDeactivate ();
	return ProcessorBase::setActive (state);
}

void SatProcessor::updateTone ()
{
	toneDesign = designBand (FilterType::TiltShelf, 1000.0, 0.5, toneDb, 0, sampleRate ());
}

void SatProcessor::prepare (double sr, int32_t maxBlock)
{
	for (auto& d : dryDelay)
		d.init (kLatency + 2);
	drive.setup (sr, 30.0);
	bias.setup (sr, 30.0);
	mix.setup (sr, 30.0);
	output.setup (sr, 30.0);
	autoComp.setup (sr, 200.0);
	dcCoef = std::exp (-2.0 * kPi * 8.0 / sr); // 8 Hz DC blocker
	powerCoef = std::exp (-1000.0 / (kAutoGainWindowMs * sr));
	updateTone ();
	const auto n = static_cast<size_t> (std::max (maxBlock, 1));
	tapIn.assign (n, 0.0f);
	tapOut.assign (n, 0.0f);
}

void SatProcessor::reset ()
{
	for (int c = 0; c < 2; ++c)
	{
		oversampler[c].reset ();
		dryDelay[c].clear ();
		toneState[c].reset ();
		dcX[c] = dcY[c] = 0.0;
	}
	inPower = outPower = 0.0;
	for (auto* s : {&drive, &bias, &mix, &output})
		s->reset (s->getTarget ());
	autoComp.reset (1.0);
}

void SatProcessor::paramChanged (uint32_t id, double v)
{
	switch (id)
	{
		case kDrive: drive.setTarget (dbToGain (v)); break;
		case kModel: model = static_cast<Model> (static_cast<int> (v)); break;
		case kBias: bias.setTarget (v / 100.0); break;
		case kTone:
			toneDb = v;
			if (sampleRate () > 0.0)
				updateTone ();
			break;
		case kMix: mix.setTarget (v / 100.0); break;
		case kOutput: output.setTarget (dbToGain (v)); break;
		case kAutoGain: autoGain = v > 0.5; break;
		default: break;
	}
}

void SatProcessor::processAudio (const float* const* in, float* const* out, int32_t numSamples)
{
	const bool tapOn = static_cast<size_t> (numSamples) <= tapIn.size ();
	const double maxComp = dbToGain (kMaxCompDb);

	for (int32_t i = 0; i < numSamples; ++i)
	{
		const double g = drive.next ();
		const double b = bias.next ();
		const double x[2] = {in[0][i], in[1][i]};
		double wet[2], dry[2];

		for (int c = 0; c < 2; ++c)
		{
			double os[kOs];
			oversampler[c].up (x[c], os);
			for (auto& s : os)
				s = saturate (model, s, g, b);
			double y = oversampler[c].down (os);

			// DC blocker: bias and asymmetric models create DC
			const double hp = y - dcX[c] + dcCoef * dcY[c];
			dcX[c] = y;
			dcY[c] = hp;
			wet[c] = toneState[c].process (hp, toneDesign);

			dryDelay[c].push (static_cast<float> (x[c]));
			dry[c] = dryDelay[c].at (kLatency + 1);
		}

		// Auto gain: match the wet RMS to the (aligned) dry RMS
		const double pin = 0.5 * (dry[0] * dry[0] + dry[1] * dry[1]);
		const double pout = 0.5 * (wet[0] * wet[0] + wet[1] * wet[1]);
		inPower = pin + powerCoef * (inPower - pin);
		outPower = pout + powerCoef * (outPower - pout);
		if (autoGain && inPower > 1e-9 && outPower > 1e-9)
			autoComp.setTarget (std::clamp (std::sqrt (inPower / outPower), 1.0 / maxComp, maxComp));
		else if (!autoGain)
			autoComp.setTarget (1.0);
		const double gain = autoComp.next () * output.next ();
		const double m = mix.next ();

		double y[2];
		for (int c = 0; c < 2; ++c)
		{
			y[c] = dry[c] + (wet[c] * gain - dry[c]) * m;
			out[c][i] = static_cast<float> (y[c]);
		}

		if (tapOn)
		{
			tapIn[i] = static_cast<float> (std::max (std::abs (x[0]), std::abs (x[1])));
			tapOut[i] = static_cast<float> (std::max (std::abs (y[0]), std::abs (y[1])));
		}
	}

	if (tapOn)
	{
		const float* streams[kNumTapStreams] = {tapIn.data (), tapOut.data ()};
		tap.push (numSamples, streams, nullptr);
	}
}

} // namespace sat
