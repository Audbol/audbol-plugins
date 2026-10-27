#include "suite/vst/AudioTap.h"

#include <algorithm>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace suite {

TapBlockView TapBlockView::from (const DataExchangeBlock& block)
{
	TapBlockView v;
	if (block.data && block.size >= sizeof (TapBlockHeader))
	{
		auto h = static_cast<const TapBlockHeader*> (block.data);
		auto needed = sizeof (TapBlockHeader) + h->numStreams * kTapCapacity * sizeof (float);
		if (h->numFrames <= kTapCapacity && block.size >= needed)
			v.header = h;
	}
	return v;
}

AudioTap::AudioTap (IAudioProcessor* processor, uint32_t streams)
: handler (processor,
           [streams] (DataExchangeHandler::Config& config, const ProcessSetup&) {
	           config.blockSize = static_cast<uint32> (
	               sizeof (TapBlockHeader) + streams * TapBlockView::kTapCapacity * sizeof (float));
	           config.numBlocks = 16;
	           config.alignment = 32;
	           return true;
           })
, numStreams (streams)
{
}

void AudioTap::onConnect (IConnectionPoint* other, FUnknown* hostContext)
{
	handler.onConnect (other, hostContext);
}

void AudioTap::onDisconnect (IConnectionPoint* other) { handler.onDisconnect (other); }

void AudioTap::onActivate (const ProcessSetup& setup)
{
	sampleRate = static_cast<float> (setup.sampleRate);
	current = nullptr;
	filled = 0;
	handler.onActivate (setup);
}

void AudioTap::onDeactivate ()
{
	current = nullptr;
	handler.onDeactivate ();
}

void AudioTap::push (int32_t numFrames, const float* const* left, const float* const* right)
{
	int32_t offset = 0;
	while (offset < numFrames)
	{
		if (!current)
		{
			auto block = handler.getCurrentOrNewBlock ();
			if (block.blockID == InvalidDataExchangeBlockID || !block.data)
				return; // queue full: drop
			current = static_cast<TapBlockHeader*> (block.data);
			current->numStreams = numStreams;
			current->sampleRate = sampleRate;
			current->info = infoValue;
			filled = 0;
		}

		auto todo = std::min<uint32_t> (TapBlockView::kTapCapacity - filled, numFrames - offset);
		auto base = reinterpret_cast<float*> (current + 1);
		for (uint32_t s = 0; s < numStreams; ++s)
		{
			float* dst = base + s * TapBlockView::kTapCapacity + filled;
			const float* l = left[s] + offset;
			const float* r = right && right[s] ? right[s] + offset : nullptr;
			if (r)
				for (uint32_t i = 0; i < todo; ++i)
					dst[i] = 0.5f * (l[i] + r[i]);
			else
				std::copy (l, l + todo, dst);
		}
		filled += todo;
		offset += todo;

		if (filled == TapBlockView::kTapCapacity)
		{
			current->numFrames = filled;
			handler.sendCurrentBlock ();
			current = nullptr;
		}
	}
}

void AudioTap::flush ()
{
	if (!current || filled == 0)
		return;
	current->numFrames = filled;
	handler.sendCurrentBlock ();
	current = nullptr;
}

} // namespace suite
