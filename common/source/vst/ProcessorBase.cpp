#include "suite/vst/ProcessorBase.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"

#include <algorithm>
#include <cstring>

#if defined(_M_X64) || defined(__x86_64__) || defined(_M_IX86) || defined(__i386__)
#include <xmmintrin.h>
#define SUITE_HAS_SSE 1
#endif

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace suite {

namespace {
constexpr uint32 kStateMagic = 0x48535531; // 'HSU1'
constexpr uint32 kStateVersion = 1;

// Flush denormals to zero for the duration of a process call; decaying
// filter and reverb tails otherwise cost a lot of CPU.
struct ScopedNoDenormals
{
#if SUITE_HAS_SSE
	unsigned int saved {_mm_getcsr ()};
	ScopedNoDenormals () { _mm_setcsr (saved | 0x8040); } // FTZ | DAZ
	~ScopedNoDenormals () { _mm_setcsr (saved); }
#endif
};
} // namespace

bool writeParamState (IBStream* stream, const ParamTable& table,
                      const std::function<double (uint32_t)>& getNormalized)
{
	IBStreamer s (stream, kLittleEndian);
	// Read-only parameters are meter outputs, not settings: don't save them.
	const auto saved = std::count_if (table.begin (), table.end (),
	                                  [] (const ParamSpec& p) { return !(p.flags & kReadOnly); });
	if (!s.writeInt32u (kStateMagic) || !s.writeInt32u (kStateVersion) ||
	    !s.writeInt32u (static_cast<uint32> (saved)))
		return false;
	for (auto& spec : table)
	{
		if (spec.flags & kReadOnly)
			continue;
		if (!s.writeInt32u (spec.id) || !s.writeDouble (getNormalized (spec.id)))
			return false;
	}
	return true;
}

bool readParamState (IBStream* stream, const std::function<void (uint32_t, double)>& set)
{
	IBStreamer s (stream, kLittleEndian);
	uint32 magic = 0, version = 0, count = 0;
	if (!s.readInt32u (magic) || magic != kStateMagic)
		return false;
	if (!s.readInt32u (version) || version > kStateVersion)
		return false;
	if (!s.readInt32u (count) || count > 100000)
		return false;
	for (uint32 i = 0; i < count; ++i)
	{
		uint32 id = 0;
		double value = 0.0;
		if (!s.readInt32u (id) || !s.readDouble (value))
			return false;
		set (id, std::clamp (value, 0.0, 1.0));
	}
	return true;
}

ProcessorBase::ProcessorBase (const ParamTable& t, const FUID& controllerUID) : table (t)
{
	setControllerClass (controllerUID);

	uint32_t maxId = 0;
	for (auto& spec : table)
		maxId = std::max (maxId, spec.id);
	idToIndex.assign (maxId + 1, -1);
	values.resize (table.size ());
	pendingOutputs.reserve (table.size ()); // no allocation on the audio thread
	for (size_t i = 0; i < table.size (); ++i)
	{
		idToIndex[table[i].id] = static_cast<int> (i);
		values[i] = table[i].defaultNormalized ();
		if (table[i].flags & kBypass)
			bypassId = table[i].id;
	}
}

tresult PLUGIN_API ProcessorBase::initialize (FUnknown* context)
{
	auto result = AudioEffect::initialize (context);
	if (result != kResultOk)
		return result;
	addAudioInput (STR16 ("Stereo In"), SpeakerArr::kStereo);
	addAudioOutput (STR16 ("Stereo Out"), SpeakerArr::kStereo);
	return kResultOk;
}

tresult PLUGIN_API ProcessorBase::setBusArrangements (SpeakerArrangement* inputs, int32 numIns,
                                                      SpeakerArrangement* outputs, int32 numOuts)
{
	if (numIns == 1 && numOuts == 1 && inputs[0] == SpeakerArr::kStereo &&
	    outputs[0] == SpeakerArr::kStereo)
		return AudioEffect::setBusArrangements (inputs, numIns, outputs, numOuts);
	return kResultFalse;
}

tresult PLUGIN_API ProcessorBase::canProcessSampleSize (int32 size)
{
	return size == kSample32 ? kResultTrue : kResultFalse;
}

tresult PLUGIN_API ProcessorBase::setupProcessing (ProcessSetup& setup)
{
	auto result = AudioEffect::setupProcessing (setup);
	if (result != kResultOk)
		return result;
	for (auto& d : dry)
		d.assign (static_cast<size_t> (std::max (setup.maxSamplesPerBlock, 1)), 0.0f);
	bypassMix.setup (setup.sampleRate, 30.0);
	bypassMix.reset (bypassed ? 1.0 : 0.0);
	prepare (setup.sampleRate, setup.maxSamplesPerBlock);
	notifyAll ();
	dryLatency = latencySamples ();
	for (auto& r : dryRing)
		r.assign (dryLatency, 0.0f);
	dryRingPos = 0;
	return kResultOk;
}

tresult PLUGIN_API ProcessorBase::setActive (TBool state)
{
	if (state)
	{
		reset ();
		bypassMix.reset (bypassed ? 1.0 : 0.0);
		for (auto& r : dryRing)
			std::fill (r.begin (), r.end (), 0.0f);
		dryRingPos = 0;
	}
	return AudioEffect::setActive (state);
}

void ProcessorBase::fillDry (const float* const* in, int32 n)
{
	if (dryLatency == 0)
	{
		std::memcpy (dry[0].data (), in[0], sizeof (float) * n);
		std::memcpy (dry[1].data (), in[1], sizeof (float) * n);
		return;
	}
	// Delay the dry signal by the plug-in's latency so bypass stays time-aligned.
	size_t pos = dryRingPos;
	for (int32 i = 0; i < n; ++i)
	{
		for (int c = 0; c < 2; ++c)
		{
			dry[c][i] = dryRing[c][pos];
			dryRing[c][pos] = in[c][i];
		}
		if (++pos == dryLatency)
			pos = 0;
	}
	dryRingPos = pos;
}

int ProcessorBase::indexOf (uint32_t id) const
{
	return id < idToIndex.size () ? idToIndex[id] : -1;
}

double ProcessorBase::normalized (uint32_t id) const
{
	auto i = indexOf (id);
	return i >= 0 ? values[i] : 0.0;
}

double ProcessorBase::plain (uint32_t id) const
{
	auto i = indexOf (id);
	return i >= 0 ? table[i].toPlain (values[i]) : 0.0;
}

void ProcessorBase::setNormalizedInternal (uint32_t id, double value, bool notify)
{
	auto i = indexOf (id);
	if (i < 0)
		return;
	values[i] = value;
	if (id == bypassId)
	{
		bypassed = value > 0.5;
		bypassMix.setTarget (bypassed ? 1.0 : 0.0);
	}
	if (notify)
		paramChanged (id, table[i].toPlain (value));
}

void ProcessorBase::setOutputParam (uint32_t id, double plain)
{
	auto i = indexOf (id);
	if (i < 0)
		return;
	const double n = table[i].toNormalized (plain);
	values[i] = n;
	for (auto& p : pendingOutputs)
		if (p.first == id)
		{
			p.second = n;
			return;
		}
	pendingOutputs.emplace_back (id, n);
}

void ProcessorBase::notifyAll ()
{
	for (size_t i = 0; i < table.size (); ++i)
		paramChanged (table[i].id, table[i].toPlain (values[i]));
}

tresult PLUGIN_API ProcessorBase::process (ProcessData& data)
{
	ScopedNoDenormals noDenormals;

	if (auto changes = data.inputParameterChanges)
	{
		const int32 count = changes->getParameterCount ();
		for (int32 i = 0; i < count; ++i)
		{
			auto queue = changes->getParameterData (i);
			if (!queue || queue->getPointCount () <= 0)
				continue;
			int32 offset = 0;
			ParamValue value = 0.0;
			if (queue->getPoint (queue->getPointCount () - 1, offset, value) == kResultTrue)
				setNormalizedInternal (queue->getParameterId (), value, true);
		}
	}

	if (data.numSamples <= 0 || data.numInputs < 1 || data.numOutputs < 1)
		return kResultOk;
	auto& inBus = data.inputs[0];
	auto& outBus = data.outputs[0];
	if (inBus.numChannels < 2 || outBus.numChannels < 2)
		return kResultOk;

	if (data.processContext && (data.processContext->state & ProcessContext::kTempoValid) &&
	    data.processContext->tempo > 0.0)
		tempo = data.processContext->tempo;
	{
		const auto* ctx = data.processContext;
		const bool known = ctx && (ctx->state & ProcessContext::kPlaying) &&
		                   (ctx->state & ProcessContext::kProjectTimeMusicValid);
		ppq = known ? ctx->projectTimeMusic : -1.0;
	}

	const int32 n = data.numSamples;
	float* const* out = outBus.channelBuffers32;
	const float* in[2] = {inBus.channelBuffers32[0], inBus.channelBuffers32[1]};

	const bool fullyBypassed = bypassed && !bypassMix.isSmoothing ();
	const bool fits = static_cast<size_t> (n) <= dry[0].size ();
	if (fullyBypassed && (dryLatency == 0 || !fits))
	{
		for (int c = 0; c < 2; ++c)
			if (out[c] != in[c])
				std::memcpy (out[c], in[c], sizeof (float) * n);
		outBus.silenceFlags = inBus.silenceFlags;
		return kResultOk;
	}

	// Plug-ins with latency keep processing while bypassed so their delay
	// lines hold current audio when they come back.
	const bool crossfading = bypassMix.isSmoothing () && fits;
	if ((crossfading || fullyBypassed || dryLatency > 0) && fits)
		fillDry (in, n);

	processAudio (in, out, n);
	outBus.silenceFlags = 0;

	if (!pendingOutputs.empty ())
	{
		if (auto changes = data.outputParameterChanges)
		{
			for (auto& [id, value] : pendingOutputs)
			{
				int32 index = 0;
				if (auto queue = changes->addParameterData (id, index))
					queue->addPoint (n - 1, value, index);
			}
		}
		pendingOutputs.clear ();
	}

	if (fullyBypassed)
	{
		std::memcpy (out[0], dry[0].data (), sizeof (float) * n);
		std::memcpy (out[1], dry[1].data (), sizeof (float) * n);
		return kResultOk;
	}

	if (crossfading)
	{
		for (int32 i = 0; i < n; ++i)
		{
			const float b = static_cast<float> (bypassMix.next ());
			out[0][i] += (dry[0][i] - out[0][i]) * b;
			out[1][i] += (dry[1][i] - out[1][i]) * b;
		}
	}
	return kResultOk;
}

tresult PLUGIN_API ProcessorBase::setState (IBStream* state)
{
	if (!state)
		return kResultFalse;
	// Parameters missing from an older state keep their defaults.
	for (size_t i = 0; i < table.size (); ++i)
		values[i] = table[i].defaultNormalized ();
	bool ok = readParamState (state, [this] (uint32_t id, double v) {
		setNormalizedInternal (id, v, false);
	});
	bypassMix.reset (bypassed ? 1.0 : 0.0);
	notifyAll ();
	return ok ? kResultOk : kResultFalse;
}

tresult PLUGIN_API ProcessorBase::getState (IBStream* state)
{
	if (!state)
		return kResultFalse;
	return writeParamState (state, table, [this] (uint32_t id) { return normalized (id); })
	           ? kResultOk
	           : kResultFalse;
}

} // namespace suite
