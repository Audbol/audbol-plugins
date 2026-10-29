#pragma once

#include "GateParams.h"

#include "suite/vst/ControllerBase.h"
#include "suite/vst/DynamicsTap.h"

namespace gate {

struct GateMeters
{
	suite::DynamicsSnapshot dyn;
	float keyPeak {0}; // linear, key (detector) level
};

class GateController : public suite::ControllerBase
{
public:
	GateController ();
	static Steinberg::FUnknown* create (void*);

	// Returns and clears the meter values accumulated since the last call.
	GateMeters takeMeters ();

protected:
	Steinberg::IPlugView* createEditor () override;
	void onTapBlock (const suite::TapBlockView& block) override;

private:
	suite::DynamicsMeterFeed meters;
	float keyPeak {0};
};

} // namespace gate
