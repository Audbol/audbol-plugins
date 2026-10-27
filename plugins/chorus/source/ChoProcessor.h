#pragma once

#include "ChoParams.h"

#include "suite/dsp/DelayLine.h"
#include "suite/dsp/Smoother.h"
#include "suite/dsp/Svf.h"
#include "suite/vst/AudioTap.h"
#include "suite/vst/ProcessorBase.h"

#include <vector>

namespace cho {

class ChoProcessor : public suite::ProcessorBase
{
public:
	ChoProcessor ();
	static Steinberg::FUnknown* create (void*);

	Steinberg::tresult PLUGIN_API connect (Steinberg::Vst::IConnectionPoint* other) override;
	Steinberg::tresult PLUGIN_API disconnect (Steinberg::Vst::IConnectionPoint* other) override;
	Steinberg::tresult PLUGIN_API setActive (Steinberg::TBool state) override;

protected:
	void prepare (double sampleRate, int32_t maxBlockSize) override;
	void reset () override;
	void paramChanged (uint32_t id, double plain) override;
	void processAudio (const float* const* in, float* const* out, int32_t numSamples) override;

private:
	void updateFilters ();

	double rate {0.6};
	int voices {2};
	double spreadDeg {90};
	double feedback {0};
	Shape shape {Shape::Sine};
	double lowCut {80}, highCut {14000};
	bool lowCutOn {true}, highCutOn {true};

	suite::dsp::Smoother mix, depth, delayMs, width;
	suite::dsp::DelayLine line[2];
	double masterPhase {0};
	suite::dsp::BandDesign lowCutDesign, highCutDesign;
	suite::dsp::BandState lowCutState[2], highCutState[2];

	suite::AudioTap tap {this, kNumTapStreams};
	std::vector<float> tapIn, tapOut;
};

} // namespace cho
