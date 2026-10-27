#pragma once

#include "EqParams.h"

#include "suite/dsp/SpectrumAnalyzer.h"
#include "suite/vst/ControllerBase.h"

namespace eq {

class EqController : public suite::ControllerBase
{
public:
	EqController ();
	static Steinberg::FUnknown* create (void*);

	suite::dsp::SpectrumAnalyzer& preAnalyzer () { return pre; }
	suite::dsp::SpectrumAnalyzer& postAnalyzer () { return post; }
	double sampleRate () const { return post.getSampleRate (); }

protected:
	Steinberg::IPlugView* createEditor () override;
	void onTapBlock (const suite::TapBlockView& block) override;

private:
	suite::dsp::SpectrumAnalyzer pre;
	suite::dsp::SpectrumAnalyzer post;
};

} // namespace eq
