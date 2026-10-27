#include "suite/vst/SpecParameter.h"

#include "public.sdk/source/vst/utility/stringconvert.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace suite {

SpecParameter::SpecParameter (const ParamSpec& s) : spec (s)
{
	info.id = spec.id;
	StringConvert::convert (spec.name, info.title);
	StringConvert::convert (spec.shortName, info.shortTitle);
	info.units[0] = 0; // formatted strings already carry their unit
	info.stepCount = spec.stepCount ();
	info.defaultNormalizedValue = spec.defaultNormalized ();
	info.unitId = spec.unitId;
	info.flags = 0;
	if (spec.flags & kAutomatable)
		info.flags |= ParameterInfo::kCanAutomate;
	if (spec.flags & kBypass)
		info.flags |= ParameterInfo::kIsBypass;
	if (spec.flags & kHidden)
		info.flags |= ParameterInfo::kIsHidden;
	if (spec.flags & kReadOnly)
		info.flags |= ParameterInfo::kIsReadOnly;
	if (spec.taper == Taper::Choice)
		info.flags |= ParameterInfo::kIsList;
	valueNormalized = info.defaultNormalizedValue;
}

void SpecParameter::toString (ParamValue normalized, String128 string) const
{
	StringConvert::convert (spec.format (spec.toPlain (normalized)), string);
}

bool SpecParameter::fromString (const TChar* string, ParamValue& normalized) const
{
	double plain = 0.0;
	if (!spec.parse (StringConvert::convert (string), plain))
		return false;
	normalized = spec.toNormalized (plain);
	return true;
}

ParamValue SpecParameter::toPlain (ParamValue normalized) const { return spec.toPlain (normalized); }

ParamValue SpecParameter::toNormalized (ParamValue plain) const { return spec.toNormalized (plain); }

} // namespace suite
