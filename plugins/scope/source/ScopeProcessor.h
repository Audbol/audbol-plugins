#pragma once

#include "ScopeParams.h"

#include "suite/vst/AudioTap.h"
#include "suite/vst/ProcessorBase.h"

#include <cstdint>
#include <vector>

namespace scope {

// Pass-through: streams the audio and song position to the editor and
// measures frequency (zero-crossing periods, sub-sample accurate) and peak.
class ScopeProcessor : public suite::ProcessorBase
{
public:
	ScopeProcessor ();
	static Steinberg::FUnknown* create (void*);

	Steinberg::tresult PLUGIN_API connect (Steinberg::Vst::IConnectionPoint* other) override;
	Steinberg::tresult PLUGIN_API disconnect (Steinberg::Vst::IConnectionPoint* other) override;
	Steinberg::tresult PLUGIN_API setActive (Steinberg::TBool state) override;

protected:
	void prepare (double sampleRate, int32_t maxBlockSize) override;
	void reset () override;
	void paramChanged (uint32_t, double) override {}
	void processAudio (const float* const* in, float* const* out, int32_t numSamples) override;

private:
	void publish ();

	// Frequency measurement on the mid signal
	int64_t sampleIndex {0};
	double lastCrossing {-1.0};
	double prevMid {0.0};
	bool armed {false};
	double envelope {0.0}, envelopeCoef {0.0};
	double periodSum {0.0};
	double periodSquares {0.0};
	int periodCount {0};
	int64_t lastCrossingSeen {0};
	double frequency {0.0};

	int hopSamples {4800}, hopCount {0};
	double hopPeak {0.0};

	suite::AudioTap tap {this, kNumTapStreams};
	std::vector<float> ppqBuffer;
};

} // namespace scope
