#include "ClipController.h"

#include "ClipEditor.h"

using namespace Steinberg;

namespace clip {

ClipController::ClipController () : ControllerBase (params (), {}, &clip::presets ()) {}

FUnknown* ClipController::create (void*)
{
	return static_cast<Vst::IEditController*> (new ClipController);
}

IPlugView* ClipController::createEditor () { return new ClipEditor (this); }

} // namespace clip
