#pragma once

#include "LimController.h"

#include "suite/ui/EditorBase.h"
#include "suite/ui/Meters.h"

namespace lim {

class LimEditor : public suite::ui::EditorBase
{
public:
	explicit LimEditor (LimController* controller);

protected:
	void build (VSTGUI::CFrame* frame) override;
	void tick () override;
	void teardown () override;

private:
	LimController* limCtrl () const { return static_cast<LimController*> (ctrl ()); }

	suite::ui::DynamicsDisplays displays;
	suite::ui::Label* grReadout {nullptr};
	suite::ui::Label* peakReadout {nullptr};
	float maxGr {0.0f};
	float maxOutDb {-120.0f};
};

} // namespace lim
