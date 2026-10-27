#pragma once

#include "DlyParams.h"

#include "suite/vst/ControllerBase.h"

namespace dly {

struct LevelSnapshot
{
	float inPeak {0};
	float outPeak {0};
	bool fresh {false};
};

class DlyController : public suite::ControllerBase
{
public:
	DlyController ();
	static Steinberg::FUnknown* create (void*);

	LevelSnapshot takeLevels ()
	{
		auto s = levels;
		levels = {};
		return s;
	}
	// Host tempo as last reported by the processor (120 until audio runs).
	double tempo () const { return hostTempo; }

	// Current left/right delay times in ms, as the processor computes them.
	void currentTimesMs (double& left, double& right);

protected:
	Steinberg::IPlugView* createEditor () override;
	void onTapBlock (const suite::TapBlockView& block) override;

private:
	LevelSnapshot levels;
	double hostTempo {120.0};
};

} // namespace dly
