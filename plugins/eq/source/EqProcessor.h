#pragma once

#include "EqParams.h"

#include "suite/dsp/Smoother.h"
#include "suite/dsp/Svf.h"
#include "suite/vst/AudioTap.h"
#include "suite/vst/ProcessorBase.h"

#include <vector>

namespace eq {

class EqProcessor : public suite::ProcessorBase
{
public:
	EqProcessor ();
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
	struct Band
	{
		bool on {false};
		suite::dsp::FilterType type {suite::dsp::FilterType::Bell};
		int slope {3};
		Placement placement {Placement::Stereo};
		suite::dsp::Smoother logFreq, gainDb, logQ, mix;
		suite::dsp::BandDesign design;
		suite::dsp::BandState state[2];
		bool dirty {true};
	};

	void processBand (Band& band, float* l, float* r, int32_t n, double mixStart, double mixEnd);

	Band bands[kNumBands];
	suite::dsp::Smoother outputGain;
	double gainScale {1.0};
	AnalyzerMode analyzerMode {AnalyzerMode::Post};
	suite::AudioTap tap {this, 2};
	std::vector<float> preL, preR;
};

} // namespace eq
