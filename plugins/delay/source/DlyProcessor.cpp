#include "DlyProcessor.h"

#include <algorithm>
#include <cmath>

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace suite::dsp;

namespace dly {

// Per channel: read the echo, mix input + feedback according to the mode,
// run low cut -> high cut -> drive (soft clip, unity small-signal gain), and
// write it back. Filtering before the write means every echo, including the
// first, passes through the filters once more than the previous one.

namespace {
constexpr double kMaxModMs = 3.0;
constexpr double kGlideMs = 120.0; // time changes glide like tape instead of clicking
} // namespace

DlyProcessor::DlyProcessor () : ProcessorBase (params (), kControllerUID)
{
	processContextRequirements.needTempo ();
}

FUnknown* DlyProcessor::create (void*) { return static_cast<IAudioProcessor*> (new DlyProcessor); }

tresult PLUGIN_API DlyProcessor::connect (IConnectionPoint* other)
{
	auto result = ProcessorBase::connect (other);
	tap.onConnect (other, getHostContext ());
	return result;
}

tresult PLUGIN_API DlyProcessor::disconnect (IConnectionPoint* other)
{
	tap.onDisconnect (other);
	return ProcessorBase::disconnect (other);
}

tresult PLUGIN_API DlyProcessor::setActive (TBool state)
{
	if (state)
		tap.onActivate (processSetup);
	else
		tap.onDeactivate ();
	return ProcessorBase::setActive (state);
}

uint32 PLUGIN_API DlyProcessor::getTailSamples ()
{
	double l, r;
	currentTimesMs (l, r);
	const double longest = std::max (l, r) * 0.001;
	if (feedback <= 0.001)
		return static_cast<uint32> (longest * sampleRate ()) + 1;
	// Repeats until -60 dB, capped at 60 s.
	const double repeats = feedback >= 0.999 ? 1e9 : 60.0 / (-20.0 * std::log10 (feedback));
	return static_cast<uint32> (std::min (60.0, (repeats + 1.0) * longest) * sampleRate ());
}

void DlyProcessor::currentTimesMs (double& left, double& right) const
{
	const double tempo = hostTempo ();
	left = delayMs (sync, noteL, timeLMs, tempo);
	right = (link || mode == Mode::PingPong) ? left : delayMs (sync, noteR, timeRMs, tempo);
}

void DlyProcessor::updateFilters ()
{
	lowCutDesign = designBand (FilterType::LowCut, lowCut, 0.707, 0.0, 1, sampleRate ());
	highCutDesign = designBand (FilterType::HighCut, highCut, 0.707, 0.0, 1, sampleRate ());
}

void DlyProcessor::prepare (double sr, int32_t maxBlock)
{
	const int maxSamples = static_cast<int> (std::ceil ((kMaxTimeMs + 2.0 * kMaxModMs) * 0.001 * sr)) + 8;
	for (auto& l : line)
		l.init (maxSamples);
	for (auto& t : timeSamples)
		t.setup (sr, kGlideMs);
	mix.setup (sr, 30.0);
	width.setup (sr, 30.0);
	duckAttack = std::exp (-1000.0 / (2.0 * sr));
	duckRelease = std::exp (-1000.0 / (250.0 * sr));
	updateFilters ();
	const auto n = static_cast<size_t> (std::max (maxBlock, 1));
	tapIn.assign (n, 0.0f);
	tapOut.assign (n, 0.0f);
}

void DlyProcessor::reset ()
{
	for (int c = 0; c < 2; ++c)
	{
		line[c].clear ();
		lowCutState[c].reset ();
		highCutState[c].reset ();
	}
	double l, r;
	currentTimesMs (l, r);
	timeSamples[0].reset (l * 0.001 * sampleRate ());
	timeSamples[1].reset (r * 0.001 * sampleRate ());
	mix.reset (mix.getTarget ());
	width.reset (width.getTarget ());
	lfoPhase = 0.0;
	duckEnv = 0.0;
}

void DlyProcessor::paramChanged (uint32_t id, double v)
{
	switch (id)
	{
		case kMix: mix.setTarget (v / 100.0); break;
		case kSync: sync = v > 0.5; break;
		case kTimeL: timeLMs = v; break;
		case kTimeR: timeRMs = v; break;
		case kNoteL: noteL = static_cast<int> (v); break;
		case kNoteR: noteR = static_cast<int> (v); break;
		case kLink: link = v > 0.5; break;
		case kFeedback: feedback = v / 100.0; break;
		case kMode: mode = static_cast<Mode> (static_cast<int> (v)); break;
		case kLowCut: lowCut = v; break;
		case kHighCut: highCut = v; break;
		case kDrive: drive = v / 100.0; break;
		case kModDepth: modDepth = v / 100.0; break;
		case kModRate: modRate = v; break;
		case kWidth: width.setTarget (v / 100.0); break;
		case kDucking: ducking = v / 100.0; break;
		default: break;
	}
	if ((id == kLowCut || id == kHighCut) && sampleRate () > 0.0)
		updateFilters ();
}

void DlyProcessor::processAudio (const float* const* in, float* const* out, int32_t numSamples)
{
	const double sr = sampleRate ();
	double msL, msR;
	currentTimesMs (msL, msR);
	timeSamples[0].setTarget (msL * 0.001 * sr);
	timeSamples[1].setTarget (msR * 0.001 * sr);

	const double modSamples = modDepth * kMaxModMs * 0.001 * sr;
	const double lfoInc = 2.0 * kPi * modRate / sr;
	const double k = 1.0 + 4.0 * drive; // drive: tanh(k x) / k
	const bool tapOn = static_cast<size_t> (numSamples) <= tapIn.size ();
	tap.setInfo (static_cast<float> (hostTempo ()));

	for (int32_t i = 0; i < numSamples; ++i)
	{
		const double xL = in[0][i];
		const double xR = in[1][i];

		lfoPhase += lfoInc;
		if (lfoPhase > 2.0 * kPi)
			lfoPhase -= 2.0 * kPi;
		const double dL = std::max (2.0, timeSamples[0].next () + modSamples * (1.0 + std::sin (lfoPhase)));
		const double dR = std::max (2.0, timeSamples[1].next () + modSamples * (1.0 + std::cos (lfoPhase)));
		const double echoL = line[0].read (dL);
		const double echoR = line[1].read (dR);

		double wL, wR;
		switch (mode)
		{
			case Mode::Stereo:
				wL = xL + feedback * echoL;
				wR = xR + feedback * echoR;
				break;
			case Mode::PingPong:
				wL = 0.5 * (xL + xR) + feedback * echoR;
				wR = feedback * echoL;
				break;
			case Mode::Cross:
			default:
				wL = xL + feedback * echoR;
				wR = xR + feedback * echoL;
				break;
		}
		double w[2] = {wL, wR};
		for (int c = 0; c < 2; ++c)
		{
			double v = highCutState[c].process (lowCutState[c].process (w[c], lowCutDesign), highCutDesign);
			if (drive > 0.0)
				v = std::tanh (k * v) / k;
			line[c].push (static_cast<float> (v));
		}

		// Ducking: pull the echoes down while the input is playing
		const double level = std::max (std::abs (xL), std::abs (xR));
		duckEnv = level + (level > duckEnv ? duckAttack : duckRelease) * (duckEnv - level);
		const double duckDb = 20.0 * std::log10 (duckEnv + 1e-9);
		const double duckGain = 1.0 - ducking * std::clamp ((duckDb + 40.0) / 40.0, 0.0, 1.0);

		// Width and mix
		const double wd = width.next ();
		const double mid = 0.5 * (echoL + echoR), side = 0.5 * (echoL - echoR) * wd;
		const double m = mix.next ();
		const double dryGain = std::cos (m * kPi * 0.5);
		const double wetGain = std::sin (m * kPi * 0.5) * duckGain;
		const double oL = xL * dryGain + (mid + side) * wetGain;
		const double oR = xR * dryGain + (mid - side) * wetGain;
		out[0][i] = static_cast<float> (oL);
		out[1][i] = static_cast<float> (oR);

		if (tapOn)
		{
			tapIn[i] = static_cast<float> (level);
			tapOut[i] = static_cast<float> (std::max (std::abs (oL), std::abs (oR)));
		}
	}

	if (tapOn)
	{
		const float* streams[kNumTapStreams] = {tapIn.data (), tapOut.data ()};
		tap.push (numSamples, streams, nullptr);
	}
}

} // namespace dly
