#pragma once

#include "DessController.h"

#include "suite/ui/EditorBase.h"
#include "suite/ui/Meters.h"

namespace dess {

class BandView;

class DessEditor : public suite::ui::EditorBase
{
public:
	explicit DessEditor (DessController* controller);

protected:
	void build (VSTGUI::CFrame* frame) override;
	void tick () override;
	void teardown () override;

private:
	DessController* dessCtrl () const { return static_cast<DessController*> (ctrl ()); }

	suite::ui::DynamicsDisplays meters;   // IN / GR / OUT
	suite::ui::DynamicsHistory* history {nullptr}; // sidechain level vs threshold
	BandView* bandView {nullptr};
	suite::ui::Knob* qKnob {nullptr};
};

} // namespace dess
