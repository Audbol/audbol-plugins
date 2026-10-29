#include "TransParams.h"

#include "suite/Presets.h"

using suite::PresetList;

namespace trans {

namespace {

PresetList build ()
{
	return {
	    // Drums
	    {"Kick Punch", "Drums", {{kAttack, 50}, {kSustain, -10}, {kAttackTime, 20}, {kSustainTime, 200}}},
	    {"Kick Click", "Drums", {{kAttack, 60}, {kAttackTime, 5}, {kSustainTime, 150}}},
	    {"Snare Crack", "Drums", {{kAttack, 45}, {kSustain, 10}, {kAttackTime, 12}, {kSustainTime, 250}}},
	    {"Snare Ring Control", "Drums", {{kSustain, -50}, {kAttackTime, 15}, {kSustainTime, 300}}},
	    {"Tight Toms", "Drums", {{kAttack, 25}, {kSustain, -45}, {kAttackTime, 15}, {kSustainTime, 400}}},
	    {"Room Bloom", "Drums", {{kAttack, -20}, {kSustain, 60}, {kAttackTime, 20}, {kSustainTime, 600}}},
	    {"Drum Bus Snap", "Drums", {{kAttack, 30}, {kAttackTime, 15}, {kSustainTime, 250}, {kMix, 70}}},
	    {"Dry Drums", "Drums", {{kAttack, 10}, {kSustain, -70}, {kAttackTime, 15}, {kSustainTime, 200}}},
	    // Instruments
	    {"Acoustic Strum Pop", "Instruments", {{kAttack, 30}, {kAttackTime, 10}, {kSustainTime, 250}}},
	    {"Soften Picking", "Instruments", {{kAttack, -40}, {kAttackTime, 8}, {kSustainTime, 200}}},
	    {"Bass Pluck", "Instruments", {{kAttack, 35}, {kSustain, -15}, {kAttackTime, 25}, {kSustainTime, 300}}},
	    {"Piano Sustain", "Instruments", {{kSustain, 40}, {kAttackTime, 20}, {kSustainTime, 900}}},
	    // Creative
	    {"Reverse Feel", "Creative", {{kAttack, -100}, {kSustain, 80}, {kAttackTime, 40}, {kSustainTime, 800}}},
	    {"Gated Snap", "Creative", {{kAttack, 70}, {kSustain, -100}, {kAttackTime, 10}, {kSustainTime, 120}}},
	};
}

} // namespace

const PresetList& presets ()
{
	static const PresetList list = build ();
	return list;
}

} // namespace trans
