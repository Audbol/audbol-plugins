#pragma once

#include "CompController.h"

#include "suite/ui/EditorBase.h"
#include "suite/ui/Meters.h"

namespace comp {

class TransferView;

class CompEditor : public suite::ui::EditorBase
{
public:
	explicit CompEditor (CompController* controller);

protected:
	void build (VSTGUI::CFrame* frame) override;
	void tick () override;
	void teardown () override;

private:
	CompController* compCtrl () const { return static_cast<CompController*> (ctrl ()); }

	suite::ui::DynamicsDisplays displays;
	TransferView* transfer {nullptr};
	suite::ui::Knob* makeupKnob {nullptr};
	suite::ui::Knob* scFreqKnob {nullptr};
	float dotDb {-120.0f};
};

} // namespace comp
