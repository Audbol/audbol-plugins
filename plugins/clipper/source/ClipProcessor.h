#pragma once

#include "ClipParams.h"

#include "suite/dsp/DelayLine.h"
#include "suite/dsp/Oversampler.h"
#include "suite/dsp/Smoother.h"
#include "suite/vst/DynamicsTap.h"
#include "suite/vst/ProcessorBase.h"

#include <vector>

namespace clip {

class ClipProcessor : public suite::ProcessorBase
{
public:
	ClipProcessor ();
	static Steinberg::FUnknown* create (void*);

	Steinberg::tresult PLUGIN_API connect (Steinberg::Vst::IConnectionPoint* other) override;
	Steinberg::tresult PLUGIN_API disconnect (Steinberg::Vst::IConnectionPoint* other) override;
	Steinberg::tresult PLUGIN_API setActive (Steinberg::TBool state) override;

protected:
	void prepare (double sampleRate, int32_t maxBlockSize) override;
	void reset () override;
	void paramChanged (uint32_t id, double plain) override;
	void processAudio (const float* const* in, float* const* out, int32_t numSamples) override;
	uint32_t latencySamples () const override { return suite::dsp::Oversampler4x::kLatency; }

private:
	void updateOutput ();

	double inputDb {0}, outputDb {0};
	bool match {false};
	bool delta {false};

	suite::dsp::Smoother inGain, ceiling, knee, outGain;
	suite::dsp::Oversampler4x oversampler[2];
	suite::dsp::Oversampler4x deltaDown[2]; // only its downsampler is used
	suite::dsp::DelayLine driven[2];        // driven input, aligned with the output

	suite::AudioTap tap {this, suite::kNumDynStreams};
	std::vector<float> tapIn, tapOut, tapGr;
};

} // namespace clip
