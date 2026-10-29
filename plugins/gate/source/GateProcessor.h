#pragma once

#include "GateParams.h"

#include "suite/dsp/Smoother.h"
#include "suite/dsp/Svf.h"
#include "suite/vst/DynamicsTap.h"
#include "suite/vst/ProcessorBase.h"

#include <vector>

namespace gate {

class GateProcessor : public suite::ProcessorBase
{
public:
	GateProcessor ();
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
	void updateKeyFilters ();
	double timeToCoef (double ms) const;

	// Parameters
	double threshold {-40}, range {40}, ratio {2}, hysteresis {4};
	double attackMs {0.5}, holdMs {50}, releaseMs {150};
	double keyHpf {20}, keyLpf {20000};
	Mode mode {Mode::Gate};
	bool listen {false};

	// State
	double attackCoef {0}, releaseCoef {0}, detectCoef {0};
	int holdSamples {0};
	int detectHoldSamples {0}, detectHoldLeft {0};
	double env {0};    // key level, linear peak
	double gain {1};   // applied gain, linear
	double grDb {0};   // the same as attenuation in dB
	double targetGain {1}, lastTargetDb {0};
	bool open {false};
	int holdLeft {0};
	suite::dsp::BandDesign hpfDesign, lpfDesign;
	suite::dsp::BandState hpfState[2], lpfState[2];
	bool hpfOn {false}, lpfOn {false};

	suite::AudioTap tap {this, kNumTapStreams};
	std::vector<float> tapIn, tapOut, tapGr, tapKey;
};

} // namespace gate
