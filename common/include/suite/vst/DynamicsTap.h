#pragma once

#include "suite/vst/AudioTap.h"

#include <algorithm>

namespace suite {

// Tap layout shared by dynamics processors (compressor, limiter...): three
// mono streams, one value per sample.
enum DynamicsStream : uint32_t
{
	kDynInput,         // max(|L|, |R|) before processing
	kDynOutput,        // max(|L|, |R|) after processing
	kDynGainReduction, // dB, >= 0
	kNumDynStreams
};

struct DynamicsSnapshot
{
	float inPeak {0};
	float outPeak {0};
	float grMax {0}; // dB
	bool fresh {false};
};

// Controller side: accumulates peaks from tap blocks until the UI takes them.
class DynamicsMeterFeed
{
public:
	void accumulate (const TapBlockView& block)
	{
		if (block.streams () < kNumDynStreams)
			return;
		const float* in = block.stream (kDynInput);
		const float* out = block.stream (kDynOutput);
		const float* gr = block.stream (kDynGainReduction);
		for (uint32_t i = 0; i < block.frames (); ++i)
		{
			acc.inPeak = std::max (acc.inPeak, in[i]);
			acc.outPeak = std::max (acc.outPeak, out[i]);
			acc.grMax = std::max (acc.grMax, gr[i]);
		}
		acc.fresh = true;
	}

	DynamicsSnapshot take ()
	{
		auto s = acc;
		acc = {};
		return s;
	}

private:
	DynamicsSnapshot acc;
};

} // namespace suite
