#pragma once

#include "LoudController.h"

#include "suite/ui/EditorBase.h"
#include "suite/ui/Meters.h"

#include <vector>

namespace loud {

class ReadoutTile;
class LoudnessHistory;

class LoudEditor : public suite::ui::EditorBase
{
public:
	explicit LoudEditor (LoudController* controller);

protected:
	void build (VSTGUI::CFrame* frame) override;
	void tick () override;
	void teardown () override;

private:
	LoudController* loudCtrl () const { return static_cast<LoudController*> (ctrl ()); }

	ReadoutTile* integrated {nullptr};
	ReadoutTile* shortTerm {nullptr};
	ReadoutTile* momentary {nullptr};
	ReadoutTile* range {nullptr};
	ReadoutTile* truePeak {nullptr};
	LoudnessHistory* history {nullptr};
	suite::ui::LevelMeter* meterM {nullptr};
	suite::ui::LevelMeter* meterS {nullptr};
	std::vector<float> histM, histS;
};

} // namespace loud
