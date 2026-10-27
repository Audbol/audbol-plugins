#include "suite/vst/ControllerBase.h"

#include "suite/vst/ProcessorBase.h"
#include "suite/vst/SpecParameter.h"

#include "base/source/fstreamer.h"
#include "public.sdk/source/vst/utility/stringconvert.h"

#include <cmath>
#include <cstring>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace suite {

namespace {
constexpr uint32 kUiStateMagic = 0x48535555; // 'HSUU'
}

ControllerBase::ControllerBase (const ParamTable& t, std::vector<UnitSpec> u, const PresetList* p)
: table (t), units (std::move (u)), presetList (p)
{
}

const PresetList& ControllerBase::presets () const
{
	static const PresetList none;
	return presetList ? *presetList : none;
}

void ControllerBase::applyPreset (int index)
{
	const auto& list = presets ();
	if (index < 0 || index >= static_cast<int> (list.size ()))
		return;
	for (auto& [id, normalized] : resolvePreset (table, list[index]))
		editNormalized (id, normalized);
	setUiValue ("preset", index);
}

int ControllerBase::currentPreset () const
{
	const int i = static_cast<int> (uiValue ("preset", -1.0));
	return i >= 0 && i < static_cast<int> (presets ().size ()) ? i : -1;
}

bool ControllerBase::presetModified () const
{
	const int i = currentPreset ();
	if (i < 0)
		return false;
	auto* self = const_cast<ControllerBase*> (this); // getParamNormalized isn't const
	for (auto& [id, normalized] : resolvePreset (table, presets ()[i]))
		if (std::abs (self->getParamNormalized (id) - normalized) > 1e-5)
			return true;
	return false;
}

tresult PLUGIN_API ControllerBase::initialize (FUnknown* context)
{
	auto result = EditControllerEx1::initialize (context);
	if (result != kResultOk)
		return result;

	for (auto& u : units)
	{
		String128 name;
		StringConvert::convert (u.name, name);
		addUnit (new Steinberg::Vst::Unit (name, u.id, kRootUnitId));
	}
	for (auto& spec : table)
		parameters.addParameter (new SpecParameter (spec));
	return kResultOk;
}

tresult PLUGIN_API ControllerBase::setComponentState (IBStream* state)
{
	if (!state)
		return kResultFalse;
	for (auto& spec : table)
		setParamNormalized (spec.id, spec.defaultNormalized ());
	return readParamState (state, [this] (uint32_t id, double v) { setParamNormalized (id, v); })
	           ? kResultOk
	           : kResultFalse;
}

tresult PLUGIN_API ControllerBase::setState (IBStream* state)
{
	IBStreamer s (state, kLittleEndian);
	uint32 magic = 0, count = 0;
	if (!s.readInt32u (magic) || magic != kUiStateMagic || !s.readInt32u (count) || count > 1000)
		return kResultFalse;
	for (uint32 i = 0; i < count; ++i)
	{
		uint32 len = 0;
		if (!s.readInt32u (len) || len > 256)
			return kResultFalse;
		std::string key (len, '\0');
		if (len > 0 && s.readRaw (key.data (), len) != static_cast<int32> (len))
			return kResultFalse;
		double value = 0.0;
		if (!s.readDouble (value))
			return kResultFalse;
		uiState[key] = value;
	}
	return kResultOk;
}

tresult PLUGIN_API ControllerBase::getState (IBStream* state)
{
	IBStreamer s (state, kLittleEndian);
	s.writeInt32u (kUiStateMagic);
	s.writeInt32u (static_cast<uint32> (uiState.size ()));
	for (auto& [key, value] : uiState)
	{
		s.writeInt32u (static_cast<uint32> (key.size ()));
		s.writeRaw (key.data (), static_cast<int32> (key.size ()));
		s.writeDouble (value);
	}
	return kResultOk;
}

IPlugView* PLUGIN_API ControllerBase::createView (FIDString name)
{
	if (name && std::strcmp (name, ViewType::kEditor) == 0)
		return createEditor ();
	return nullptr;
}

tresult PLUGIN_API ControllerBase::notify (IMessage* message)
{
	if (dataExchange.onMessage (message))
		return kResultTrue;
	return EditControllerEx1::notify (message);
}

void PLUGIN_API ControllerBase::queueOpened (DataExchangeUserContextID, uint32,
                                             TBool& dispatchOnBackgroundThread)
{
	dispatchOnBackgroundThread = false; // UI consumes it directly
}

void PLUGIN_API ControllerBase::queueClosed (DataExchangeUserContextID) {}

void PLUGIN_API ControllerBase::onDataExchangeBlocksReceived (DataExchangeUserContextID,
                                                              uint32 numBlocks,
                                                              DataExchangeBlock* blocks, TBool)
{
	for (uint32 i = 0; i < numBlocks; ++i)
	{
		auto view = TapBlockView::from (blocks[i]);
		if (view.valid ())
			onTapBlock (view);
	}
}

double ControllerBase::getPlain (uint32_t id)
{
	auto s = spec (id);
	return s ? s->toPlain (getParamNormalized (id)) : 0.0;
}

void ControllerBase::editNormalized (uint32_t id, double v)
{
	beginEdit (id);
	setParamNormalized (id, v);
	performEdit (id, getParamNormalized (id));
	endEdit (id);
}

void ControllerBase::editPlain (uint32_t id, double plain)
{
	if (auto s = spec (id))
		editNormalized (id, s->toNormalized (plain));
}

double ControllerBase::uiValue (const std::string& key, double fallback) const
{
	auto it = uiState.find (key);
	return it != uiState.end () ? it->second : fallback;
}

} // namespace suite
