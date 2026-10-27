#pragma once

#include "suite/ParamSpec.h"
#include "suite/dsp/Smoother.h"

#include "public.sdk/source/vst/vstaudioeffect.h"

#include <functional>
#include <vector>

namespace suite {

// Versioned, order-independent state: a list of (id, normalized) pairs, so
// adding or removing parameters never breaks old sessions.
bool writeParamState (Steinberg::IBStream* stream, const ParamTable& table,
                      const std::function<double (uint32_t id)>& getNormalized);
bool readParamState (Steinberg::IBStream* stream,
                     const std::function<void (uint32_t id, double normalized)>& set);

// Base for every suite processor: stereo in/out, 32-bit float, parameter
// dispatch from the spec table, persistent state and a click-free bypass.
class ProcessorBase : public Steinberg::Vst::AudioEffect
{
public:
	ProcessorBase (const ParamTable& table, const Steinberg::FUID& controllerUID);

	Steinberg::tresult PLUGIN_API initialize (Steinberg::FUnknown* context) override;
	Steinberg::tresult PLUGIN_API setBusArrangements (Steinberg::Vst::SpeakerArrangement* inputs,
	                                                  Steinberg::int32 numIns,
	                                                  Steinberg::Vst::SpeakerArrangement* outputs,
	                                                  Steinberg::int32 numOuts) override;
	Steinberg::tresult PLUGIN_API canProcessSampleSize (Steinberg::int32 symbolicSampleSize) override;
	Steinberg::tresult PLUGIN_API setupProcessing (Steinberg::Vst::ProcessSetup& setup) override;
	Steinberg::tresult PLUGIN_API setActive (Steinberg::TBool state) override;
	Steinberg::tresult PLUGIN_API process (Steinberg::Vst::ProcessData& data) override;
	Steinberg::tresult PLUGIN_API setState (Steinberg::IBStream* state) override;
	Steinberg::tresult PLUGIN_API getState (Steinberg::IBStream* state) override;
	Steinberg::uint32 PLUGIN_API getLatencySamples () override { return latencySamples (); }

protected:
	// --- Hooks for concrete plug-ins -------------------------------------
	// Called (not on the audio thread) when sample rate / block size are known.
	virtual void prepare (double sampleRate, int32_t maxBlockSize) = 0;
	// Clear delay lines, filter states etc.
	virtual void reset () {}
	// A parameter changed. Called on the audio thread before processAudio,
	// and once for every parameter after prepare / setState.
	virtual void paramChanged (uint32_t id, double plain) = 0;
	// Process stereo audio. in and out may alias.
	virtual void processAudio (const float* const* in, float* const* out, int32_t numSamples) = 0;
	// Optional latency report.
	virtual uint32_t latencySamples () const { return 0; }

	double plain (uint32_t id) const;
	double normalized (uint32_t id) const;
	double sampleRate () const { return processSetup.sampleRate; }
	// Host tempo during processAudio (120 BPM if the host doesn't send one).
	// Plug-ins that use it call processContextRequirements.needTempo ().
	double hostTempo () const { return tempo; }
	// Song position in quarter notes at the start of the current block, or a
	// negative value when the transport isn't playing or the host doesn't say.
	// Plug-ins that use it call processContextRequirements.needProjectTimeMusic ().
	double blockPpq () const { return ppq; }
	// Report a read-only (output) parameter, e.g. a meter value. Call from
	// processAudio; the base sends it to the host at the end of the block.
	void setOutputParam (uint32_t id, double plain);

	const ParamTable& table;

private:
	int indexOf (uint32_t id) const;
	void setNormalizedInternal (uint32_t id, double value, bool notify);
	void notifyAll ();
	void fillDry (const float* const* in, Steinberg::int32 n);

	std::vector<double> values;   // normalized, indexed like table
	std::vector<int> idToIndex;   // dense lookup id -> table index
	double tempo {120.0};
	double ppq {-1.0};
	std::vector<std::pair<uint32_t, double>> pendingOutputs; // id, normalized
	uint32_t bypassId {0xFFFFFFFF};
	bool bypassed {false};
	dsp::Smoother bypassMix;
	std::vector<float> dry[2];
	std::vector<float> dryRing[2]; // latency compensation for the dry path
	size_t dryRingPos {0};
	uint32_t dryLatency {0};
};

} // namespace suite
