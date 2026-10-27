#pragma once

#include "ScopeParams.h"

#include "suite/vst/ControllerBase.h"

#include <vector>

namespace scope {

class ScopeController : public suite::ControllerBase
{
public:
	static constexpr double kHistorySeconds = 10.0;

	ScopeController ();
	static Steinberg::FUnknown* create (void*);

	double sampleRate () const { return rate; }
	double frequency () const { return freq; }
	size_t capacity () const { return left.size (); }

	// The newest `count` samples (oldest first); count is clamped to capacity.
	void copyRecent (size_t count, std::vector<float>& l, std::vector<float>& r, std::vector<float>& ppq) const;

protected:
	Steinberg::IPlugView* createEditor () override;
	void onTapBlock (const suite::TapBlockView& block) override;

private:
	std::vector<float> left, right, songPos;
	size_t writePos {0};
	double rate {48000.0};
	double freq {0.0};
};

} // namespace scope
