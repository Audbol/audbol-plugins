#include "suite/Presets.h"

namespace suite {

bool isPresetParam (const ParamSpec& spec)
{
	return !(spec.flags & (kBypass | kHidden | kReadOnly));
}

std::vector<std::pair<uint32_t, double>> resolvePreset (const ParamTable& table, const Preset& preset)
{
	std::vector<std::pair<uint32_t, double>> out;
	for (auto& spec : table)
	{
		if (!isPresetParam (spec))
			continue;
		double plain = spec.def;
		for (auto& [id, value] : preset.values)
			if (id == spec.id)
				plain = value;
		out.emplace_back (spec.id, spec.toNormalized (plain));
	}
	return out;
}

} // namespace suite
