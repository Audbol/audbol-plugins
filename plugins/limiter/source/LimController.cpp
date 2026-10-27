#include "LimController.h"

#include "LimEditor.h"

using namespace Steinberg;

namespace lim {

LimController::LimController () : ControllerBase (params (), {}, &lim::presets ()) {}

FUnknown* LimController::create (void*)
{
	return static_cast<Vst::IEditController*> (new LimController);
}

IPlugView* LimController::createEditor () { return new LimEditor (this); }

} // namespace lim
