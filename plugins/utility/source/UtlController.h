#pragma once

#include "UtlParams.h"

#include "suite/vst/ControllerBase.h"

#include <vector>

namespace utl {

// Statistics accumulated from the processor's output since the UI last asked.
struct StereoStats
{
	float peakL {0}, peakR {0}, peakM {0}, peakS {0};
	double sumLR {0}, sumLL {0}, sumRR {0};
	bool fresh {false};
};

class UtlController : public suite::ControllerBase
{
public:
	static constexpr int kScopeSize = 2048;

	UtlController ();
	static Steinberg::FUnknown* create (void*);

	StereoStats takeStats ()
	{
		auto s = stats;
		stats = {};
		return s;
	}
	// Most recent output samples, oldest first (goniometer).
	void copyScope (std::vector<float>& l, std::vector<float>& r) const;

protected:
	Steinberg::IPlugView* createEditor () override;
	void onTapBlock (const suite::TapBlockView& block) override;

private:
	StereoStats stats;
	std::vector<float> scopeL, scopeR;
	int scopePos {0};
};

} // namespace utl
