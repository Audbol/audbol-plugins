#include "DessProcessor.h"

#include <algorithm>
#include <cmath>

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace suite::dsp;

namespace dess {

namespace {
constexpr double kFloorDb = -150.0;
constexpr double kAttackMs = 0.3; // fast enough to catch the start of an "s"
} // namespace

DessProcessor::DessProcessor () : ProcessorBase (params (), kControllerUID) {}

FUnknown* DessProcessor::create (void*) { return static_cast<IAudioProcessor*> (new DessProcessor); }

tresult PLUGIN_API DessProcessor::connect (IConnectionPoint* other)
{
	auto result = ProcessorBase::connect (other);
	tap.onConnect (other, getHostContext ());
	return result;
}

tresult PLUGIN_API DessProcessor::disconnect (IConnectionPoint* other)
{
	tap.onDisconnect (other);
	return ProcessorBase::disconnect (other);
}

tresult PLUGIN_API DessProcessor::setActive (TBool state)
{
	if (state)
		tap.onActivate (processSetup);
	else
		tap.onDeactivate ();
	return ProcessorBase::setActive (state);
}

double DessProcessor::timeToCoef (double ms) const
{
	return std::exp (-1000.0 / (std::max (ms, 0.01) * sampleRate ()));
}

void DessProcessor::updateFilter () { design = designDetector (filter, frequency, q, sampleRate ()); }

void DessProcessor::prepare (double, int32_t maxBlock)
{
	attackCoef = timeToCoef (kAttackMs);
	releaseCoef = timeToCoef (releaseMs);
	updateFilter ();
	outputGain.setup (sampleRate (), 50.0);
	const auto n = static_cast<size_t> (std::max (maxBlock, 1));
	for (auto* v : {&tapIn, &tapOut, &tapGr, &tapSc, &tapAudio})
		v->assign (n, 0.0f);
}

void DessProcessor::reset ()
{
	for (int c = 0; c < 2; ++c)
	{
		env[c] = 0.0;
		band[c].reset ();
	}
	outputGain.reset (outputGain.getTarget ());
}

void DessProcessor::paramChanged (uint32_t id, double v)
{
	switch (id)
	{
		case kThreshold: threshold = v; break;
		case kRange: range = v; break;
		case kFrequency: frequency = v; break;
		case kQ: q = v; break;
		case kRelease: releaseMs = v; break;
		case kFilter: filter = static_cast<Filter> (static_cast<int> (v)); break;
		case kMode: mode = static_cast<Mode> (static_cast<int> (v)); break;
		case kListen: listen = v > 0.5; break;
		case kStereoLink: link = v > 0.5; break;
		case kOutput: outputGain.setTarget (dbToGain (v)); break;
		default: break;
	}
	if (sampleRate () > 0.0)
	{
		if (id == kRelease)
			releaseCoef = timeToCoef (releaseMs);
		if (id == kFrequency || id == kQ || id == kFilter)
			updateFilter ();
	}
}

void DessProcessor::processAudio (const float* const* in, float* const* out, int32_t numSamples)
{
	const bool tapOn = static_cast<size_t> (numSamples) <= tapIn.size ();
	const bool split = mode == Mode::Split;

	for (int32_t i = 0; i < numSamples; ++i)
	{
		const double x[2] = {in[0][i], in[1][i]};
		double b[2], levelDb[2];
		for (int c = 0; c < 2; ++c)
		{
			b[c] = band[c].process (x[c], design);
			levelDb[c] = gainToDb (std::abs (b[c]), kFloorDb);
		}
		const double linked = std::max (levelDb[0], levelDb[1]);

		double gr[2];
		for (int c = 0; c < 2; ++c)
		{
			const double target = reductionDb (link ? linked : levelDb[c], threshold, range);
			const double coef = target > env[c] ? attackCoef : releaseCoef;
			env[c] = target + coef * (env[c] - target);
			gr[c] = env[c];
		}

		const double gain = outputGain.next ();
		double y[2];
		for (int c = 0; c < 2; ++c)
		{
			const double g = dbToGain (-gr[c]);
			double v;
			if (listen)
				v = b[c];
			else if (split)
				v = x[c] + b[c] * (g - 1.0); // turn down only the detected band
			else
				v = x[c] * g;
			y[c] = v * gain;
			out[c][i] = static_cast<float> (y[c]);
		}

		if (tapOn)
		{
			tapIn[i] = static_cast<float> (std::max (std::abs (x[0]), std::abs (x[1])));
			tapOut[i] = static_cast<float> (std::max (std::abs (y[0]), std::abs (y[1])));
			tapGr[i] = static_cast<float> (std::max (gr[0], gr[1]));
			tapSc[i] = static_cast<float> (std::max (std::abs (b[0]), std::abs (b[1])));
			tapAudio[i] = static_cast<float> ((x[0] + x[1]) * 0.5);
		}
	}

	if (tapOn)
	{
		const float* streams[kNumTapStreams] = {tapIn.data (), tapOut.data (), tapGr.data (), tapSc.data (),
		                                        tapAudio.data ()};
		tap.push (numSamples, streams, nullptr);
	}
}

} // namespace dess
