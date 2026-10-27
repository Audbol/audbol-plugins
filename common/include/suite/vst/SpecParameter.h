#pragma once

#include "suite/ParamSpec.h"

#include "public.sdk/source/vst/vstparameters.h"

namespace suite {

// A VST3 parameter whose mapping and text conversion come from a ParamSpec.
class SpecParameter : public Steinberg::Vst::Parameter
{
public:
	explicit SpecParameter (const ParamSpec& spec);

	void toString (Steinberg::Vst::ParamValue normalized,
	               Steinberg::Vst::String128 string) const override;
	bool fromString (const Steinberg::Vst::TChar* string,
	                 Steinberg::Vst::ParamValue& normalized) const override;
	Steinberg::Vst::ParamValue toPlain (Steinberg::Vst::ParamValue normalized) const override;
	Steinberg::Vst::ParamValue toNormalized (Steinberg::Vst::ParamValue plain) const override;

	const ParamSpec& getSpec () const { return spec; }

private:
	ParamSpec spec;
};

} // namespace suite
