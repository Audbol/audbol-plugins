#pragma once

#include "CompParams.h"

#include "suite/dsp/Smoother.h"
#include "suite/dsp/Svf.h"
#include "suite/vst/DynamicsTap.h"
#include "suite/vst/ProcessorBase.h"

#include <vector>

namespace comp {

class CompProcessor : public suite::ProcessorBase
{
public:
	CompProcessor ();
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
	void updateScFilter ();
	double timeToCoef (double ms) const;

	// Parameters
	double threshold {-18}, ratio {4}, knee {6};
	double attackMs {10}, releaseMs {150};
	bool autoRelease {false};
	double makeupDb {0};
	bool autoMakeup {false};
	Detection detection {Detection::Peak};
	double link {1.0};
	bool scFilterOn {false};
	double scFilterFreq {100};

	// State
	double attackCoef {0}, releaseCoef {0}, fastReleaseCoef {0}, slowReleaseCoef {0};
	double rmsCoef {0}, autoCoef {0};
	double rms[2] {};
	double env[2] {};     // smoothed gain reduction, dB
	double slowEnv[2] {}; // tracks sustained reduction for auto release
	suite::dsp::BandDesign scDesign;
	suite::dsp::BandState scState[2];
	suite::dsp::Smoother makeupGain, mix;

	suite::AudioTap tap {this, suite::kNumDynStreams};
	std::vector<float> tapIn, tapOut, tapGr;
};

} // namespace comp
