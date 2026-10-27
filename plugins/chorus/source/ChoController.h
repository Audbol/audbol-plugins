#pragma once

#include "ChoParams.h"

#include "suite/vst/ControllerBase.h"

namespace cho {

struct LevelSnapshot
{
	float inPeak {0};
	float outPeak {0};
	bool fresh {false};
};

class ChoController : public suite::ControllerBase
{
public:
	ChoController ();
	static Steinberg::FUnknown* create (void*);

	LevelSnapshot takeLevels ()
	{
		auto s = levels;
		levels = {};
		return s;
	}
	// Master LFO phase (0..1) from the processor; < 0 until audio runs.
	double lfoPhase () const { return phase; }

protected:
	Steinberg::IPlugView* createEditor () override;
	void onTapBlock (const suite::TapBlockView& block) override;

private:
	LevelSnapshot levels;
	double phase {-1.0};
};

} // namespace cho
