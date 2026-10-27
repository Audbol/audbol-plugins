#pragma once

#include "RevParams.h"

#include "suite/dsp/DelayLine.h"
#include "suite/dsp/Smoother.h"
#include "suite/dsp/Svf.h"
#include "suite/vst/AudioTap.h"
#include "suite/vst/ProcessorBase.h"

#include <vector>

namespace rev {

using DelayBuffer = suite::dsp::DelayLine;

class RevProcessor : public suite::ProcessorBase
{
public:
	RevProcessor ();
	static Steinberg::FUnknown* create (void*);

	Steinberg::tresult PLUGIN_API connect (Steinberg::Vst::IConnectionPoint* other) override;
	Steinberg::tresult PLUGIN_API disconnect (Steinberg::Vst::IConnectionPoint* other) override;
	Steinberg::tresult PLUGIN_API setActive (Steinberg::TBool state) override;
	Steinberg::uint32 PLUGIN_API getTailSamples () override;

protected:
	void prepare (double sampleRate, int32_t maxBlockSize) override;
	void reset () override;
	void paramChanged (uint32_t id, double plain) override;
	void processAudio (const float* const* in, float* const* out, int32_t numSamples) override;

private:
	void updateLoop (double scale);
	void updateFilters ();

	struct Allpass
	{
		DelayBuffer buf;
		int delay {1};
		double process (double x, double g)
		{
			const double d = buf.at (delay);
			const double w = x + g * d;
			buf.push (static_cast<float> (w));
			return d - g * w;
		}
	};

	struct Line
	{
		DelayBuffer buf;
		double baseSamples {0}; // length at the current size
		double lpDamp {0}, lpCross {0}; // one-pole states
		double gLow {0}, gMid {0}, gHigh {0};
		double lfoPhase {0};
	};

	// Parameters
	double decaySec {2.5}, sizePct {60}, diffusion {0.8}, earlyLevel {0.5};
	double damping {6000}, bassMult {1.0}, lowCut {60}, highCut {12000};
	double modDepth {0.3}, modRate {0.8};
	bool freeze {false};

	suite::dsp::Smoother mix, width, preDelayMs, scale, freezeIn;
	DelayBuffer preDelay[2];
	DelayBuffer early;
	Allpass diffusers[2][4];
	Line lines[kNumLines];
	double dampCoef {0}, crossCoef {0};
	suite::dsp::BandDesign lowCutDesign, highCutDesign;
	suite::dsp::BandState lowCutState[2], highCutState[2];
	double appliedScale {-1};

	suite::AudioTap tap {this, kNumTapStreams};
	std::vector<float> tapIn, tapOut, tapWetL, tapWetR;
};

} // namespace rev
