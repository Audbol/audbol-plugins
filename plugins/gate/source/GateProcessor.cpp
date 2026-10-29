#include "GateProcessor.h"

#include <algorithm>
#include <cmath>

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace suite::dsp;

namespace gate {

// Key (detector) path: high-pass -> low-pass -> peak envelope, linked across
// channels. Gate mode opens above the threshold and closes once the key falls
// below threshold - hysteresis and the hold time has run out. Expand mode
// turns the signal down by (threshold - level) * (ratio - 1) below the
// threshold. Opening is smoothed in the linear gain domain (fast, clean
// attacks); closing in dB, so releases fade evenly instead of lingering.

namespace {
constexpr double kFloorDb = -150.0;
// The key peak holds briefly before releasing, so it stays steady through
// low-frequency cycles (down to ~35 Hz) instead of rippling.
constexpr double kDetectHoldMs = 15.0;
constexpr double kDetectReleaseMs = 20.0;
} // namespace

GateProcessor::GateProcessor () : ProcessorBase (params (), kControllerUID) {}

FUnknown* GateProcessor::create (void*) { return static_cast<IAudioProcessor*> (new GateProcessor); }

tresult PLUGIN_API GateProcessor::connect (IConnectionPoint* other)
{
	auto result = ProcessorBase::connect (other);
	tap.onConnect (other, getHostContext ());
	return result;
}

tresult PLUGIN_API GateProcessor::disconnect (IConnectionPoint* other)
{
	tap.onDisconnect (other);
	return ProcessorBase::disconnect (other);
}

tresult PLUGIN_API GateProcessor::setActive (TBool state)
{
	if (state)
		tap.onActivate (processSetup);
	else
		tap.onDeactivate ();
	return ProcessorBase::setActive (state);
}

double GateProcessor::timeToCoef (double ms) const
{
	return std::exp (-1000.0 / (std::max (ms, 0.005) * sampleRate ()));
}

void GateProcessor::updateCoefficients ()
{
	attackCoef = timeToCoef (attackMs);
	releaseCoef = timeToCoef (releaseMs);
	detectCoef = timeToCoef (kDetectReleaseMs);
	detectHoldSamples = static_cast<int> (kDetectHoldMs * 0.001 * sampleRate ());
	holdSamples = static_cast<int> (holdMs * 0.001 * sampleRate ());
}

void GateProcessor::updateKeyFilters ()
{
	hpfDesign = designBand (FilterType::LowCut, keyHpf, 0.707, 0.0, 1, sampleRate ());
	lpfDesign = designBand (FilterType::HighCut, keyLpf, 0.707, 0.0, 1, sampleRate ());
	// At their end stops the filters are out of the path (exactly flat); a
	// filter coming back in starts from a clean state.
	const bool hpf = keyHpf > 20.05, lpf = keyLpf < 19950.0;
	for (int c = 0; c < 2; ++c)
	{
		if (hpf && !hpfOn)
			hpfState[c].reset ();
		if (lpf && !lpfOn)
			lpfState[c].reset ();
	}
	hpfOn = hpf;
	lpfOn = lpf;
}

void GateProcessor::prepare (double, int32_t maxBlock)
{
	updateCoefficients ();
	updateKeyFilters ();
	const auto n = static_cast<size_t> (std::max (maxBlock, 1));
	tapIn.assign (n, 0.0f);
	tapOut.assign (n, 0.0f);
	tapGr.assign (n, 0.0f);
	tapKey.assign (n, 0.0f);
}

void GateProcessor::reset ()
{
	for (int c = 0; c < 2; ++c)
	{
		hpfState[c].reset ();
		lpfState[c].reset ();
	}
	env = 0.0;
	detectHoldLeft = 0;
	grDb = lastTargetDb = range; // start closed, like a gate in silence
	gain = targetGain = dbToGain (-range);
	open = false;
	holdLeft = 0;
}

void GateProcessor::paramChanged (uint32_t id, double v)
{
	switch (id)
	{
		case kThreshold: threshold = v; break;
		case kRange: range = v; break;
		case kMode: mode = static_cast<Mode> (static_cast<int> (v)); break;
		case kRatio: ratio = std::max (1.0, v); break;
		case kHysteresis: hysteresis = v; break;
		case kAttack: attackMs = v; break;
		case kHold: holdMs = v; break;
		case kRelease: releaseMs = v; break;
		case kKeyHpf: keyHpf = v; break;
		case kKeyLpf: keyLpf = v; break;
		case kListen: listen = v > 0.5; break;
		default: break;
	}
	if (sampleRate () > 0.0)
	{
		if (id == kAttack || id == kHold || id == kRelease)
			updateCoefficients ();
		if (id == kKeyHpf || id == kKeyLpf)
			updateKeyFilters ();
	}
}

void GateProcessor::processAudio (const float* const* in, float* const* out, int32_t numSamples)
{
	const bool tapOn = static_cast<size_t> (numSamples) <= tapIn.size ();
	const double openLin = dbToGain (threshold);
	const double closeLin = dbToGain (threshold - hysteresis);

	for (int32_t i = 0; i < numSamples; ++i)
	{
		const double x[2] = {in[0][i], in[1][i]};
		double key[2];
		for (int c = 0; c < 2; ++c)
		{
			key[c] = x[c];
			if (hpfOn)
				key[c] = hpfState[c].process (key[c], hpfDesign);
			if (lpfOn)
				key[c] = lpfState[c].process (key[c], lpfDesign);
		}

		// Linked peak envelope: instant attack, short hold, short release
		const double a = std::max (std::abs (key[0]), std::abs (key[1]));
		if (a >= env)
		{
			env = a;
			detectHoldLeft = detectHoldSamples;
		}
		else if (detectHoldLeft > 0)
			--detectHoldLeft;
		else
			env = a + detectCoef * (env - a);
		if (env < 1e-15)
			env = 0.0; // no denormals in silence

		// Target attenuation (Gate compares linear levels; only Expand needs dB)
		double targetDb;
		if (mode == Mode::Gate)
		{
			if (env >= openLin || (open && env >= closeLin))
			{
				open = true;
				holdLeft = holdSamples;
			}
			else if (holdLeft > 0)
				--holdLeft;
			else
				open = false;
			targetDb = open ? 0.0 : range;
		}
		else
		{
			const double levelDb = gainToDb (env, kFloorDb);
			if (levelDb >= threshold)
				holdLeft = holdSamples;
			else if (holdLeft > 0)
				--holdLeft;
			open = holdLeft > 0 || levelDb >= threshold;
			targetDb = open ? 0.0 : staticReductionDb (mode, levelDb, threshold, range, ratio);
		}

		// Ballistics; both representations are kept, and snap once they arrive,
		// so a settled gate costs no transcendental maths per sample.
		if (targetDb != lastTargetDb)
		{
			lastTargetDb = targetDb;
			targetGain = dbToGain (-targetDb);
		}
		if (targetGain > gain)
		{
			gain = targetGain + attackCoef * (gain - targetGain);
			if (targetGain - gain < 1e-7)
				gain = targetGain;
			grDb = -gainToDb (gain, kFloorDb);
		}
		else if (targetGain < gain)
		{
			grDb = targetDb + releaseCoef * (grDb - targetDb);
			if (targetDb - grDb < 1e-4)
				grDb = targetDb;
			gain = grDb == targetDb ? targetGain : dbToGain (-grDb);
		}

		double y[2];
		for (int c = 0; c < 2; ++c)
		{
			y[c] = listen ? key[c] : x[c] * gain;
			out[c][i] = static_cast<float> (y[c]);
		}

		if (tapOn)
		{
			tapIn[i] = static_cast<float> (std::max (std::abs (x[0]), std::abs (x[1])));
			tapOut[i] = static_cast<float> (std::max (std::abs (y[0]), std::abs (y[1])));
			tapGr[i] = static_cast<float> (std::max (0.0, grDb));
			tapKey[i] = static_cast<float> (env);
		}
	}

	if (tapOn)
	{
		const float* streams[kNumTapStreams] = {tapIn.data (), tapOut.data (), tapGr.data (), tapKey.data ()};
		tap.push (numSamples, streams, nullptr);
	}
}

} // namespace gate
