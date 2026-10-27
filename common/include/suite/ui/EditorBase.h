#pragma once

#include "suite/ui/Controls.h"
#include "suite/vst/ControllerBase.h"

#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#include "public.sdk/source/vst/vstguieditor.h"
#include "vstgui/lib/cvstguitimer.h"

#include <vector>

namespace suite::ui {

// Code-built VSTGUI editor. Plug-ins override build() to lay out widgets
// and call bind() for each parameter control; the base keeps controls and
// parameters in sync (host automation, presets, undo) and handles text
// entry, HiDPI scaling and the UI refresh timer.
class EditorBase : public Steinberg::Vst::VSTGUIEditor,
                   public VSTGUI::IControlListener,
                   public ParamBridge,
                   public Steinberg::IPlugViewContentScaleSupport
{
public:
	EditorBase (ControllerBase* controller, int width, int height);
	~EditorBase () override;

	bool PLUGIN_API open (void* parent, const VSTGUI::PlatformType& platformType) override;
	void PLUGIN_API close () override;
	Steinberg::tresult PLUGIN_API onSize (Steinberg::ViewRect* newSize) override;

	// VSTGUIEditorInterface: only forward gestures for real parameters.
	void beginEdit (VSTGUI_INT32 index) override;
	void endEdit (VSTGUI_INT32 index) override;

	// IControlListener
	void valueChanged (VSTGUI::CControl* control) override;

	// ParamBridge
	std::string formatValue (int32_t tag, double normalized) override;
	std::string paramName (int32_t tag) override;
	int32_t stepCount (int32_t tag) override;
	std::vector<std::string> choiceLabels (int32_t tag) override;
	void openTextEntry (VSTGUI::CControl* control) override;

	// IPlugViewContentScaleSupport
	Steinberg::tresult PLUGIN_API setContentScaleFactor (ScaleFactor factor) override;

	OBJ_METHODS (EditorBase, VSTGUIEditor)
	DEFINE_INTERFACES
		DEF_INTERFACE (IPlugViewContentScaleSupport)
	END_DEFINE_INTERFACES (VSTGUIEditor)
	REFCOUNT_METHODS (VSTGUIEditor)

protected:
	virtual void build (VSTGUI::CFrame* frame) = 0;
	virtual void tick () {}     // ~30 Hz while open
	virtual void teardown () {} // before the frame is destroyed

	ControllerBase* ctrl () const;

	// Adds a view to the frame and returns it.
	template <typename T>
	T* add (T* view)
	{
		frame->addView (view);
		return view;
	}

	// Binds a control to a parameter (or re-binds it to another one).
	template <typename T>
	T* bind (T* control, int32_t tag)
	{
		bindControl (control, tag);
		return control;
	}
	void bindControl (VSTGUI::CControl* control, int32_t tag);

private:
	void syncControls ();
	void finishTextEntry (bool commit);

	std::vector<VSTGUI::CControl*> bound;
	VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer> timer;
	VSTGUI::CCoord baseWidth;
	VSTGUI::CCoord baseHeight;
	double scale {1.0};

	VSTGUI::CTextEdit* textEntry {nullptr};
	int32_t textEntryTag {-1};
};

} // namespace suite::ui
