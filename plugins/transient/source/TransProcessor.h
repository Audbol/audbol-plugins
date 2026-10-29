#pragma once

#include "TransParams.h"

#include "suite/dsp/Smoother.h"
#include "suite/vst/AudioTap.h"
#include "suite/vst/ProcessorBase.h"

#include <vector>

namespace trans {

class TransProcessor : public suite::ProcessorBase
{
public:
	TransProcessor ();
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
	void updateCoefficients ();
	double timeToCoef (double ms) const;

	double attackTimeMs {15}, sustainTimeMs {250};

	// Envelopes of the linked input level (linear)
	double peakEnv {0};    // instant attack, short hold, short release
	int holdLeft {0}, holdSamples {0};
	double slowAtkEnv {0}; // rises over Attack Time, falls with peakEnv
	double longEnv {0};    // rises with peakEnv, falls over Sustain Time
	double peakRelCoef {0}, slowAtkCoef {0}, longRelCoef {0};
	double gainDb {0};
	double gainCoef {0};

	suite::dsp::Smoother attackAmt, sustainAmt, mix, output;

	suite::AudioTap tap {this, kNumTapStreams};
	std::vector<float> tapIn, tapOut, tapGain;
};

} // namespace trans
