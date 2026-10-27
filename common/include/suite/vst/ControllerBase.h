#pragma once

#include "suite/ParamSpec.h"
#include "suite/Presets.h"
#include "suite/vst/AudioTap.h"

#include "public.sdk/source/vst/utility/dataexchange.h"
#include "public.sdk/source/vst/vsteditcontroller.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace suite {

struct UnitSpec
{
	int32_t id;
	std::string name;
};

// Base for every suite edit controller: builds parameters (and parameter
// groups) from the spec table, mirrors the processor state, receives tap
// audio from the processor and persists UI-only settings.
class ControllerBase : public Steinberg::Vst::EditControllerEx1,
                       public Steinberg::Vst::IDataExchangeReceiver
{
public:
	ControllerBase (const ParamTable& table, std::vector<UnitSpec> units = {},
	                const PresetList* presets = nullptr);

	Steinberg::tresult PLUGIN_API initialize (Steinberg::FUnknown* context) override;
	Steinberg::tresult PLUGIN_API setComponentState (Steinberg::IBStream* state) override;
	Steinberg::tresult PLUGIN_API setState (Steinberg::IBStream* state) override;
	Steinberg::tresult PLUGIN_API getState (Steinberg::IBStream* state) override;
	Steinberg::IPlugView* PLUGIN_API createView (Steinberg::FIDString name) override;
	Steinberg::tresult PLUGIN_API notify (Steinberg::Vst::IMessage* message) override;

	// IDataExchangeReceiver
	void PLUGIN_API queueOpened (Steinberg::Vst::DataExchangeUserContextID userContextID,
	                             Steinberg::uint32 blockSize,
	                             Steinberg::TBool& dispatchOnBackgroundThread) override;
	void PLUGIN_API queueClosed (Steinberg::Vst::DataExchangeUserContextID userContextID) override;
	void PLUGIN_API onDataExchangeBlocksReceived (
	    Steinberg::Vst::DataExchangeUserContextID userContextID, Steinberg::uint32 numBlocks,
	    Steinberg::Vst::DataExchangeBlock* blocks, Steinberg::TBool onBackgroundThread) override;

	const ParamTable& getTable () const { return table; }
	const ParamSpec* spec (uint32_t id) const { return findSpec (table, id); }

	double getPlain (uint32_t id);
	// Full begin/perform/end gesture for programmatic edits from the UI.
	void editPlain (uint32_t id, double plain);
	void editNormalized (uint32_t id, double normalized);

	// UI-only settings (zoom, selected tab, display ranges...). Saved with the
	// controller state, never automated.
	double uiValue (const std::string& key, double fallback) const;
	void setUiValue (const std::string& key, double value) { uiState[key] = value; }

	// Factory presets (empty for plug-ins without them)
	const PresetList& presets () const;
	void applyPreset (int index);  // as one undoable host edit per parameter
	int currentPreset () const;    // -1 when none has been loaded
	bool presetModified () const;  // current values differ from the loaded preset

	OBJ_METHODS (ControllerBase, EditControllerEx1)
	DEFINE_INTERFACES
		DEF_INTERFACE (IDataExchangeReceiver)
	END_DEFINE_INTERFACES (EditControllerEx1)
	REFCOUNT_METHODS (EditControllerEx1)

protected:
	virtual Steinberg::IPlugView* createEditor () = 0;
	// Audio from the processor's AudioTap, delivered on the main thread.
	virtual void onTapBlock (const TapBlockView& block) {}

	const ParamTable& table;

private:
	std::vector<UnitSpec> units;
	const PresetList* presetList {nullptr};
	std::map<std::string, double> uiState;
	Steinberg::Vst::DataExchangeReceiverHandler dataExchange {this};
};

} // namespace suite
