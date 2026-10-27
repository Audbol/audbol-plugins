#include "LimProcessor.h"

#include <algorithm>
#include <cmath>

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace suite::dsp;

namespace lim {

// Gain envelope, per channel:
//   required = ceiling / peak            (peak: sample or 4x true peak)
//   env      = instant attack, smooth release towards `required`
//   hold     = min of env over H = L + 2 samples
//   gain     = mean of hold over B = L samples
// The mean of B holds is <= env at the three samples the holds all share, so
// the gain never exceeds what any of them requires. Detection describes the
// signal kDelay samples back, hence total latency L + kDelay.

namespace {
constexpr int kDetDelay = TruePeakDetector::kDelay;
constexpr double kAutoWindowMs = 400.0;
} // namespace

LimProcessor::LimProcessor () : ProcessorBase (params (), kControllerUID) {}

FUnknown* LimProcessor::create (void*) { return static_cast<IAudioProcessor*> (new LimProcessor); }

tresult PLUGIN_API LimProcessor::connect (IConnectionPoint* other)
{
	auto result = ProcessorBase::connect (other);
	tap.onConnect (other, getHostContext ());
	return result;
}

tresult PLUGIN_API LimProcessor::disconnect (IConnectionPoint* other)
{
	tap.onDisconnect (other);
	return ProcessorBase::disconnect (other);
}

tresult PLUGIN_API LimProcessor::setActive (TBool state)
{
	if (state)
		tap.onActivate (processSetup);
	else
		tap.onDeactivate ();
	return ProcessorBase::setActive (state);
}

void LimProcessor::updateRelease ()
{
	auto coef = [this] (double ms) { return std::exp (-1000.0 / (std::max (ms, 0.1) * sampleRate ())); };
	releaseCoef = coef (releaseMs);
	fastCoef = coef (releaseMs * 0.3);
	slowCoef = coef (releaseMs * 3.0);
	autoCoef = coef (kAutoWindowMs);
}

void LimProcessor::prepare (double sr, int32_t maxBlock)
{
	lookahead = std::max (1, static_cast<int> (std::round (sr * kLookaheadMs * 0.001)));
	// Audio sample a = n - (H + 4) lines up with the centre of the shared
	// window (see above) once detection's own delay is included.
	latency = static_cast<uint32_t> (lookahead + 2 + 4);
	for (auto& c : ch)
	{
		c.detDelay.assign (kDetDelay, 0.0);
		c.audioDelay.assign (latency, 0.0f);
		c.hold.resize (lookahead + 2);
		c.box.resize (lookahead);
	}
	inputGain.setup (sr, 20.0);
	ceiling.setup (sr, 20.0);
	updateRelease ();
	const auto n = static_cast<size_t> (std::max (maxBlock, 1));
	tapIn.assign (n, 0.0f);
	tapOut.assign (n, 0.0f);
	tapGr.assign (n, 0.0f);
}

void LimProcessor::reset ()
{
	for (auto& c : ch)
	{
		c.tp.reset ();
		std::fill (c.detDelay.begin (), c.detDelay.end (), 0.0);
		std::fill (c.audioDelay.begin (), c.audioDelay.end (), 0.0f);
		c.detPos = c.audioPos = 0;
		c.env = c.slow = 1.0;
		c.hold.reset ();
		c.box.reset ();
	}
	inputGain.reset (inputGain.getTarget ());
	ceiling.reset (ceiling.getTarget ());
}

void LimProcessor::paramChanged (uint32_t id, double v)
{
	switch (id)
	{
		case kInputGain: inputGain.setTarget (dbToGain (v)); break;
		case kCeiling: ceiling.setTarget (dbToGain (v)); break;
		case kRelease:
			releaseMs = v;
			if (sampleRate () > 0.0)
				updateRelease ();
			break;
		case kAutoRelease: autoRelease = v > 0.5; break;
		case kStereoLink: link = v / 100.0; break;
		case kTruePeak: truePeak = v > 0.5; break;
		default: break;
	}
}

void LimProcessor::processAudio (const float* const* in, float* const* out, int32_t numSamples)
{
	const bool tapOn = static_cast<size_t> (numSamples) <= tapIn.size ();
	const int lat = static_cast<int> (latency);

	for (int32_t i = 0; i < numSamples; ++i)
	{
		const double g = inputGain.next ();
		const double ceil = ceiling.next ();
		const double x[2] = {in[0][i] * g, in[1][i] * g};

		// Required gain per channel
		double req[2];
		for (int c = 0; c < 2; ++c)
		{
			auto& s = ch[c];
			const double tpPeak = s.tp.process (x[c]); // always run to keep history warm
			double peak;
			if (truePeak)
				peak = tpPeak;
			else
			{
				peak = std::abs (s.detDelay[s.detPos]);
				s.detDelay[s.detPos] = x[c];
				s.detPos = (s.detPos + 1) % kDetDelay;
			}
			req[c] = peak > ceil ? ceil / peak : 1.0;
		}
		const double linked = std::min (req[0], req[1]);

		double gain[2];
		for (int c = 0; c < 2; ++c)
		{
			auto& s = ch[c];
			const double r = req[c] + link * (linked - req[c]);
			s.slow = r + autoCoef * (s.slow - r);
			if (r < s.env)
				s.env = r; // instant attack: lookahead does the smoothing
			else
			{
				double coef = releaseCoef;
				if (autoRelease)
				{
					// Long reduction releases slowly, isolated peaks quickly.
					const double sustain = std::clamp ((1.0 - s.slow) / (1.0 - s.env + 1e-9), 0.0, 1.0);
					coef = fastCoef + (slowCoef - fastCoef) * sustain;
				}
				s.env = r + coef * (s.env - r);
			}
			gain[c] = s.box.push (s.hold.push (s.env));
		}

		double y[2];
		for (int c = 0; c < 2; ++c)
		{
			auto& s = ch[c];
			const double delayed = s.audioDelay[s.audioPos];
			s.audioDelay[s.audioPos] = static_cast<float> (x[c]);
			s.audioPos = (s.audioPos + 1) % lat;
			// Safety clip catches float rounding; it should never engage audibly.
			y[c] = std::clamp (delayed * gain[c], -ceil, ceil);
			out[c][i] = static_cast<float> (y[c]);
		}

		if (tapOn)
		{
			tapIn[i] = static_cast<float> (std::max (std::abs (in[0][i]), std::abs (in[1][i])));
			tapOut[i] = static_cast<float> (std::max (std::abs (y[0]), std::abs (y[1])));
			tapGr[i] = static_cast<float> (-gainToDb (std::min (gain[0], gain[1]), -150.0));
		}
	}

	if (tapOn)
	{
		const float* streams[suite::kNumDynStreams] = {tapIn.data (), tapOut.data (), tapGr.data ()};
		tap.push (numSamples, streams, nullptr);
	}
}

} // namespace lim
