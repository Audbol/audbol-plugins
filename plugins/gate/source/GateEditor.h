#pragma once

#include "GateController.h"

#include "suite/ui/EditorBase.h"
#include "suite/ui/Meters.h"

namespace gate {

class GateCurveView;

class GateEditor : public suite::ui::EditorBase
{
public:
	explicit GateEditor (GateController* controller);

protected:
	void build (VSTGUI::CFrame* frame) override;
	void tick () override;
	void teardown () override;

private:
	GateController* gateCtrl () const { return static_cast<GateController*> (ctrl ()); }

	suite::ui::DynamicsDisplays meters;            // IN / GR / OUT
	suite::ui::DynamicsHistory* history {nullptr}; // key level vs threshold
	GateCurveView* curve {nullptr};
	suite::ui::Knob* ratioKnob {nullptr};
	suite::ui::Knob* hystKnob {nullptr};
	float dotDb {-120.0f};
};

} // namespace gate
