#include "ChoProcessor.h"

#include <algorithm>
#include <cmath>

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace suite::dsp;

namespace cho {

// Per channel one delay line, read by 1-4 voices whose delays follow the
// master LFO with fixed phase offsets (voice index and stereo spread):
//   line input = x + feedback * (sum of voices) / N
//   wet        = (sum of voices) / N -> width -> low/high cut
//   out        = linear mix of dry and wet
// Voices and dry are strongly correlated at low frequencies, so the 1/N
// voice sum and the linear crossfade keep bass at unity instead of adding
// up (a 1/sqrt(N) sum with an equal-power mix boosted kicks by ~4 dB).
// The master phase goes to the UI with every tap block so the displays show
// the real LFO position.

namespace {
constexpr int kControlBlock = 16;
}

ChoProcessor::ChoProcessor () : ProcessorBase (params (), kControllerUID) {}

FUnknown* ChoProcessor::create (void*) { return static_cast<IAudioProcessor*> (new ChoProcessor); }

tresult PLUGIN_API ChoProcessor::connect (IConnectionPoint* other)
{
	auto result = ProcessorBase::connect (other);
	tap.onConnect (other, getHostContext ());
	return result;
}

tresult PLUGIN_API ChoProcessor::disconnect (IConnectionPoint* other)
{
	tap.onDisconnect (other);
	return ProcessorBase::disconnect (other);
}

tresult PLUGIN_API ChoProcessor::setActive (TBool state)
{
	if (state)
		tap.onActivate (processSetup);
	else
		tap.onDeactivate ();
	return ProcessorBase::setActive (state);
}

void ChoProcessor::updateFilters ()
{
	lowCutOn = lowCut > kLowCutOffHz;
	highCutOn = highCut < kHighCutOffHz;
	lowCutDesign = designBand (FilterType::LowCut, lowCut, 0.707, 0.0, 1, sampleRate ());
	highCutDesign = designBand (FilterType::HighCut, highCut, 0.707, 0.0, 1, sampleRate ());
}

void ChoProcessor::prepare (double sr, int32_t maxBlock)
{
	// Longest voice: max delay, +15 % voice offset, +90 % depth
	const int maxSamples = static_cast<int> (std::ceil (kMaxDelayMs * 1.15 * 1.9 * 0.001 * sr)) + 8;
	for (auto& l : line)
		l.init (maxSamples);
	mix.setup (sr, 30.0);
	depth.setup (sr, 50.0);
	delayMs.setup (sr, 80.0);
	width.setup (sr, 30.0);
	updateFilters ();
	const auto n = static_cast<size_t> (std::max (maxBlock, 1));
	tapIn.assign (n, 0.0f);
	tapOut.assign (n, 0.0f);
}

void ChoProcessor::reset ()
{
	for (int c = 0; c < 2; ++c)
	{
		line[c].clear ();
		lowCutState[c].reset ();
		highCutState[c].reset ();
	}
	masterPhase = 0.0;
	for (auto* s : {&mix, &depth, &delayMs, &width})
		s->reset (s->getTarget ());
}

void ChoProcessor::paramChanged (uint32_t id, double v)
{
	switch (id)
	{
		case kMix: mix.setTarget (v / 100.0); break;
		case kRate: rate = v; break;
		case kDepth: depth.setTarget (v / 100.0); break;
		case kDelay: delayMs.setTarget (v); break;
		case kVoices: voices = static_cast<int> (v) + 1; break;
		case kSpread: spreadDeg = v; break;
		case kFeedback: feedback = v / 100.0; break;
		case kShape: shape = static_cast<Shape> (static_cast<int> (v)); break;
		case kWidth: width.setTarget (v / 100.0); break;
		case kLowCut: lowCut = v; break;
		case kHighCut: highCut = v; break;
		default: break;
	}
	if ((id == kLowCut || id == kHighCut) && sampleRate () > 0.0)
		updateFilters ();
}

void ChoProcessor::processAudio (const float* const* in, float* const* out, int32_t numSamples)
{
	const double sr = sampleRate ();
	const double phaseInc = rate / sr;
	const bool tapOn = static_cast<size_t> (numSamples) <= tapIn.size ();
	const double wetNorm = 1.0 / voices;
	tap.setInfo (static_cast<float> (masterPhase));

	// Voice delays (in samples) are recomputed every kControlBlock samples and
	// interpolated linearly in between.
	double from[2][kMaxVoices], to[2][kMaxVoices];
	auto delaysAt = [&] (double phase, double d, double dep, double (&dst)[2][kMaxVoices]) {
		for (int c = 0; c < 2; ++c)
			for (int v = 0; v < voices; ++v)
			{
				const double p = voicePhase (phase, v, voices, c, spreadDeg);
				dst[c][v] = std::max (2.0, voiceDelayMs (shape, d, dep, v, voices, p) * 0.001 * sr);
			}
	};
	delaysAt (masterPhase, delayMs.getCurrent (), depth.getCurrent (), from);

	for (int32_t start = 0; start < numSamples; start += kControlBlock)
	{
		const int32_t n = std::min (kControlBlock, numSamples - start);
		masterPhase += phaseInc * n;
		masterPhase -= std::floor (masterPhase);
		const double d = delayMs.skip (n);
		const double dep = depth.skip (n);
		delaysAt (masterPhase, d, dep, to);

		for (int32_t k = 0; k < n; ++k)
		{
			const int32_t i = start + k;
			const double t = double (k + 1) / n;
			const double x[2] = {in[0][i], in[1][i]};
			double wet[2];
			for (int c = 0; c < 2; ++c)
			{
				double sum = 0.0;
				for (int v = 0; v < voices; ++v)
					sum += line[c].read (from[c][v] + (to[c][v] - from[c][v]) * t);
				line[c].push (static_cast<float> (x[c] + feedback * sum / voices));
				double w = sum * wetNorm;
				if (lowCutOn)
					w = lowCutState[c].process (w, lowCutDesign);
				if (highCutOn)
					w = highCutState[c].process (w, highCutDesign);
				wet[c] = w;
			}

			const double wd = width.next ();
			const double mid = 0.5 * (wet[0] + wet[1]), side = 0.5 * (wet[0] - wet[1]) * wd;
			const double m = mix.next ();
			const double dryGain = 1.0 - m;
			const double wetGain = m;
			const double oL = x[0] * dryGain + (mid + side) * wetGain;
			const double oR = x[1] * dryGain + (mid - side) * wetGain;
			out[0][i] = static_cast<float> (oL);
			out[1][i] = static_cast<float> (oR);

			if (tapOn)
			{
				tapIn[i] = static_cast<float> (std::max (std::abs (x[0]), std::abs (x[1])));
				tapOut[i] = static_cast<float> (std::max (std::abs (oL), std::abs (oR)));
			}
		}
		std::copy (&to[0][0], &to[0][0] + 2 * kMaxVoices, &from[0][0]);
	}

	if (tapOn)
	{
		const float* streams[kNumTapStreams] = {tapIn.data (), tapOut.data ()};
		tap.push (numSamples, streams, nullptr);
	}
}

} // namespace cho
