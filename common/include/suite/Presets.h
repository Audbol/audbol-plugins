#pragma once

#include "suite/ParamSpec.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace suite {

// A factory preset: values in plain units for the parameters it sets. Every
// other preset-able parameter goes back to its default, so a preset always
// produces the same sound regardless of what was loaded before.
struct Preset
{
	std::string name;
	std::string category;
	std::vector<std::pair<uint32_t, double>> values; // id, plain value
};

using PresetList = std::vector<Preset>;

// Bypass, hidden (UI-only) and read-only (meter) parameters are never part of
// a preset.
bool isPresetParam (const ParamSpec& spec);

// Normalized value of every preset-able parameter for this preset.
std::vector<std::pair<uint32_t, double>> resolvePreset (const ParamTable& table, const Preset& preset);

} // namespace suite
