#pragma once

#include "DlyController.h"

#include "suite/ui/EditorBase.h"
#include "suite/ui/Meters.h"

namespace dly {

class EchoView;
class FilterView;

class DlyEditor : public suite::ui::EditorBase
{
public:
	explicit DlyEditor (DlyController* controller);

protected:
	void build (VSTGUI::CFrame* frame) override;
	void tick () override;
	void teardown () override;

private:
	DlyController* dlyCtrl () const { return static_cast<DlyController*> (ctrl ()); }
	static void setEnabled (VSTGUI::CView* v, bool enabled);

	EchoView* echoView {nullptr};
	FilterView* filterView {nullptr};
	suite::ui::LevelMeter* inMeter {nullptr};
	suite::ui::LevelMeter* outMeter {nullptr};
	suite::ui::Knob* timeL {nullptr};
	suite::ui::Knob* timeR {nullptr};
	suite::ui::ChoiceMenu* noteL {nullptr};
	suite::ui::ChoiceMenu* noteR {nullptr};
	suite::ui::Toggle* linkToggle {nullptr};
};

} // namespace dly
