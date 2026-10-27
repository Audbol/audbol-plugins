#include "EqParams.h"

#include "suite/Presets.h"

using suite::Preset;
using suite::PresetList;

namespace eq {

namespace {

using V = std::vector<std::pair<uint32_t, double>>;
enum T
{
	Bell,
	LowShelf,
	HighShelf,
	LowCut,
	HighCut,
	Notch,
	BandPass,
	Tilt
};
enum Slope
{
	s6,
	s12,
	s18,
	s24,
	s36,
	s48
};
enum Place
{
	Stereo,
	Left,
	Right,
	Mid,
	Side
};

// One enabled band.
V band (int b, T type, double freq, double gain = 0.0, double q = 0.707, int slope = s24, int place = Stereo)
{
	return {{bandParam (b, kBandOn), 1.0},       {bandParam (b, kBandType), double (type)},
	        {bandParam (b, kBandFreq), freq},    {bandParam (b, kBandGain), gain},
	        {bandParam (b, kBandQ), q},          {bandParam (b, kBandSlope), double (slope)},
	        {bandParam (b, kBandPlacement), double (place)}};
}

Preset make (const char* name, const char* category, std::initializer_list<V> bands)
{
	Preset p {name, category, {}};
	for (auto& b : bands)
		p.values.insert (p.values.end (), b.begin (), b.end ());
	return p;
}

PresetList build ()
{
	return {
	    // Vocals
	    make ("Vocal Presence", "Vocals",
	          {band (0, LowCut, 90), band (2, Bell, 250, -2.5, 1.2), band (4, Bell, 3000, 3.0, 1.0),
	           band (6, HighShelf, 10000, 2.5)}),
	    make ("Vocal De-Mud", "Vocals",
	          {band (0, LowCut, 100), band (2, Bell, 300, -4.0, 1.5), band (3, Bell, 600, -2.0, 2.0)}),
	    make ("Airy Vocal", "Vocals",
	          {band (0, LowCut, 80), band (4, Bell, 5500, -1.5, 3.0), band (6, HighShelf, 12000, 4.0)}),
	    make ("Radio Voice", "Vocals",
	          {band (0, LowCut, 400, 0, 0.707, s36), band (3, Bell, 1500, 4.0, 1.2),
	           band (7, HighCut, 4000, 0, 0.707, s36)}),
	    // Drums
	    make ("Kick Punch", "Drums",
	          {band (1, Bell, 60, 4.0, 1.2), band (2, Bell, 350, -5.0, 1.5), band (5, Bell, 4000, 3.0, 1.5),
	           band (7, HighCut, 12000, 0, 0.707, s12)}),
	    make ("Snare Crack", "Drums",
	          {band (0, LowCut, 80), band (2, Bell, 200, 2.0, 1.5), band (3, Bell, 900, -3.0, 2.0),
	           band (5, Bell, 5000, 3.5, 1.2)}),
	    make ("Overheads Shine", "Drums",
	          {band (0, LowCut, 200, 0, 0.707, s18), band (5, Bell, 3500, -1.5, 2.0), band (6, HighShelf, 9000, 3.0)}),
	    make ("Drum Bus Weight", "Drums",
	          {band (1, LowShelf, 80, 3.0), band (3, Bell, 400, -2.0, 1.0), band (6, HighShelf, 8000, 2.0)}),
	    // Bass & Guitars
	    make ("Bass Definition", "Bass & Guitars",
	          {band (0, LowCut, 30), band (1, Bell, 80, 2.0, 1.0), band (2, Bell, 250, -3.0, 1.5),
	           band (4, Bell, 800, 2.5, 1.5)}),
	    make ("Guitar Bite", "Bass & Guitars",
	          {band (0, LowCut, 100), band (3, Bell, 400, -2.5, 1.2), band (5, Bell, 2500, 3.0, 1.2),
	           band (7, HighCut, 10000, 0, 0.707, s12)}),
	    make ("Acoustic Sparkle", "Bass & Guitars",
	          {band (0, LowCut, 80), band (2, Bell, 200, -3.0, 1.2), band (6, HighShelf, 8000, 3.5)}),
	    // Mix & Master
	    make ("Gentle Smile", "Mix & Master", {band (1, LowShelf, 100, 2.0), band (6, HighShelf, 10000, 2.0)}),
	    make ("Mix Bus Polish", "Mix & Master",
	          {band (0, LowCut, 25), band (2, Bell, 250, -1.0, 0.8), band (6, HighShelf, 12000, 1.5)}),
	    make ("Warm Tilt", "Mix & Master", {band (4, Tilt, 1000, -3.0)}),
	    make ("Bright Tilt", "Mix & Master", {band (4, Tilt, 1000, 3.0)}),
	    make ("Wider Sides", "Mix & Master",
	          {band (0, LowCut, 150, 0, 0.707, s24, Side), band (5, Bell, 3000, 2.5, 0.8, s24, Side)}),
	    // Corrective
	    make ("Rumble Filter", "Corrective", {band (0, LowCut, 40, 0, 0.707, s48)}),
	    make ("Hum Remover 50 Hz", "Corrective",
	          {band (1, Notch, 50, 0, 10.0), band (2, Notch, 100, 0, 10.0), band (3, Notch, 150, 0, 10.0)}),
	    make ("Hum Remover 60 Hz", "Corrective",
	          {band (1, Notch, 60, 0, 10.0), band (2, Notch, 120, 0, 10.0), band (3, Notch, 180, 0, 10.0)}),
	    make ("Telephone", "Corrective",
	          {band (0, LowCut, 300, 0, 0.707, s36), band (3, Bell, 1000, 3.0, 1.0),
	           band (7, HighCut, 3400, 0, 0.707, s36)}),
	};
}

} // namespace

const PresetList& presets ()
{
	static const PresetList list = build ();
	return list;
}

} // namespace eq
