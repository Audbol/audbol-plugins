#pragma once

#include "public.sdk/source/vst/utility/dataexchange.h"

#include <cstdint>

namespace suite {

// Wire format of one tap block: a header followed by `numStreams` mono
// streams of `numFrames` samples each.
struct TapBlockHeader
{
	uint32_t numFrames;
	uint32_t numStreams;
	float sampleRate;
	float info; // one plug-in-defined value per block (e.g. host tempo)
};

struct TapBlockView
{
	const TapBlockHeader* header {nullptr};

	bool valid () const { return header != nullptr; }
	uint32_t frames () const { return header->numFrames; }
	uint32_t streams () const { return header->numStreams; }
	double sampleRate () const { return header->sampleRate; }
	float info () const { return header->info; }
	const float* stream (uint32_t i) const
	{
		return reinterpret_cast<const float*> (header + 1) + i * kTapCapacity;
	}

	static constexpr uint32_t kTapCapacity = 512;
	static TapBlockView from (const Steinberg::Vst::DataExchangeBlock& block);
};

// Processor-side sender. Collects mono-summed audio from one or more points
// in the signal chain and ships it to the controller without blocking the
// audio thread (blocks are dropped if the UI side falls behind).
class AudioTap
{
public:
	AudioTap (Steinberg::Vst::IAudioProcessor* processor, uint32_t numStreams);

	void onConnect (Steinberg::Vst::IConnectionPoint* other, Steinberg::FUnknown* hostContext);
	void onDisconnect (Steinberg::Vst::IConnectionPoint* other);
	void onActivate (const Steinberg::Vst::ProcessSetup& setup);
	void onDeactivate ();
	// Value carried in the header of the blocks sent from now on.
	void setInfo (float v) { infoValue = v; }

	// left/right are arrays indexed by stream; right[i] may be null for mono.
	void push (int32_t numFrames, const float* const* left, const float* const* right);
	// Send the partly filled block now (for low-rate data such as meter values).
	void flush ();

private:
	Steinberg::Vst::DataExchangeHandler handler;
	uint32_t numStreams;
	float sampleRate {48000.0f};
	uint32_t filled {0};
	TapBlockHeader* current {nullptr};
	float infoValue {0.0f};
};

} // namespace suite
