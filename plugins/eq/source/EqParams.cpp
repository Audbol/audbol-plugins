#include "EqParams.h"

#include <string>

using namespace suite;

namespace eq {

namespace {

struct BandDefault
{
	dsp::FilterType type;
	double freq;
	double q;
};

constexpr BandDefault kDefaults[kNumBands] = {
    {dsp::FilterType::LowCut, 30.0, 0.707},     {dsp::FilterType::LowShelf, 100.0, 0.707},
    {dsp::FilterType::Bell, 250.0, 1.0},        {dsp::FilterType::Bell, 600.0, 1.0},
    {dsp::FilterType::Bell, 1500.0, 1.0},       {dsp::FilterType::Bell, 3500.0, 1.0},
    {dsp::FilterType::HighShelf, 8000.0, 0.707}, {dsp::FilterType::HighCut, 18000.0, 0.707},
};

ParamTable build ()
{
	ParamTable t;
	t.push_back (makeBypass (kBypass));
	t.push_back (makeLinear (kOutputGain, "Output Gain", Unit::Decibel, -24.0, 24.0, 0.0));
	t.back ().shortName = "Output";
	t.push_back (makeLinear (kGainScale, "Gain Scale", Unit::Percent, 0.0, 200.0, 100.0));
	t.back ().shortName = "Scale";
	t.push_back (makeChoice (kAnalyzerMode, "Analyzer", {"Off", "Pre", "Post", "Both"}, 2));
	t.back ().flags = kHidden;
	t.push_back (makeChoice (kDisplayRange, "Display Range", {"6", "12", "24", "36"}, 2));
	t.back ().flags = kHidden;

	for (int b = 0; b < kNumBands; ++b)
	{
		const std::string prefix = "Band " + std::to_string (b + 1) + " ";
		const int unit = b + 1;
		const auto& d = kDefaults[b];

		auto add = [&] (ParamSpec s, const char* shortName) {
			s.shortName = shortName;
			s.unitId = unit;
			t.push_back (std::move (s));
		};
		add (makeToggle (bandParam (b, kBandOn), prefix + "On", false), "On");
		add (makeChoice (bandParam (b, kBandType), prefix + "Type",
		                 {"Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut", "Notch",
		                  "Band Pass", "Tilt Shelf"},
		                 static_cast<int> (d.type)),
		     "Type");
		add (makeLog (bandParam (b, kBandFreq), prefix + "Freq", Unit::Hz, kMinFreq, kMaxFreq, d.freq),
		     "Freq");
		add (makeLinear (bandParam (b, kBandGain), prefix + "Gain", Unit::Decibel, -kMaxGain, kMaxGain, 0.0),
		     "Gain");
		add (makeLog (bandParam (b, kBandQ), prefix + "Q", Unit::None, kMinQ, kMaxQ, d.q), "Q");
		add (makeChoice (bandParam (b, kBandSlope), prefix + "Slope",
		                 {"6 dB/oct", "12 dB/oct", "18 dB/oct", "24 dB/oct", "36 dB/oct",
		                  "48 dB/oct", "72 dB/oct", "96 dB/oct"},
		                 3),
		     "Slope");
		add (makeChoice (bandParam (b, kBandPlacement), prefix + "Stereo",
		                 {"Stereo", "Left", "Right", "Mid", "Side"}, 0),
		     "Stereo");
	}
	return t;
}

} // namespace

const ParamTable& params ()
{
	static const ParamTable table = build ();
	return table;
}

} // namespace eq
