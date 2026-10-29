#include "TransProcessor.h"

#include <algorithm>
#include <cmath>

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace suite::dsp;

namespace trans {

// Level-independent, threshold-free shaping. A peak detector with a short
// hold (P) gives a ripple-free level for steady tones down to ~35 Hz; two
// envelopes follow it:
//
//   S rises over Attack Time, falls with P   attack  = dB(P / S) > 0 while S
//                                            is still catching up with an onset
//   L rises with P, falls over Sustain Time  sustain = dB(L / P) > 0 while the
//                                            sound decays faster than L
//
// Steady sounds give 0 for both. Each is capped at kMaxShapeDb and scaled by
// its amount (-100..100 %), so a hit gets the same treatment whether it's
// played softly or loudly.

namespace {
constexpr double kPeakHoldMs = 15.0;
constexpr double kPeakReleaseMs = 20.0;
constexpr double kGainSmoothMs = 0.3;
constexpr double kEnvFloor = 1e-6; // -120 dB: silence gets no shaping
constexpr double kDbPerNeper = 8.685889638065035;
} // namespace

TransProcessor::TransProcessor () : ProcessorBase (params (), kControllerUID) {}

FUnknown* TransProcessor::create (void*) { return static_cast<IAudioProcessor*> (new TransProcessor); }

tresult PLUGIN_API TransProcessor::connect (IConnectionPoint* other)
{
	auto result = ProcessorBase::connect (other);
	tap.onConnect (other, getHostContext ());
	return result;
}

tresult PLUGIN_API TransProcessor::disconnect (IConnectionPoint* other)
{
	tap.onDisconnect (other);
	return ProcessorBase::disconnect (other);
}

tresult PLUGIN_API TransProcessor::setActive (TBool state)
{
	if (state)
		tap.onActivate (processSetup);
	else
		tap.onDeactivate ();
	return ProcessorBase::setActive (state);
}

double TransProcessor::timeToCoef (double ms) const
{
	return std::exp (-1000.0 / (std::max (ms, 0.01) * sampleRate ()));
}

void TransProcessor::updateCoefficients ()
{
	peakRelCoef = timeToCoef (kPeakReleaseMs);
	holdSamples = static_cast<int> (kPeakHoldMs * 0.001 * sampleRate ());
	slowAtkCoef = timeToCoef (attackTimeMs);
	longRelCoef = timeToCoef (sustainTimeMs);
	gainCoef = timeToCoef (kGainSmoothMs);
}

void TransProcessor::prepare (double sr, int32_t maxBlock)
{
	updateCoefficients ();
	for (auto* s : {&attackAmt, &sustainAmt, &mix, &output})
		s->setup (sr, 30.0);
	const auto n = static_cast<size_t> (std::max (maxBlock, 1));
	tapIn.assign (n, 0.0f);
	tapOut.assign (n, 0.0f);
	tapGain.assign (n, 0.0f);
}

void TransProcessor::reset ()
{
	peakEnv = slowAtkEnv = longEnv = kEnvFloor;
	holdLeft = 0;
	gainDb = 0.0;
	for (auto* s : {&attackAmt, &sustainAmt, &mix, &output})
		s->reset (s->getTarget ());
}

void TransProcessor::paramChanged (uint32_t id, double v)
{
	switch (id)
	{
		case kAttack: attackAmt.setTarget (v / 100.0); break;
		case kSustain: sustainAmt.setTarget (v / 100.0); break;
		case kAttackTime: attackTimeMs = v; break;
		case kSustainTime: sustainTimeMs = v; break;
		case kMix: mix.setTarget (v / 100.0); break;
		case kOutput: output.setTarget (dbToGain (v)); break;
		default: break;
	}
	if (sampleRate () > 0.0 && (id == kAttackTime || id == kSustainTime))
		updateCoefficients ();
}

void TransProcessor::processAudio (const float* const* in, float* const* out, int32_t numSamples)
{
	const bool tapOn = static_cast<size_t> (numSamples) <= tapIn.size ();

	for (int32_t i = 0; i < numSamples; ++i)
	{
		const double x[2] = {in[0][i], in[1][i]};
		const double a = std::max ({std::abs (x[0]), std::abs (x[1]), kEnvFloor});

		if (a >= peakEnv)
		{
			peakEnv = a;
			holdLeft = holdSamples;
		}
		else if (holdLeft > 0)
			--holdLeft;
		else
			peakEnv = a + peakRelCoef * (peakEnv - a);
		const double p = peakEnv;
		slowAtkEnv = p > slowAtkEnv ? p + slowAtkCoef * (slowAtkEnv - p) : p;
		longEnv = p >= longEnv ? p : p + longRelCoef * (longEnv - p);

		const double atkDb = std::clamp (kDbPerNeper * std::log (p / slowAtkEnv), 0.0, kMaxShapeDb);
		const double susDb = std::clamp (kDbPerNeper * std::log (longEnv / p), 0.0, kMaxShapeDb);
		const double target = attackAmt.next () * atkDb + sustainAmt.next () * susDb;
		gainDb = target + gainCoef * (gainDb - target);
		if (std::abs (gainDb) < 1e-12)
			gainDb = 0.0; // no denormals while idle

		const double g = dbToGain (gainDb);
		const double wet = mix.next ();
		const double outGain = output.next ();
		double y[2];
		for (int c = 0; c < 2; ++c)
		{
			y[c] = (x[c] + (x[c] * g - x[c]) * wet) * outGain;
			out[c][i] = static_cast<float> (y[c]);
		}

		if (tapOn)
		{
			tapIn[i] = static_cast<float> (std::max (std::abs (x[0]), std::abs (x[1])));
			tapOut[i] = static_cast<float> (std::max (std::abs (y[0]), std::abs (y[1])));
			tapGain[i] = static_cast<float> (gainDb * wet);
		}
	}

	if (tapOn)
	{
		const float* streams[kNumTapStreams] = {tapIn.data (), tapOut.data (), tapGain.data ()};
		tap.push (numSamples, streams, nullptr);
	}
}

} // namespace trans
