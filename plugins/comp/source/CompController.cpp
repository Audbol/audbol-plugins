#include "CompController.h"

#include "CompEditor.h"

using namespace Steinberg;

namespace comp {

CompController::CompController () : ControllerBase (params ()) {}

FUnknown* CompController::create (void*)
{
	return static_cast<Vst::IEditController*> (new CompController);
}

IPlugView* CompController::createEditor () { return new CompEditor (this); }

} // namespace comp
