// Headless CPU benchmark: loads a VST3, feeds it stereo noise in 512-sample
// blocks and reports processing time as a percentage of real time (one core).
//
// usage: bench plugin.vst3 [seconds=20] [sampleRates=48000,96000] [--fuzz]
//
// --fuzz also changes random parameters to random values every block and
// checks every output sample for NaN / inf / runaway level (> +100 dBFS).
//
// --render in.wav out.wav "Title=normalized;..." processes a file offline at
// its own sample rate and writes the output as 32-bit float (used by
// tools/bench/offline_tests.ps1 for sample-accurate checks).

#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "public.sdk/source/vst/vstpresetfile.h"

#include <array>
#include <filesystem>
#include <cmath>
#include "public.sdk/source/vst/hosting/plugprovider.h"
#include "public.sdk/source/vst/utility/stringconvert.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"

#include <chrono>
#include <cstdio>
#include <cstring>
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

// --tail: 2 s of noise, then silence. CPU during the last half of the
// silence vs. under signal shows denormal slowdowns in decaying tails.
struct TailResult
{
	double loudCpu {0}, silentCpu {0};
	double lastPeak {0}; // output peak in the final second
	bool finite {true};
};

static double benchmark (IComponent* component, IAudioProcessor* processor, double sr, double seconds,
                         const std::vector<ParamID>* fuzzParams = nullptr, FuzzResult* fuzz = nullptr,
                         TailResult* tail = nullptr)
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
	std::chrono::duration<double> busy {0}, loud {0}, silent {0};
	const int loudBlocks = static_cast<int> (2.0 * sr / kBlock);
	const int lastSecond = blocks - static_cast<int> (sr / kBlock);
	for (int b = 0; b < blocks; ++b)
	{
		// Fuzz input: noise bursts, silence and full-scale material
		float level = fuzz ? std::array<float, 4> {0.0f, 0.3f, 1.0f, 3.0f}[b / 50 % 4] : 1.0f;
		if (tail)
			level = b < loudBlocks ? 1.0f : 0.0f;
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
		// Fuzz block sizes like hosts do: anything from a 0-sample parameter
		// flush to a full block.
		data.numSamples = kBlock;
		if (fuzz)
		{
			const double r = unit (rng);
			data.numSamples = r < 0.05 ? 0 : r < 0.15 ? 1 : r < 0.5 ? 1 + static_cast<int32> (unit (rng) * (kBlock - 1)) : kBlock;
		}
		ctx.projectTimeMusic = b * kBlock / sr * 2.0; // 120 BPM
		const auto t0 = std::chrono::steady_clock::now ();
		processor->process (data);
		const auto spent = std::chrono::steady_clock::now () - t0;
		busy += spent;
		if (tail)
		{
			if (b < loudBlocks)
				loud += spent;
			else if (b >= (loudBlocks + blocks) / 2)
				silent += spent;
			for (int i = 0; i < kBlock; ++i)
				for (float v : {outL[i], outR[i]})
				{
					if (!std::isfinite (v))
						tail->finite = false;
					else if (b >= lastSecond)
						tail->lastPeak = std::max (tail->lastPeak, double (std::abs (v)));
				}
		}
		if (fuzz)
		{
			for (int i = 0; i < data.numSamples; ++i)
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
	if (tail)
	{
		tail->loudCpu = 100.0 * loud.count () / (loudBlocks * kBlock / sr);
		const int silentBlocks = blocks - (loudBlocks + blocks) / 2;
		tail->silentCpu = 100.0 * silent.count () / (silentBlocks * kBlock / sr);
	}
	return 100.0 * busy.count () / (blocks * kBlock / sr);
}

// Output level of the plug-in with its current state for program-level
// pink-ish noise (~ -18 dBFS RMS). Returns false if any sample was NaN/inf.
static bool measure (IComponent* component, IAudioProcessor* processor, double seconds, double& peakDb, double& rmsDb)
{
	constexpr int32 kBlock = 512;
	constexpr double sr = 48000.0;
	ProcessSetup setup {kRealtime, kSample32, kBlock, sr};
	processor->setupProcessing (setup);
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

	std::mt19937 rng (7);
	std::uniform_real_distribution<float> white (-1.0f, 1.0f);
	float b0 = 0, b1 = 0, b2 = 0;
	double peak = 0, energy = 0;
	long long count = 0;
	bool finite = true;
	const int blocks = static_cast<int> (seconds * sr / kBlock);
	for (int b = 0; b < blocks; ++b)
	{
		for (int i = 0; i < kBlock; ++i)
		{
			const float w = white (rng);
			b0 = 0.99765f * b0 + w * 0.099046f;
			b1 = 0.963f * b1 + w * 0.2965164f;
			b2 = 0.57f * b2 + w * 1.0526913f;
			inL[i] = inR[i] = (b0 + b1 + b2 + w * 0.1848f) * 0.04f;
		}
		ctx.projectTimeMusic = b * kBlock / sr * 2.0;
		processor->process (data);
		if (b < blocks / 4)
			continue; // let tails, envelopes and auto gain settle
		for (int i = 0; i < kBlock; ++i)
			for (float v : {outL[i], outR[i]})
			{
				if (!std::isfinite (v))
				{
					finite = false;
					continue;
				}
				peak = std::max (peak, double (std::abs (v)));
				energy += double (v) * v;
				++count;
			}
	}
	processor->setProcessing (false);
	component->setActive (false);
	peakDb = peak > 0 ? 20.0 * std::log10 (peak) : -150.0;
	rmsDb = energy > 0 ? 10.0 * std::log10 (energy / count) : -150.0;
	return finite;
}

// --- --render: process a WAV file offline --------------------------------

struct Wav
{
	double sampleRate {48000};
	std::vector<float> l, r;
};

// Reads 16-bit PCM or 32-bit float WAV, mono or stereo.
static bool readWav (const std::string& path, Wav& wav)
{
	FILE* f = std::fopen (path.c_str (), "rb");
	if (!f)
		return false;
	std::vector<char> bytes;
	char buf[65536];
	for (size_t n; (n = std::fread (buf, 1, sizeof (buf), f)) > 0;)
		bytes.insert (bytes.end (), buf, buf + n);
	std::fclose (f);
	auto u16 = [&] (size_t o) { return uint16_t (uint8_t (bytes[o]) | uint8_t (bytes[o + 1]) << 8); };
	auto u32 = [&] (size_t o) { return uint32_t (u16 (o) | uint32_t (u16 (o + 2)) << 16); };
	int format = 0, channels = 0, bits = 0;
	for (size_t o = 12; o + 8 <= bytes.size ();)
	{
		const std::string id (&bytes[o], 4);
		const size_t size = u32 (o + 4);
		if (id == "fmt ")
		{
			format = u16 (o + 8);
			channels = u16 (o + 10);
			wav.sampleRate = u32 (o + 12);
			bits = u16 (o + 22);
		}
		else if (id == "data" && channels > 0)
		{
			const size_t frameBytes = size_t (channels) * bits / 8;
			const size_t frames = std::min (size, bytes.size () - o - 8) / frameBytes;
			wav.l.resize (frames);
			wav.r.resize (frames);
			for (size_t i = 0; i < frames; ++i)
				for (int c = 0; c < 2; ++c)
				{
					const size_t at = o + 8 + i * frameBytes + size_t (std::min (c, channels - 1)) * bits / 8;
					float v = 0.0f;
					if (format == 3 && bits == 32)
						std::memcpy (&v, &bytes[at], 4);
					else if (format == 1 && bits == 16)
						v = int16_t (u16 (at)) / 32768.0f;
					else
						return false;
					(c == 0 ? wav.l : wav.r)[i] = v;
				}
			return true;
		}
		o += 8 + size + (size & 1);
	}
	return false;
}

static bool writeWav (const std::string& path, const Wav& wav)
{
	FILE* f = std::fopen (path.c_str (), "wb");
	if (!f)
		return false;
	auto w32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
	auto w16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
	const uint32_t data = uint32_t (wav.l.size () * 8);
	std::fwrite ("RIFF", 1, 4, f);
	w32 (36 + data);
	std::fwrite ("WAVEfmt ", 1, 8, f);
	w32 (16);
	w16 (3); // IEEE float
	w16 (2);
	w32 (uint32_t (wav.sampleRate));
	w32 (uint32_t (wav.sampleRate) * 8);
	w16 (8);
	w16 (32);
	std::fwrite ("data", 1, 4, f);
	w32 (data);
	for (size_t i = 0; i < wav.l.size (); ++i)
	{
		std::fwrite (&wav.l[i], 4, 1, f);
		std::fwrite (&wav.r[i], 4, 1, f);
	}
	std::fclose (f);
	return true;
}

// settings: "Title=normalized;Title=normalized", applied before the first sample.
static int render (IComponent* component, IAudioProcessor* processor, IEditController* controller,
                   const std::string& inPath, const std::string& outPath, const std::string& settings)
{
	Wav wav;
	if (!readWav (inPath, wav))
	{
		std::printf ("cannot read %s\n", inPath.c_str ());
		return 4;
	}
	ParameterChanges changes (64);
	std::stringstream ss (settings);
	for (std::string s; std::getline (ss, s, ';');)
	{
		const auto eq = s.find ('=');
		if (eq == std::string::npos)
			continue;
		const std::string title = s.substr (0, eq);
		const double value = std::atof (s.substr (eq + 1).c_str ());
		bool found = false;
		for (int32 i = 0; controller && i < controller->getParameterCount (); ++i)
		{
			ParameterInfo pi {};
			if (controller->getParameterInfo (i, pi) == kResultOk && StringConvert::convert (pi.title) == title)
			{
				int32 index = 0;
				if (auto q = changes.addParameterData (pi.id, index))
					q->addPoint (0, value, index);
				controller->setParamNormalized (pi.id, value);
				found = true;
			}
		}
		if (!found)
		{
			std::printf ("unknown parameter '%s'\n", title.c_str ());
			return 5;
		}
	}

	constexpr int32 kBlock = 512;
	ProcessSetup setup {kOffline, kSample32, kBlock, wav.sampleRate};
	processor->setupProcessing (setup);
	component->activateBus (kAudio, kInput, 0, true);
	component->activateBus (kAudio, kOutput, 0, true);
	component->setActive (true);
	processor->setProcessing (true);

	Wav out;
	out.sampleRate = wav.sampleRate;
	out.l.resize (wav.l.size ());
	out.r.resize (wav.r.size ());
	AudioBusBuffers inBus {}, outBus {};
	inBus.numChannels = outBus.numChannels = 2;
	ProcessContext ctx {};
	ctx.sampleRate = wav.sampleRate;
	ctx.tempo = 120.0;
	ctx.state = ProcessContext::kPlaying | ProcessContext::kTempoValid | ProcessContext::kProjectTimeMusicValid;
	ProcessData data {};
	data.processMode = kOffline;
	data.symbolicSampleSize = kSample32;
	data.numInputs = data.numOutputs = 1;
	data.inputs = &inBus;
	data.outputs = &outBus;
	data.processContext = &ctx;
	data.inputParameterChanges = &changes;
	for (size_t pos = 0; pos < wav.l.size (); pos += kBlock)
	{
		data.numSamples = static_cast<int32> (std::min<size_t> (kBlock, wav.l.size () - pos));
		float* inPtr[2] = {&wav.l[pos], &wav.r[pos]};
		float* outPtr[2] = {&out.l[pos], &out.r[pos]};
		inBus.channelBuffers32 = inPtr;
		outBus.channelBuffers32 = outPtr;
		ctx.projectTimeMusic = pos / wav.sampleRate * 2.0;
		processor->process (data);
		if (pos == 0)
		{
			changes.clearQueue ();
			data.inputParameterChanges = nullptr;
		}
	}
	processor->setProcessing (false);
	component->setActive (false);
	if (!writeWav (outPath, out))
	{
		std::printf ("cannot write %s\n", outPath.c_str ());
		return 6;
	}
	return 0;
}

int main (int argc, char** argv)
{
	if (argc < 2)
	{
		std::printf ("usage: bench plugin.vst3 [seconds] [rates]\n");
		return 1;
	}
	std::string renderIn, renderOut, renderSettings; // --render in.wav out.wav "Title=norm;..."
	bool fuzzMode = false;
	bool tailMode = false; // --tail: noise then 20 s of silence (denormals, decay)
	std::string presetDir; // --presets <dir>: load each .vstpreset and check its output
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
		else if (arg == "--tail")
		{
			tailMode = true;
			consumed = 1;
		}
		else if (arg == "--render" && a + 3 < argc)
		{
			renderIn = argv[a + 1];
			renderOut = argv[a + 2];
			renderSettings = argv[a + 3];
			consumed = 4;
		}
		else if (arg == "--presets" && a + 1 < argc)
		{
			presetDir = argv[a + 1];
			consumed = 2;
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
		if (!renderIn.empty ())
		{
			auto controller = provider->getController ();
			const int result = render (component, processor, controller, renderIn, renderOut, renderSettings);
			if (controller)
				controller->release ();
			component->release ();
			PluginContextFactory::instance ().setPluginContext (nullptr);
			return result;
		}
		if (!presetDir.empty ())
		{
			// Input is ~-18 dBFS RMS noise; flag invalid, clipping or silent output.
			auto controller = provider->getController ();
			int problems = 0, count = 0;
			for (auto& entry : std::filesystem::directory_iterator (std::filesystem::u8path (presetDir)))
			{
				if (entry.path ().extension () != ".vstpreset")
					continue;
				auto stream = owned (FileStream::open (entry.path ().string ().c_str (), "rb"));
				const bool loaded = stream && PresetFile::loadPreset (stream, FUID::fromTUID (info.ID ().data ()),
				                                                     component, controller);
				double peak = 0, rms = 0;
				const bool finite = loaded && measure (component, processor, 4.0, peak, rms);
				const char* verdict = !loaded ? "LOAD FAILED" : !finite ? "NaN/inf"
				                    : peak > 6.0                     ? "CLIPS"
				                    : rms < -60.0                    ? "SILENT"
				                                                     : "ok";
				if (std::string (verdict) != "ok")
					++problems;
				++count;
				std::printf ("  %-26s peak %+6.1f dBFS  rms %+6.1f dBFS  %s\n",
				             entry.path ().stem ().u8string ().c_str (), peak, rms, verdict);
			}
			std::printf ("%s: %d presets, %d problems\n", info.name ().c_str (), count, problems);
			if (controller)
				controller->release ();
			component->release ();
			break;
		}
		std::printf ("%-24s", info.name ().c_str ());
		if (tailMode)
		{
			bool bad = false;
			for (double sr : rates)
			{
				TailResult t;
				benchmark (component, processor, sr, 22.0, nullptr, nullptr, &t);
				// Silence costing much more than signal means denormals.
				const bool slow = t.silentCpu > 1.5 * t.loudCpu && t.silentCpu > 0.5;
				bad = bad || slow || !t.finite;
				std::printf ("  %6.0f Hz: signal %5.2f %%  silence %5.2f %%  tail %+6.0f dBFS%s", sr, t.loudCpu,
				             t.silentCpu, t.lastPeak > 0 ? 20.0 * std::log10 (t.lastPeak) : -150.0,
				             !t.finite ? " NaN/inf" : slow ? " SLOW" : "");
			}
			std::printf ("%s\n", bad ? "  PROBLEM" : "");
		}
		else if (!fuzzMode)
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
