#pragma once

#include "ScopeController.h"

#include "suite/ui/EditorBase.h"

namespace scope {

class ScopeView;

class ScopeEditor : public suite::ui::EditorBase
{
public:
	explicit ScopeEditor (ScopeController* controller);

protected:
	void build (VSTGUI::CFrame* frame) override;
	void tick () override;
	void teardown () override;

private:
	ScopeController* scopeCtrl () const { return static_cast<ScopeController*> (ctrl ()); }

	ScopeView* view {nullptr};
	suite::ui::Knob* timeKnob {nullptr};
	suite::ui::Knob* levelKnob {nullptr};
	suite::ui::ChoiceMenu* beatMenu {nullptr};
	suite::ui::Label* freqReadout {nullptr};
	suite::ui::Label* peakReadout {nullptr};
};

} // namespace scope
