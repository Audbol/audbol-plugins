#pragma once

#include "LoudParams.h"

#include "suite/vst/ControllerBase.h"

#include <array>
#include <vector>

namespace loud {

struct LoudnessValues
{
	float momentary {-70}, shortTerm {-70}, integrated {-70}, range {0};
	float truePeakMax {-150}, truePeakRecent {-150};
};

class LoudController : public suite::ControllerBase
{
public:
	static constexpr int kHistory = 600; // 60 s at 10 updates per second

	LoudController ();
	static Steinberg::FUnknown* create (void*);

	const LoudnessValues& latest () const { return values; }
	bool hasData () const { return received; }
	// Momentary / short-term history, oldest first.
	void copyHistory (std::vector<float>& momentary, std::vector<float>& shortTerm) const;

	// Editor button: pulse the Reset parameter.
	void requestReset ();

protected:
	Steinberg::IPlugView* createEditor () override;
	void onTapBlock (const suite::TapBlockView& block) override;

private:
	LoudnessValues values;
	bool received {false};
	std::array<float, kHistory> histM {}, histS {};
	int histPos {0};
};

} // namespace loud
