#pragma once

#include "SatController.h"

#include "suite/ui/EditorBase.h"
#include "suite/ui/Meters.h"

namespace sat {

class CurveView;
class HarmonicsView;

class SatEditor : public suite::ui::EditorBase
{
public:
	explicit SatEditor (SatController* controller);

protected:
	void build (VSTGUI::CFrame* frame) override;
	void tick () override;
	void teardown () override;

private:
	SatController* satCtrl () const { return static_cast<SatController*> (ctrl ()); }

	CurveView* curve {nullptr};
	HarmonicsView* harmonics {nullptr};
	suite::ui::LevelMeter* inMeter {nullptr};
	suite::ui::LevelMeter* outMeter {nullptr};
	suite::ui::Knob* outputKnob {nullptr};
	float inputPeak {0.0f};
};

} // namespace sat
