// Headless CPU benchmark: loads a VST3, feeds it stereo noise in 512-sample
// blocks and reports processing time as a percentage of real time (one core).
//
// usage: bench plugin.vst3 [seconds=20] [sampleRates=48000,96000] [--fuzz]
//
// --fuzz also changes random parameters to random values every block and
// checks every output sample for NaN / inf / runaway level (> +100 dBFS).

#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"

#include <array>
#include <cmath>
#include "public.sdk/source/vst/hosting/plugprovider.h"
#include "public.sdk/source/vst/utility/stringconvert.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"

#include <chrono>
#include <cstdio>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace Steinberg;
using namespace Steinberg::Vst;

struct FuzzResult
{
	long long badSamples {0};
	double worstPeak {0};
};

static double benchmark (IComponent* component, IAudioProcessor* processor, double sr, double seconds,
                         const std::vector<ParamID>* fuzzParams = nullptr, FuzzResult* fuzz = nullptr)
{
	constexpr int32 kBlock = 512;
	ProcessSetup setup {kRealtime, kSample32, kBlock, sr};
	if (processor->setupProcessing (setup) != kResultOk)
		return -1.0;
	component->activateBus (kAudio, kInput, 0, true);
	component->activateBus (kAudio, kOutput, 0, true);
	component->setActive (true);
	processor->setProcessing (true);

	std::vector<float> inL (kBlock), inR (kBlock), outL (kBlock), outR (kBlock);
	float* inPtr[2] = {inL.data (), inR.data ()};
	float* outPtr[2] = {outL.data (), outR.data ()};
	AudioBusBuffers inBus {}, outBus {};
	inBus.numChannels = outBus.numChannels = 2;
	inBus.channelBuffers32 = inPtr;
	outBus.channelBuffers32 = outPtr;
	ProcessContext ctx {};
	ctx.sampleRate = sr;
	ctx.tempo = 120.0;
	ctx.state = ProcessContext::kPlaying | ProcessContext::kTempoValid | ProcessContext::kProjectTimeMusicValid;

	ProcessData data {};
	data.processMode = kRealtime;
	data.symbolicSampleSize = kSample32;
	data.numSamples = kBlock;
	data.numInputs = data.numOutputs = 1;
	data.inputs = &inBus;
	data.outputs = &outBus;
	data.processContext = &ctx;

	std::mt19937 rng (1);
	std::uniform_real_distribution<float> noise (-0.3f, 0.3f);
	std::uniform_real_distribution<double> unit (0.0, 1.0);
	ParameterChanges changes (64);
	const int blocks = static_cast<int> (seconds * sr / kBlock);
	std::chrono::duration<double> busy {0};
	for (int b = 0; b < blocks; ++b)
	{
		// Fuzz input: noise bursts, silence and full-scale material
		const float level = fuzz ? std::array<float, 4> {0.0f, 0.3f, 1.0f, 3.0f}[b / 50 % 4] : 1.0f;
		for (int i = 0; i < kBlock; ++i)
		{
			inL[i] = noise (rng) * level;
			inR[i] = noise (rng) * level;
		}
		changes.clearQueue ();
		data.inputParameterChanges = nullptr;
		if (fuzzParams && !fuzzParams->empty ())
		{
			const int count = static_cast<int> (unit (rng) * 4.0);
			for (int c = 0; c < count; ++c)
			{
				const ParamID id = (*fuzzParams)[static_cast<size_t> (unit (rng) * fuzzParams->size ()) % fuzzParams->size ()];
				int32 index = 0;
				if (auto q = changes.addParameterData (id, index))
					q->addPoint (0, unit (rng) < 0.3 ? (unit (rng) < 0.5 ? 0.0 : 1.0) : unit (rng), index);
			}
			data.inputParameterChanges = &changes;
		}
		ctx.projectTimeMusic = b * kBlock / sr * 2.0; // 120 BPM
		const auto t0 = std::chrono::steady_clock::now ();
		processor->process (data);
		busy += std::chrono::steady_clock::now () - t0;
		if (fuzz)
		{
			for (int i = 0; i < kBlock; ++i)
				for (float v : {outL[i], outR[i]})
				{
					// NaN/inf, or runaway beyond any legitimate gain (+100 dBFS)
					if (!std::isfinite (v) || std::abs (v) > 1e5f)
						++fuzz->badSamples;
					else
						fuzz->worstPeak = std::max (fuzz->worstPeak, double (std::abs (v)));
				}
		}
	}
	processor->setProcessing (false);
	component->setActive (false);
	return 100.0 * busy.count () / (blocks * kBlock / sr);
}

int main (int argc, char** argv)
{
	if (argc < 2)
	{
		std::printf ("usage: bench plugin.vst3 [seconds] [rates]\n");
		return 1;
	}
	bool fuzzMode = false;
	std::vector<std::string> exclude; // parameter titles containing any of these aren't fuzzed
	for (int a = 1; a < argc;)
	{
		const std::string arg = argv[a];
		int consumed = 0;
		if (arg == "--fuzz")
		{
			fuzzMode = true;
			consumed = 1;
		}
		else if (arg == "--exclude" && a + 1 < argc)
		{
			std::stringstream ss (argv[a + 1]);
			for (std::string s; std::getline (ss, s, ',');)
				exclude.push_back (s);
			consumed = 2;
		}
		if (!consumed)
		{
			++a;
			continue;
		}
		for (int b = a; b + consumed < argc; ++b)
			argv[b] = argv[b + consumed];
		argc -= consumed;
	}
	const double seconds = argc > 2 ? std::atof (argv[2]) : 20.0;
	std::vector<double> rates {48000.0, 96000.0};
	if (argc > 3)
	{
		rates.clear ();
		std::stringstream ss (argv[3]);
		for (std::string s; std::getline (ss, s, ',');)
			rates.push_back (std::atof (s.c_str ()));
	}

	HostApplication host;
	PluginContextFactory::instance ().setPluginContext (&host);
	std::string error;
	auto module = VST3::Hosting::Module::create (argv[1], error);
	if (!module)
	{
		std::printf ("load failed: %s\n", error.c_str ());
		return 2;
	}
	auto factory = module->getFactory ();
	for (auto& info : factory.classInfos ())
	{
		if (info.category () != kVstAudioEffectClass)
			continue;
		auto provider = owned (new PlugProvider (factory, info, true));
		if (!provider->initialize ())
			return 3;
		auto component = provider->getComponent ();
		auto processor = U::cast<IAudioProcessor> (component);
		std::printf ("%-24s", info.name ().c_str ());
		if (!fuzzMode)
		{
			for (double sr : rates)
				std::printf ("  %6.0f Hz: %6.2f %%", sr, benchmark (component, processor, sr, seconds));
			std::printf ("\n");
		}
		else
		{
			// Every writable parameter is a fuzz target
			std::vector<ParamID> ids;
			if (auto controller = provider->getController ())
			{
				for (int32 i = 0; i < controller->getParameterCount (); ++i)
				{
					ParameterInfo pi {};
					if (controller->getParameterInfo (i, pi) != kResultOk || (pi.flags & ParameterInfo::kIsReadOnly))
						continue;
					const std::string title = StringConvert::convert (pi.title);
					bool skip = false;
					for (auto& e : exclude)
						skip = skip || title.find (e) != std::string::npos;
					if (!skip)
						ids.push_back (pi.id);
				}
				controller->release ();
			}
			long long bad = 0;
			for (double sr : rates)
			{
				FuzzResult r;
				benchmark (component, processor, sr, seconds, &ids, &r);
				std::printf ("  %6.0f Hz: %s (peak %+.1f dBFS)", sr, r.badSamples ? "FAIL" : "ok",
				             r.worstPeak > 0 ? 20.0 * std::log10 (r.worstPeak) : -150.0);
				bad += r.badSamples;
			}
			std::printf ("  [%zu params]%s\n", ids.size (), bad ? "  BAD SAMPLES" : "");
		}
		component->release ();
		break;
	}
	PluginContextFactory::instance ().setPluginContext (nullptr);
	return 0;
}
