#pragma once

#include "LimParams.h"

#include "suite/vst/ControllerBase.h"
#include "suite/vst/DynamicsTap.h"

namespace lim {

class LimController : public suite::ControllerBase
{
public:
	LimController ();
	static Steinberg::FUnknown* create (void*);

	suite::DynamicsSnapshot takeMeters () { return meters.take (); }

protected:
	Steinberg::IPlugView* createEditor () override;
	void onTapBlock (const suite::TapBlockView& block) override { meters.accumulate (block); }

private:
	suite::DynamicsMeterFeed meters;
};

} // namespace lim
