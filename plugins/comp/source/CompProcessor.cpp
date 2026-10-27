#include "CompProcessor.h"

#include <algorithm>
#include <cmath>

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace suite::dsp;

namespace comp {

namespace {
constexpr double kFloorDb = -150.0;
constexpr double kRmsWindowMs = 10.0;
constexpr double kAutoReleaseWindowMs = 600.0;
} // namespace

CompProcessor::CompProcessor () : ProcessorBase (params (), kControllerUID) {}

FUnknown* CompProcessor::create (void*) { return static_cast<IAudioProcessor*> (new CompProcessor); }

tresult PLUGIN_API CompProcessor::connect (IConnectionPoint* other)
{
	auto result = ProcessorBase::connect (other);
	tap.onConnect (other, getHostContext ());
	return result;
}

tresult PLUGIN_API CompProcessor::disconnect (IConnectionPoint* other)
{
	tap.onDisconnect (other);
	return ProcessorBase::disconnect (other);
}

tresult PLUGIN_API CompProcessor::setActive (TBool state)
{
	if (state)
		tap.onActivate (processSetup);
	else
		tap.onDeactivate ();
	return ProcessorBase::setActive (state);
}

double CompProcessor::timeToCoef (double ms) const
{
	// One-pole coefficient reaching ~63% of a step after `ms`.
	return std::exp (-1000.0 / (std::max (ms, 0.01) * sampleRate ()));
}

void CompProcessor::updateCoefficients ()
{
	attackCoef = timeToCoef (attackMs);
	releaseCoef = timeToCoef (releaseMs);
	fastReleaseCoef = timeToCoef (releaseMs * 0.3);
	slowReleaseCoef = timeToCoef (releaseMs * 2.5);
	rmsCoef = timeToCoef (kRmsWindowMs);
	autoCoef = timeToCoef (kAutoReleaseWindowMs);
}

void CompProcessor::updateScFilter ()
{
	scDesign = designBand (FilterType::LowCut, scFilterFreq, 0.707, 0.0, 1, sampleRate ());
}

void CompProcessor::prepare (double, int32_t maxBlock)
{
	updateCoefficients ();
	updateScFilter ();
	makeupGain.setup (sampleRate (), 50.0);
	mix.setup (sampleRate (), 50.0);
	const auto n = static_cast<size_t> (std::max (maxBlock, 1));
	tapIn.assign (n, 0.0f);
	tapOut.assign (n, 0.0f);
	tapGr.assign (n, 0.0f);
}

void CompProcessor::reset ()
{
	for (int c = 0; c < 2; ++c)
	{
		rms[c] = 0.0;
		env[c] = 0.0;
		slowEnv[c] = 0.0;
		scState[c].reset ();
	}
	makeupGain.reset (makeupGain.getTarget ());
	mix.reset (mix.getTarget ());
}

void CompProcessor::paramChanged (uint32_t id, double v)
{
	switch (id)
	{
		case kThreshold: threshold = v; break;
		case kRatio: ratio = std::max (1.0, v); break;
		case kKnee: knee = v; break;
		case kAttack: attackMs = v; break;
		case kRelease: releaseMs = v; break;
		case kAutoRelease: autoRelease = v > 0.5; break;
		case kMakeup: makeupDb = v; break;
		case kAutoMakeup: autoMakeup = v > 0.5; break;
		case kMix: mix.setTarget (v / 100.0); break;
		case kDetection: detection = static_cast<Detection> (static_cast<int> (v)); break;
		case kStereoLink: link = v / 100.0; break;
		case kScFilterOn: scFilterOn = v > 0.5; break;
		case kScFilterFreq: scFilterFreq = v; break;
		default: break;
	}
	if (sampleRate () > 0.0)
	{
		if (id == kAttack || id == kRelease)
			updateCoefficients ();
		if (id == kScFilterFreq)
			updateScFilter ();
	}
	// Manual and auto makeup add up; cap the sum at the manual range (+36 dB).
	const double totalMakeup = std::min (36.0, makeupDb + (autoMakeup ? autoMakeupDb (threshold, ratio) : 0.0));
	makeupGain.setTarget (dbToGain (totalMakeup));
}

void CompProcessor::processAudio (const float* const* in, float* const* out, int32_t numSamples)
{
	const bool tapOn = static_cast<size_t> (numSamples) <= tapIn.size ();
	const bool rmsMode = detection == Detection::Rms;

	for (int32_t i = 0; i < numSamples; ++i)
	{
		const double x[2] = {in[0][i], in[1][i]};

		// Detector level per channel, in dB.
		double levelDb[2];
		for (int c = 0; c < 2; ++c)
		{
			double sc = scFilterOn ? scState[c].process (x[c], scDesign) : x[c];
			double level;
			if (rmsMode)
			{
				rms[c] = sc * sc + rmsCoef * (rms[c] - sc * sc);
				level = std::sqrt (rms[c] * 2.0); // +3 dB so a sine reads its peak
			}
			else
				level = std::abs (sc);
			levelDb[c] = gainToDb (level, kFloorDb);
		}
		const double linked = std::max (levelDb[0], levelDb[1]);

		double gr[2];
		for (int c = 0; c < 2; ++c)
		{
			const double det = levelDb[c] + link * (linked - levelDb[c]);
			const double target = det - compressDb (det, threshold, ratio, knee);

			// Smooth in the gain-reduction domain (attack while GR rises).
			slowEnv[c] = target + autoCoef * (slowEnv[c] - target);
			double coef;
			if (target > env[c])
				coef = attackCoef;
			else if (autoRelease)
			{
				// Sustained reduction releases slowly, transients release fast.
				const double sustain = std::clamp (slowEnv[c] / (env[c] + 1e-3), 0.0, 1.0);
				coef = fastReleaseCoef + (slowReleaseCoef - fastReleaseCoef) * sustain;
			}
			else
				coef = releaseCoef;
			env[c] = target + coef * (env[c] - target);
			gr[c] = env[c];
		}

		const double makeup = makeupGain.next ();
		const double wet = mix.next ();
		double y[2];
		for (int c = 0; c < 2; ++c)
		{
			const double processed = x[c] * dbToGain (-gr[c]) * makeup;
			y[c] = x[c] + (processed - x[c]) * wet;
			out[c][i] = static_cast<float> (y[c]);
		}

		if (tapOn)
		{
			tapIn[i] = static_cast<float> (std::max (std::abs (x[0]), std::abs (x[1])));
			tapOut[i] = static_cast<float> (std::max (std::abs (y[0]), std::abs (y[1])));
			tapGr[i] = static_cast<float> (std::max (gr[0], gr[1]));
		}
	}

	if (tapOn)
	{
		const float* streams[suite::kNumDynStreams] = {tapIn.data (), tapOut.data (), tapGr.data ()};
		tap.push (numSamples, streams, nullptr);
	}
}

} // namespace comp
