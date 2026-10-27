#pragma once

#include "DlyParams.h"

#include "suite/dsp/DelayLine.h"
#include "suite/dsp/Smoother.h"
#include "suite/dsp/Svf.h"
#include "suite/vst/AudioTap.h"
#include "suite/vst/ProcessorBase.h"

#include <vector>

namespace dly {

class DlyProcessor : public suite::ProcessorBase
{
public:
	DlyProcessor ();
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
	void updateFilters ();
	void currentTimesMs (double& left, double& right) const;

	bool sync {true}, link {true};
	double timeLMs {375}, timeRMs {500};
	int noteL {5}, noteR {7};
	double feedback {0.4};
	Mode mode {Mode::Stereo};
	double lowCut {100}, highCut {8000};
	double drive {0};
	double modDepth {0}, modRate {0.6};
	double ducking {0};

	suite::dsp::DelayLine line[2];
	suite::dsp::Smoother timeSamples[2], mix, width;
	suite::dsp::BandDesign lowCutDesign, highCutDesign;
	suite::dsp::BandState lowCutState[2], highCutState[2];
	double lfoPhase {0};
	double duckEnv {0};
	double duckAttack {0}, duckRelease {0};

	suite::AudioTap tap {this, kNumTapStreams};
	std::vector<float> tapIn, tapOut;
};

} // namespace dly
