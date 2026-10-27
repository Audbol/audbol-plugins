#pragma once

#include "SatParams.h"

#include "suite/dsp/DelayLine.h"
#include "suite/dsp/Oversampler.h"
#include "suite/dsp/Smoother.h"
#include "suite/dsp/Svf.h"
#include "suite/vst/AudioTap.h"
#include "suite/vst/ProcessorBase.h"

#include <vector>

namespace sat {

class SatProcessor : public suite::ProcessorBase
{
public:
	SatProcessor ();
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
	void updateTone ();

	Model model {Model::Tape};
	bool autoGain {true};
	double toneDb {0};

	suite::dsp::Smoother drive, bias, mix, output, autoComp;
	suite::dsp::Oversampler4x oversampler[2];
	suite::dsp::DelayLine dryDelay[2];
	suite::dsp::BandDesign toneDesign;
	suite::dsp::BandState toneState[2];
	double dcX[2] {}, dcY[2] {}, dcCoef {0.9995};
	double inPower {0}, outPower {0}, powerCoef {0};

	suite::AudioTap tap {this, kNumTapStreams};
	std::vector<float> tapIn, tapOut;
};

} // namespace sat
