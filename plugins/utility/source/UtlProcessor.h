#pragma once

#include "UtlParams.h"

#include "suite/dsp/Smoother.h"
#include "suite/dsp/Svf.h"
#include "suite/vst/AudioTap.h"
#include "suite/vst/ProcessorBase.h"

namespace utl {

class UtlProcessor : public suite::ProcessorBase
{
public:
	UtlProcessor ();
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

	ChannelMode mode {ChannelMode::Stereo};
	bool bassMono {false}, dcFilter {false};
	double bassMonoFreq {120};

	// Smoothed so every change is click-free
	suite::dsp::Smoother gain, mute, invL, invR, width, panL, panR;
	suite::dsp::BandDesign bassHp, dcHp;
	suite::dsp::BandState bassState;   // on the side signal
	suite::dsp::BandState dcState[2];

	suite::AudioTap tap {this, kNumTapStreams};
};

} // namespace utl
