#pragma once

#include "ClipParams.h"

#include "suite/vst/ControllerBase.h"
#include "suite/vst/DynamicsTap.h"

namespace clip {

class ClipController : public suite::ControllerBase
{
public:
	ClipController ();
	static Steinberg::FUnknown* create (void*);

	// Returns and clears the meter values accumulated since the last call.
	suite::DynamicsSnapshot takeMeters () { return meters.take (); }

protected:
	Steinberg::IPlugView* createEditor () override;
	void onTapBlock (const suite::TapBlockView& block) override { meters.accumulate (block); }

private:
	suite::DynamicsMeterFeed meters;
};

} // namespace clip
