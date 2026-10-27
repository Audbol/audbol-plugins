#pragma once

#include "SatParams.h"

#include "suite/vst/ControllerBase.h"

namespace sat {

struct LevelSnapshot
{
	float inPeak {0};
	float outPeak {0};
	bool fresh {false};
};

class SatController : public suite::ControllerBase
{
public:
	SatController ();
	static Steinberg::FUnknown* create (void*);

	LevelSnapshot takeLevels ()
	{
		auto s = levels;
		levels = {};
		return s;
	}

protected:
	Steinberg::IPlugView* createEditor () override;
	void onTapBlock (const suite::TapBlockView& block) override;

private:
	LevelSnapshot levels;
};

} // namespace sat
