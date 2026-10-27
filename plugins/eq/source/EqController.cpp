#include "EqController.h"

#include "EqEditor.h"

#include <string>

using namespace Steinberg;

namespace eq {

namespace {
std::vector<suite::UnitSpec> bandUnits ()
{
	std::vector<suite::UnitSpec> units;
	for (int b = 0; b < kNumBands; ++b)
		units.push_back ({b + 1, "Band " + std::to_string (b + 1)});
	return units;
}
} // namespace

EqController::EqController () : ControllerBase (params (), bandUnits ()) {}

FUnknown* EqController::create (void*)
{
	return static_cast<Vst::IEditController*> (new EqController);
}

IPlugView* EqController::createEditor () { return new EqEditor (this); }

void EqController::onTapBlock (const suite::TapBlockView& block)
{
	pre.setSampleRate (block.sampleRate ());
	post.setSampleRate (block.sampleRate ());
	if (block.streams () >= 2)
	{
		pre.push (block.stream (0), static_cast<int> (block.frames ()));
		post.push (block.stream (1), static_cast<int> (block.frames ()));
	}
}

} // namespace eq
