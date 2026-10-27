#pragma once

#include "suite/ParamSpec.h"
#include "suite/Presets.h"
#include "suite/dsp/Svf.h"

#include "pluginterfaces/base/funknown.h"

namespace eq {

// Class IDs (never change these once released: hosts store them in projects)
static const Steinberg::FUID kProcessorUID (0xA7E9D2A4, 0x34974D75, 0xB28FAE86, 0xDCF145EB);
static const Steinberg::FUID kControllerUID (0x103FF70B, 0xD8E94210, 0x9C01263C, 0x929E6F00);

constexpr int kNumBands = 8;

enum GlobalParam : uint32_t
{
	kBypass = 0,
	kOutputGain = 1,
	kGainScale = 2,
	kAnalyzerMode = 3,
	kDisplayRange = 4,
};

enum BandParam : uint32_t
{
	kBandOn = 0,
	kBandType,
	kBandFreq,
	kBandGain,
	kBandQ,
	kBandSlope,
	kBandPlacement,
};

constexpr uint32_t kBandBase = 100;
constexpr uint32_t kBandStride = 20;

constexpr uint32_t bandParam (int band, BandParam p) { return kBandBase + band * kBandStride + p; }
constexpr bool isBandParam (uint32_t id) { return id >= kBandBase && id < kBandBase + kNumBands * kBandStride; }
constexpr int bandOf (uint32_t id) { return static_cast<int> ((id - kBandBase) / kBandStride); }
constexpr BandParam bandParamOf (uint32_t id) { return static_cast<BandParam> ((id - kBandBase) % kBandStride); }

enum class Placement : int
{
	Stereo,
	Left,
	Right,
	Mid,
	Side
};

enum class AnalyzerMode : int
{
	Off,
	Pre,
	Post,
	Both
};

constexpr double kMinFreq = 10.0;
constexpr double kMaxFreq = 30000.0;
constexpr double kMaxGain = 30.0;
constexpr double kMinQ = 0.1;
constexpr double kMaxQ = 40.0;
constexpr double kDisplayRanges[] = {6.0, 12.0, 24.0, 36.0};

const suite::ParamTable& params ();
const suite::PresetList& presets ();

} // namespace eq
