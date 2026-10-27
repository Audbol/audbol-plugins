#pragma once

#include "UtlController.h"

#include "suite/ui/EditorBase.h"
#include "suite/ui/Meters.h"

#include <vector>

namespace utl {

class Goniometer;
class BipolarMeter;

class UtlEditor : public suite::ui::EditorBase
{
public:
	explicit UtlEditor (UtlController* controller);

protected:
	void build (VSTGUI::CFrame* frame) override;
	void tick () override;
	void teardown () override;

private:
	UtlController* utlCtrl () const { return static_cast<UtlController*> (ctrl ()); }

	Goniometer* scope {nullptr};
	BipolarMeter* correlation {nullptr};
	BipolarMeter* balance {nullptr};
	suite::ui::LevelMeter* meters[4] {}; // L, R, M, S
	suite::ui::Knob* bassFreq {nullptr};
	std::vector<float> scopeL, scopeR;
	double corrSmoothed {1.0};
	double balanceSmoothed {0.0};
};

} // namespace utl
