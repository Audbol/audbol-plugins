#pragma once

#include "DessParams.h"

#include "suite/dsp/SpectrumAnalyzer.h"
#include "suite/vst/ControllerBase.h"
#include "suite/vst/DynamicsTap.h"

namespace dess {

struct DessMeters
{
	suite::DynamicsSnapshot dyn;
	float scPeak {0}; // linear, sidechain (filtered) level
};

class DessController : public suite::ControllerBase
{
public:
	DessController ();
	static Steinberg::FUnknown* create (void*);

	// Returns and clears the meter values accumulated since the last call.
	DessMeters takeMeters ();
	suite::dsp::SpectrumAnalyzer& analyzer () { return spectrum; }

protected:
	Steinberg::IPlugView* createEditor () override;
	void onTapBlock (const suite::TapBlockView& block) override;

private:
	suite::DynamicsMeterFeed meters;
	float scPeak {0};
	suite::dsp::SpectrumAnalyzer spectrum;
};

} // namespace dess
