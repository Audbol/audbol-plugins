#pragma once

#include "TransParams.h"

#include "suite/vst/ControllerBase.h"

namespace trans {

struct ShapeSnapshot
{
	float inPeak {0};
	float outPeak {0};
	float gainMax {0}; // dB, most boost since the last take
	float gainMin {0}; // dB, most cut since the last take
	bool fresh {false};
};

class TransController : public suite::ControllerBase
{
public:
	TransController ();
	static Steinberg::FUnknown* create (void*);

	// Returns and clears the values accumulated since the last call.
	ShapeSnapshot takeMeters ()
	{
		auto s = acc;
		acc = {};
		return s;
	}

protected:
	Steinberg::IPlugView* createEditor () override;
	void onTapBlock (const suite::TapBlockView& block) override;

private:
	ShapeSnapshot acc;
};

} // namespace trans
