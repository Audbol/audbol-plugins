#include "RevProcessor.h"

#include <algorithm>
#include <cmath>

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace suite::dsp;

namespace rev {

// Signal flow (per stereo sample):
//   in -> pre-delay -> early reflections (multi-tap)               -> wet
//                   -> diffusion (4 allpasses / channel) -> FDN    -> wet
//   FDN: 8 modulated delay lines, Householder feedback, a 3-band loop filter
//        per line (low / mid / high gains set from Decay, Bass Mult, Damping)
//   wet -> width (M/S) -> low cut / high cut -> equal-power mix with dry

namespace {
constexpr double kDiffuserMs[2][4] = {{4.77, 3.59, 12.73, 9.31}, {5.13, 3.31, 11.89, 9.87}};
constexpr double kMaxModMs = 1.2;
constexpr double kMaxSizeScale = 1.6;
constexpr double kInputGain = 0.5;
constexpr double kOutputGain = 0.4;
constexpr double kEarlyGain = 0.6;
constexpr int kControlBlock = 32;

double onePoleCoef (double fc, double sr)
{
	const double g = std::tan (kPi * std::min (fc, sr * 0.45) / sr);
	return g / (1.0 + g);
}
} // namespace

RevProcessor::RevProcessor () : ProcessorBase (params (), kControllerUID) {}

FUnknown* RevProcessor::create (void*) { return static_cast<IAudioProcessor*> (new RevProcessor); }

tresult PLUGIN_API RevProcessor::connect (IConnectionPoint* other)
{
	auto result = ProcessorBase::connect (other);
	tap.onConnect (other, getHostContext ());
	return result;
}

tresult PLUGIN_API RevProcessor::disconnect (IConnectionPoint* other)
{
	tap.onDisconnect (other);
	return ProcessorBase::disconnect (other);
}

tresult PLUGIN_API RevProcessor::setActive (TBool state)
{
	if (state)
		tap.onActivate (processSetup);
	else
		tap.onDeactivate ();
	return ProcessorBase::setActive (state);
}

uint32 PLUGIN_API RevProcessor::getTailSamples ()
{
	if (freeze)
		return kInfiniteTail;
	const double seconds = preDelayMs.getTarget () * 0.001 + decaySec * std::max (1.0, bassMult) * 1.2;
	return static_cast<uint32> (seconds * sampleRate ());
}

void RevProcessor::prepare (double sr, int32_t maxBlock)
{
	auto samples = [sr] (double ms) { return static_cast<int> (std::ceil (ms * 0.001 * sr)); };
	for (auto& d : preDelay)
		d.init (samples (kMaxPreDelayMs) + 4);
	early.init (samples (kEarlyTaps[kNumEarlyTaps - 1].ms * kMaxSizeScale) + 4);
	for (int c = 0; c < 2; ++c)
		for (int i = 0; i < 4; ++i)
		{
			diffusers[c][i].delay = std::max (1, samples (kDiffuserMs[c][i]));
			diffusers[c][i].buf.init (diffusers[c][i].delay + 1);
		}
	for (int i = 0; i < kNumLines; ++i)
		lines[i].buf.init (samples (kLineMs[i] * kMaxSizeScale + kMaxModMs * 2) + 8);

	mix.setup (sr, 30.0);
	width.setup (sr, 30.0);
	preDelayMs.setup (sr, 80.0);
	scale.setup (sr, 300.0); // slow: size changes glide rather than click
	freezeIn.setup (sr, 30.0);
	appliedScale = -1;
	updateFilters ();

	const auto n = static_cast<size_t> (std::max (maxBlock, 1));
	tapIn.assign (n, 0.0f);
	tapOut.assign (n, 0.0f);
	tapWetL.assign (n, 0.0f);
	tapWetR.assign (n, 0.0f);
}

void RevProcessor::reset ()
{
	for (auto& d : preDelay)
		d.clear ();
	early.clear ();
	for (auto& ch : diffusers)
		for (auto& a : ch)
			a.buf.clear ();
	for (int i = 0; i < kNumLines; ++i)
	{
		auto& l = lines[i];
		l.buf.clear ();
		l.lpDamp = l.lpCross = 0.0;
		l.lfoPhase = 2.0 * kPi * i / kNumLines;
	}
	for (int c = 0; c < 2; ++c)
	{
		lowCutState[c].reset ();
		highCutState[c].reset ();
	}
	mix.reset (mix.getTarget ());
	width.reset (width.getTarget ());
	preDelayMs.reset (preDelayMs.getTarget ());
	scale.reset (scale.getTarget ());
	freezeIn.reset (freezeIn.getTarget ());
	appliedScale = -1;
}

void RevProcessor::updateLoop (double s)
{
	const double sr = sampleRate ();
	for (int i = 0; i < kNumLines; ++i)
	{
		auto& l = lines[i];
		l.baseSamples = kLineMs[i] * s * 0.001 * sr;
		if (freeze)
			l.gLow = l.gMid = l.gHigh = 1.0;
		else
		{
			const auto g = loopGains (l.baseSamples / sr, decaySec, bassMult);
			l.gLow = g.low;
			l.gMid = g.mid;
			l.gHigh = g.high;
		}
	}
	appliedScale = s;
}

void RevProcessor::updateFilters ()
{
	const double sr = sampleRate ();
	dampCoef = onePoleCoef (damping, sr);
	crossCoef = onePoleCoef (kCrossoverHz, sr);
	lowCutDesign = designBand (FilterType::LowCut, lowCut, 0.707, 0.0, 1, sr);
	highCutDesign = designBand (FilterType::HighCut, highCut, 0.707, 0.0, 1, sr);
}

void RevProcessor::paramChanged (uint32_t id, double v)
{
	switch (id)
	{
		case kMix: mix.setTarget (v / 100.0); break;
		case kPreDelay: preDelayMs.setTarget (v); break;
		case kDecay: decaySec = v / 1000.0; break;
		case kSize: sizePct = v; scale.setTarget (sizeScale (v)); break;
		case kDiffusion: diffusion = v / 100.0; break;
		case kEarly: earlyLevel = v / 100.0; break;
		case kWidth: width.setTarget (v / 100.0); break;
		case kDamping: damping = v; break;
		case kBassMult: bassMult = v; break;
		case kLowCut: lowCut = v; break;
		case kHighCut: highCut = v; break;
		case kModDepth: modDepth = v / 100.0; break;
		case kModRate: modRate = v; break;
		case kFreeze:
			freeze = v > 0.5;
			freezeIn.setTarget (freeze ? 0.0 : 1.0);
			break;
		default: break;
	}
	if (sampleRate () <= 0.0)
		return;
	if (id == kDamping || id == kLowCut || id == kHighCut)
		updateFilters ();
	if (id == kDecay || id == kBassMult || id == kFreeze)
		appliedScale = -1; // recompute loop gains on the next block
}

void RevProcessor::processAudio (const float* const* in, float* const* out, int32_t numSamples)
{
	const double sr = sampleRate ();
	const bool tapOn = static_cast<size_t> (numSamples) <= tapIn.size ();
	const double modSamples = modDepth * kMaxModMs * 0.001 * sr;
	const double lfoInc = 2.0 * kPi * modRate / sr;
	const double diffGain = 0.75 * diffusion;
	static constexpr double kOutSign[2][kNumLines] = {{1, -1, 1, -1, 1, -1, 1, -1}, {1, 1, -1, -1, 1, 1, -1, -1}};

	for (int32_t start = 0; start < numSamples; start += kControlBlock)
	{
		const int32_t end = std::min (numSamples, start + kControlBlock);
		const double s = scale.skip (end - start);
		if (std::abs (s - appliedScale) > 1e-6)
			updateLoop (s);

		// The LFOs are slow (<= 5 Hz): evaluate them at the block edges and
		// interpolate, instead of eight sin() calls per sample.
		const int32_t blockLen = end - start;
		double delayFrom[kNumLines], delayStep[kNumLines];
		double earlyDelay[kNumEarlyTaps];
		for (int n = 0; n < kNumLines; ++n)
		{
			auto& l = lines[n];
			const double from = l.baseSamples + modSamples * (1.0 + std::sin (l.lfoPhase));
			l.lfoPhase += lfoInc * (1.0 + 0.13 * n) * blockLen; // slightly different rates
			if (l.lfoPhase > 2.0 * kPi)
				l.lfoPhase -= 2.0 * kPi;
			const double to = l.baseSamples + modSamples * (1.0 + std::sin (l.lfoPhase));
			delayFrom[n] = from;
			delayStep[n] = (to - from) / blockLen;
		}
		for (int k = 0; k < kNumEarlyTaps; ++k)
			earlyDelay[k] = std::max (2.0, kEarlyTaps[k].ms * s * 0.001 * sr);

		for (int32_t i = start; i < end; ++i)
		{
			const int32_t step = i - start + 1;
			const double dryL = in[0][i];
			const double dryR = in[1][i];
			const double feed = freezeIn.next ();

			// Pre-delay
			const double pd = std::max (2.0, preDelayMs.next () * 0.001 * sr);
			preDelay[0].push (static_cast<float> (dryL));
			preDelay[1].push (static_cast<float> (dryR));
			const double pl = preDelay[0].read (pd) * feed;
			const double pr = preDelay[1].read (pd) * feed;

			// Early reflections from the pre-delayed mono signal
			early.push (static_cast<float> (0.5 * (pl + pr)));
			double erL = 0.0, erR = 0.0;
			for (int k = 0; k < kNumEarlyTaps; ++k)
			{
				const double v = early.read (earlyDelay[k]);
				erL += v * kEarlyTaps[k].gainL;
				erR += v * kEarlyTaps[k].gainR;
			}

			// Input diffusion
			double dl = pl, dr = pr;
			for (int k = 0; k < 4; ++k)
			{
				dl = diffusers[0][k].process (dl, diffGain);
				dr = diffusers[1][k].process (dr, diffGain);
			}

			// FDN: read, filter, mix, write
			double y[kNumLines];
			double wetL = 0.0, wetR = 0.0, sum = 0.0;
			for (int n = 0; n < kNumLines; ++n)
			{
				auto& l = lines[n];
				const double d = delayFrom[n] + delayStep[n] * step;
				const double x = l.buf.read (std::max (2.0, d));
				wetL += kOutSign[0][n] * x;
				wetR += kOutSign[1][n] * x;

				// 3-band loop filter: gH + (gM - gH) LP(damping) + (gL - gM) LP(250 Hz)
				const double vd = (x - l.lpDamp) * dampCoef;
				const double lpd = vd + l.lpDamp;
				l.lpDamp = lpd + vd;
				const double vc = (x - l.lpCross) * crossCoef;
				const double lpc = vc + l.lpCross;
				l.lpCross = lpc + vc;
				y[n] = l.gHigh * x + (l.gMid - l.gHigh) * lpd + (l.gLow - l.gMid) * lpc;
				sum += y[n];
			}
			const double house = sum * (2.0 / kNumLines); // Householder reflection
			for (int n = 0; n < kNumLines; ++n)
			{
				const double input = (n < kNumLines / 2 ? dl : dr) * ((n & 1) ? -kInputGain : kInputGain);
				lines[n].buf.push (static_cast<float> (y[n] - house + input));
			}

			// Wet bus
			double wl = wetL * kOutputGain + erL * earlyLevel * kEarlyGain;
			double wr = wetR * kOutputGain + erR * earlyLevel * kEarlyGain;
			const double w = width.next ();
			const double mid = 0.5 * (wl + wr), side = 0.5 * (wl - wr) * w;
			wl = mid + side;
			wr = mid - side;
			wl = highCutState[0].process (lowCutState[0].process (wl, lowCutDesign), highCutDesign);
			wr = highCutState[1].process (lowCutState[1].process (wr, lowCutDesign), highCutDesign);

			// Equal-power mix
			const double m = mix.next ();
			const double dryGain = std::cos (m * kPi * 0.5);
			const double wetGain = std::sin (m * kPi * 0.5);
			const double ol = dryL * dryGain + wl * wetGain;
			const double orr = dryR * dryGain + wr * wetGain;
			out[0][i] = static_cast<float> (ol);
			out[1][i] = static_cast<float> (orr);

			if (tapOn)
			{
				tapIn[i] = static_cast<float> (std::max (std::abs (dryL), std::abs (dryR)));
				tapOut[i] = static_cast<float> (std::max (std::abs (ol), std::abs (orr)));
				tapWetL[i] = static_cast<float> (wl);
				tapWetR[i] = static_cast<float> (wr);
			}
		}
	}

	if (tapOn)
	{
		const float* lefts[kNumTapStreams] = {tapIn.data (), tapOut.data (), tapWetL.data ()};
		const float* rights[kNumTapStreams] = {nullptr, nullptr, tapWetR.data ()};
		tap.push (numSamples, lefts, rights);
	}
}

} // namespace rev
