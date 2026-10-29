#pragma once

#include "DessParams.h"

#include "suite/dsp/Smoother.h"
#include "suite/dsp/Svf.h"
#include "suite/vst/DynamicsTap.h"
#include "suite/vst/ProcessorBase.h"

#include <vector>

namespace dess {

// Tap layout: the dynamics streams, then the sidechain level and mono input
// audio for the spectrum.
enum TapStream : uint32_t
{
	kTapSidechain = suite::kNumDynStreams, // max(|band L|, |band R|)
	kTapAudio,                             // (L + R) / 2 before processing
	kNumTapStreams
};

class DessProcessor : public suite::ProcessorBase
{
public:
	DessProcessor ();
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
	void updateFilter ();
	double timeToCoef (double ms) const;

	// Parameters
	double threshold {-30}, range {8};
	double frequency {6500}, q {1.5};
	double releaseMs {40};
	Filter filter {Filter::Band};
	Mode mode {Mode::Split};
	bool listen {false};
	bool link {true};

	// State
	double attackCoef {0}, releaseCoef {0};
	double env[2] {}; // smoothed gain reduction, dB
	suite::dsp::BandDesign design;
	suite::dsp::BandState band[2];
	suite::dsp::Smoother outputGain;

	suite::AudioTap tap {this, kNumTapStreams};
	std::vector<float> tapIn, tapOut, tapGr, tapSc, tapAudio;
};

} // namespace dess
