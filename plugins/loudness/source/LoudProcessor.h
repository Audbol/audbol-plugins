#pragma once

#include "LoudParams.h"

#include "suite/dsp/LoudnessMeter.h"
#include "suite/vst/AudioTap.h"
#include "suite/vst/ProcessorBase.h"

namespace loud {

// Pass-through meter: audio is untouched; loudness is measured here (so the
// integrated value keeps counting while the editor is closed), reported as
// output parameters and sent to the editor every 100 ms.
class LoudProcessor : public suite::ProcessorBase
{
public:
	LoudProcessor ();
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
	void publish ();

	suite::dsp::LoudnessMeter meter;
	bool resetRequested {false};
	bool resetHeld {false};
	suite::AudioTap tap {this, kNumTapStreams};
};

} // namespace loud
