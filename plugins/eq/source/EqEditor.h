#pragma once

#include "EqController.h"

#include "suite/ui/EditorBase.h"

namespace eq {

class EqGraph;
class BandTab;

class EqEditor : public suite::ui::EditorBase
{
public:
	explicit EqEditor (EqController* controller);

	void selectBand (int band);
	EqController* eqCtrl () const { return static_cast<EqController*> (ctrl ()); }

protected:
	void build (VSTGUI::CFrame* frame) override;
	void tick () override;
	void teardown () override;

private:
	void updateBandControls ();

	EqGraph* graph {nullptr};
	BandTab* tabs[kNumBands] {};
	suite::ui::Toggle* bandOn {nullptr};
	suite::ui::ChoiceMenu* bandType {nullptr};
	suite::ui::ChoiceMenu* bandSlope {nullptr};
	suite::ui::ChoiceMenu* bandPlacement {nullptr};
	suite::ui::Knob* bandFreq {nullptr};
	suite::ui::Knob* bandGain {nullptr};
	suite::ui::Knob* bandQ {nullptr};
	suite::ui::Label* bandTitle {nullptr};
	int selected {0};
};

} // namespace eq
