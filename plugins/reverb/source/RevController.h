#pragma once

#include "RevParams.h"

#include "suite/dsp/SpectrumAnalyzer.h"
#include "suite/vst/ControllerBase.h"

namespace rev {

struct LevelSnapshot
{
	float inPeak {0};
	float outPeak {0};
	bool fresh {false};
};

class RevController : public suite::ControllerBase
{
public:
	RevController ();
	static Steinberg::FUnknown* create (void*);

	LevelSnapshot takeLevels ()
	{
		auto s = levels;
		levels = {};
		return s;
	}
	suite::dsp::SpectrumAnalyzer& wetAnalyzer () { return wet; }

protected:
	Steinberg::IPlugView* createEditor () override;
	void onTapBlock (const suite::TapBlockView& block) override;

private:
	LevelSnapshot levels;
	suite::dsp::SpectrumAnalyzer wet;
};

} // namespace rev
