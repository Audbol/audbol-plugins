#pragma once

#include "RevController.h"

#include "suite/ui/EditorBase.h"
#include "suite/ui/Meters.h"

namespace rev {

class DecayView;
class ToneView;

class RevEditor : public suite::ui::EditorBase
{
public:
	explicit RevEditor (RevController* controller);

protected:
	void build (VSTGUI::CFrame* frame) override;
	void tick () override;
	void teardown () override;

private:
	RevController* revCtrl () const { return static_cast<RevController*> (ctrl ()); }

	DecayView* decayView {nullptr};
	ToneView* toneView {nullptr};
	suite::ui::LevelMeter* inMeter {nullptr};
	suite::ui::LevelMeter* outMeter {nullptr};
};

} // namespace rev
