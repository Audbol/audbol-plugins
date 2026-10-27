#pragma once

#include "CompParams.h"

#include "suite/vst/ControllerBase.h"
#include "suite/vst/DynamicsTap.h"

namespace comp {

class CompController : public suite::ControllerBase
{
public:
	CompController ();
	static Steinberg::FUnknown* create (void*);

	// Returns and clears the meter values accumulated since the last call.
	suite::DynamicsSnapshot takeMeters () { return meters.take (); }

protected:
	Steinberg::IPlugView* createEditor () override;
	void onTapBlock (const suite::TapBlockView& block) override { meters.accumulate (block); }

private:
	suite::DynamicsMeterFeed meters;
};

} // namespace comp
