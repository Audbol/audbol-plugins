#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace suite {

// How a normalized [0, 1] value maps onto the plain range.
enum class Taper : uint8_t
{
	Linear,
	Log,    // min * (max / min)^n, requires min > 0
	Power,  // min + n^curve * (max - min)
	Choice, // integer index in [0, choices.size() - 1]
	Toggle, // 0 or 1
};

// How a plain value is shown to (and parsed from) the user.
enum class Unit : uint8_t
{
	None,
	Hz,
	Decibel,
	Percent,
	Milliseconds,
	Ratio,
	Semitones,
	Degrees,
	Choice,
	OnOff,
	Pan, // -100..100 shown as "L 30" / "C" / "R 30"
	Lufs,
	LoudnessUnits,
	DbTruePeak,
};

enum ParamFlags : uint32_t
{
	kAutomatable = 1 << 0,
	kBypass = 1 << 1,
	kHidden = 1 << 2,
	kReadOnly = 1 << 3,
};

// Declarative description of one parameter. The processor, controller and
// UI all derive their behaviour from the same table, so mapping, display
// and parsing can never drift apart.
struct ParamSpec
{
	uint32_t id;
	std::string name;
	std::string shortName;
	Taper taper;
	Unit unit;
	double min;
	double max;
	double def; // plain units
	std::vector<std::string> choices {};
	double curve {1.0};
	uint32_t flags {kAutomatable};
	int32_t unitId {0}; // VST3 unit (parameter group); 0 = root

	int32_t stepCount () const;
	double toPlain (double normalized) const;
	double toNormalized (double plain) const;
	double defaultNormalized () const { return toNormalized (def); }

	std::string format (double plain) const;
	bool parse (const std::string& text, double& plainOut) const;
};

using ParamTable = std::vector<ParamSpec>;

const ParamSpec* findSpec (const ParamTable& table, uint32_t id);

// --- Convenience builders -------------------------------------------------
ParamSpec makeLinear (uint32_t id, std::string name, Unit unit, double min, double max, double def);
ParamSpec makeLog (uint32_t id, std::string name, Unit unit, double min, double max, double def);
ParamSpec makePower (uint32_t id, std::string name, Unit unit, double min, double max, double def,
                     double curve);
ParamSpec makeChoice (uint32_t id, std::string name, std::vector<std::string> choices, int def);
ParamSpec makeToggle (uint32_t id, std::string name, bool def);
ParamSpec makeBypass (uint32_t id);

} // namespace suite
