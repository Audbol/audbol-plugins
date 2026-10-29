#pragma once

#include "TransController.h"

#include "suite/ui/EditorBase.h"
#include "suite/ui/Meters.h"

namespace trans {

class TransientView;

class TransEditor : public suite::ui::EditorBase
{
public:
	explicit TransEditor (TransController* controller);

protected:
	void build (VSTGUI::CFrame* frame) override;
	void tick () override;
	void teardown () override;

private:
	TransController* transCtrl () const { return static_cast<TransController*> (ctrl ()); }

	TransientView* view {nullptr};
	suite::ui::LevelMeter* inMeter {nullptr};
	suite::ui::LevelMeter* outMeter {nullptr};
};

} // namespace trans
