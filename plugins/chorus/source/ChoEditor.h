#pragma once

#include "ChoController.h"

#include "suite/ui/EditorBase.h"
#include "suite/ui/Meters.h"

namespace cho {

class LfoView;
class ResponseView;

class ChoEditor : public suite::ui::EditorBase
{
public:
	explicit ChoEditor (ChoController* controller);

protected:
	void build (VSTGUI::CFrame* frame) override;
	void tick () override;
	void teardown () override;

private:
	ChoController* choCtrl () const { return static_cast<ChoController*> (ctrl ()); }

	LfoView* lfoView {nullptr};
	ResponseView* responseView {nullptr};
	suite::ui::LevelMeter* inMeter {nullptr};
	suite::ui::LevelMeter* outMeter {nullptr};
};

} // namespace cho
