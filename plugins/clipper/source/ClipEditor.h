#pragma once

#include "ClipController.h"

#include "suite/ui/EditorBase.h"
#include "suite/ui/Meters.h"

namespace clip {

class ClipCurveView;

class ClipEditor : public suite::ui::EditorBase
{
public:
	explicit ClipEditor (ClipController* controller);

protected:
	void build (VSTGUI::CFrame* frame) override;
	void tick () override;
	void teardown () override;

private:
	ClipController* clipCtrl () const { return static_cast<ClipController*> (ctrl ()); }

	suite::ui::DynamicsDisplays displays;
	ClipCurveView* curve {nullptr};
	suite::ui::Knob* outputKnob {nullptr};
	suite::ui::Label* clipReadout {nullptr};
	suite::ui::Label* peakReadout {nullptr};
	float dotPeak {0};
	float heldClip {0}, heldPeak {0};
	float holdAge {0};
};

} // namespace clip
