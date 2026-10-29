#include "suite/ui/EditorBase.h"

#include "suite/ui/PresetBar.h"

#include "vstgui/lib/cframe.h"
#include "vstgui/lib/controls/ctextedit.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#if LINUX
#include "base/source/fobject.h"
#include "pluginterfaces/gui/iplugview.h"
#include "vstgui/lib/platform/linux/x11frame.h"
#include <vector>
#endif

using namespace Steinberg;
using namespace VSTGUI;

namespace suite::ui {

namespace {

constexpr int32_t kTextEntryTag = -1000;

#if LINUX
// On Linux VSTGUI has no event loop of its own: it runs its X11 connection
// and timers on the host's (Linux::IRunLoop, offered by the plug frame).
// The SDK's VSTGUIEditor doesn't pass it on, so the frame would open without
// an X connection. This adapter is the one VSTGUI's VST3Editor uses.
class HostRunLoop : public VSTGUI::IRunLoop, public AtomicReferenceCounted
{
public:
	explicit HostRunLoop (FUnknown* plugFrame) : runLoop (plugFrame) {}

	bool registerEventHandler (int fd, VSTGUI::IEventHandler* handler) final
	{
		if (!runLoop)
			return false;
		auto h = Steinberg::owned (new EventHandler ());
		h->handler = handler;
		if (runLoop->registerEventHandler (h, fd) != kResultTrue)
			return false;
		eventHandlers.push_back (h);
		return true;
	}
	bool unregisterEventHandler (VSTGUI::IEventHandler* handler) final
	{
		if (!runLoop)
			return false;
		for (auto it = eventHandlers.begin (); it != eventHandlers.end (); ++it)
			if ((*it)->handler == handler)
			{
				runLoop->unregisterEventHandler (*it);
				eventHandlers.erase (it);
				return true;
			}
		return false;
	}
	bool registerTimer (uint64_t interval, VSTGUI::ITimerHandler* handler) final
	{
		if (!runLoop)
			return false;
		auto h = Steinberg::owned (new TimerHandler ());
		h->handler = handler;
		if (runLoop->registerTimer (h, interval) != kResultTrue)
			return false;
		timerHandlers.push_back (h);
		return true;
	}
	bool unregisterTimer (VSTGUI::ITimerHandler* handler) final
	{
		if (!runLoop)
			return false;
		for (auto it = timerHandlers.begin (); it != timerHandlers.end (); ++it)
			if ((*it)->handler == handler)
			{
				runLoop->unregisterTimer (*it);
				timerHandlers.erase (it);
				return true;
			}
		return false;
	}

private:
	struct EventHandler : Linux::IEventHandler, public FObject
	{
		VSTGUI::IEventHandler* handler {nullptr};
		void PLUGIN_API onFDIsSet (Linux::FileDescriptor) override
		{
			if (handler)
				handler->onEvent ();
		}
		DELEGATE_REFCOUNT (FObject)
		DEFINE_INTERFACES
			DEF_INTERFACE (Linux::IEventHandler)
		END_DEFINE_INTERFACES (FObject)
	};
	struct TimerHandler : Linux::ITimerHandler, public FObject
	{
		VSTGUI::ITimerHandler* handler {nullptr};
		void PLUGIN_API onTimer () final
		{
			if (handler)
				handler->onTimer ();
		}
		DELEGATE_REFCOUNT (FObject)
		DEFINE_INTERFACES
			DEF_INTERFACE (Linux::ITimerHandler)
		END_DEFINE_INTERFACES (FObject)
	};

	std::vector<IPtr<EventHandler>> eventHandlers;
	std::vector<IPtr<TimerHandler>> timerHandlers;
	FUnknownPtr<Linux::IRunLoop> runLoop;
};
#endif

// Text field that removes itself once it loses focus (Enter, Escape or click outside).
class EntryField : public CTextEdit
{
public:
	EntryField (const CRect& r, IControlListener* l, std::function<void ()> onDone)
	: CTextEdit (r, l, kTextEntryTag), done (std::move (onDone))
	{
	}

	void looseFocus () override
	{
		CTextEdit::looseFocus ();
		if (done)
		{
			auto cb = std::move (done);
			done = nullptr;
			cb ();
		}
	}

private:
	std::function<void ()> done;
};

} // namespace

EditorBase::EditorBase (ControllerBase* controller, int width, int height)
: VSTGUIEditor (controller), baseWidth (width), baseHeight (height)
{
	ViewRect r (0, 0, width, height);
	setRect (r);
}

EditorBase::~EditorBase () = default;

ControllerBase* EditorBase::ctrl () const { return static_cast<ControllerBase*> (getController ()); }

tresult PLUGIN_API EditorBase::isPlatformTypeSupported (FIDString type)
{
#if LINUX
	// X11 embedding only: Wayland embedding needs host interfaces VSTGUI's
	// plain editor doesn't wire up, and Linux hosts fall back to X11.
	return std::strcmp (type, kPlatformTypeX11EmbedWindowID) == 0 ? kResultTrue : kInvalidArgument;
#else
	return VSTGUIEditor::isPlatformTypeSupported (type);
#endif
}

bool PLUGIN_API EditorBase::open (void* parent, const PlatformType& platformType)
{
	if (frame)
		return false;

	frame = new CFrame (CRect (0, 0, baseWidth, baseHeight), this);
	frame->setBackgroundColor (colors::background);
	frame->enableTooltips (true, 700); // usage hints live in tooltips, not on the panel
	build (frame);
#if LINUX
	X11::FrameConfig config;
	config.runLoop = VSTGUI::owned (new HostRunLoop (plugFrame));
	frame->open (parent, platformType, &config);
#else
	frame->open (parent, platformType);
#endif
	if (scale != 1.0)
		frame->setZoom (scale);

	syncControls ();
	timer = makeOwned<CVSTGUITimer> (
	    [this] (CVSTGUITimer*) {
		    syncControls ();
		    if (presetBar)
			    presetBar->refresh ();
		    tick ();
	    },
	    33, true);
	return true;
}

void PLUGIN_API EditorBase::close ()
{
	if (timer)
	{
		timer->stop ();
		timer = nullptr;
	}
	if (frame)
	{
		teardown ();
		bound.clear ();
		presetBar = nullptr;
		textEntry = nullptr;
		frame->close ();
		frame = nullptr;
	}
}

tresult PLUGIN_API EditorBase::onSize (ViewRect* newSize)
{
	// Fixed layout; size changes only come from content scaling.
	if (newSize)
		rect = *newSize;
	return kResultTrue;
}

tresult PLUGIN_API EditorBase::setContentScaleFactor (ScaleFactor factor)
{
	if (factor <= 0.f || std::abs (factor - scale) < 0.001)
		return kResultTrue;
	scale = factor;
	ViewRect r (0, 0, static_cast<int32> (std::round (baseWidth * scale)),
	            static_cast<int32> (std::round (baseHeight * scale)));
	rect = r;
	if (frame)
	{
		frame->setZoom (scale);
		if (plugFrame)
			plugFrame->resizeView (this, &r);
	}
	return kResultTrue;
}

void EditorBase::beginEdit (VSTGUI_INT32 index)
{
	if (index >= 0 && ctrl ()->spec (static_cast<uint32_t> (index)))
		VSTGUIEditor::beginEdit (index);
}

void EditorBase::endEdit (VSTGUI_INT32 index)
{
	if (index >= 0 && ctrl ()->spec (static_cast<uint32_t> (index)))
		VSTGUIEditor::endEdit (index);
}

void EditorBase::bindControl (CControl* control, int32_t tag)
{
	control->setTag (tag);
	control->setListener (this);
	if (auto s = ctrl ()->spec (static_cast<uint32_t> (tag)))
	{
		control->setDefaultValue (static_cast<float> (s->defaultNormalized ()));
		control->setValueNormalized (static_cast<float> (ctrl ()->getParamNormalized (tag)));
	}
	if (std::find (bound.begin (), bound.end (), control) == bound.end ())
		bound.push_back (control);
	if (dynamic_cast<Knob*> (control))
		control->setTooltipText ("Drag to adjust (Shift: fine)\nDouble-click: default\nRight-click: type a value");
	control->invalid ();
}

void EditorBase::addPresetBar (const CRect& r)
{
	if (frame && !ctrl ()->presets ().empty ())
	{
		presetBar = new PresetBar (r, ctrl ());
		frame->addView (presetBar);
	}
}

void EditorBase::syncControls ()
{
	for (auto c : bound)
	{
		if (c->isEditing () || c->getTag () < 0)
			continue;
		auto v = static_cast<float> (ctrl ()->getParamNormalized (c->getTag ()));
		if (std::abs (v - c->getValueNormalized ()) > 1e-6f)
		{
			c->setValueNormalized (v);
			c->invalid ();
		}
	}
}

void EditorBase::valueChanged (CControl* control)
{
	if (control->getTag () == kTextEntryTag)
		return; // handled when the field closes
	const auto tag = control->getTag ();
	if (tag < 0 || !ctrl ()->spec (static_cast<uint32_t> (tag)))
		return;
	const double v = control->getValueNormalized ();
	ctrl ()->setParamNormalized (tag, v);
	ctrl ()->performEdit (tag, v);
}

std::string EditorBase::formatValue (int32_t tag, double normalized)
{
	auto s = ctrl ()->spec (static_cast<uint32_t> (tag));
	return s ? s->format (s->toPlain (normalized)) : std::string {};
}

std::string EditorBase::paramName (int32_t tag)
{
	auto s = ctrl ()->spec (static_cast<uint32_t> (tag));
	return s ? s->shortName : std::string {};
}

int32_t EditorBase::stepCount (int32_t tag)
{
	auto s = ctrl ()->spec (static_cast<uint32_t> (tag));
	return s ? s->stepCount () : 0;
}

std::vector<std::string> EditorBase::choiceLabels (int32_t tag)
{
	auto s = ctrl ()->spec (static_cast<uint32_t> (tag));
	if (!s)
		return {};
	if (!s->choices.empty ())
		return s->choices;
	if (s->taper == Taper::Toggle)
		return {"Off", "On"};
	return {};
}

void EditorBase::openTextEntry (CControl* control)
{
	if (!frame || textEntry)
		return;
	const int32_t tag = control->getTag ();
	if (!ctrl ()->spec (static_cast<uint32_t> (tag)))
		return;

	CRect r = control->getViewSize ();
	const CCoord h = 22;
	CRect field (r.left - 6, r.bottom - h, r.right + 6, r.bottom);

	textEntryTag = tag;
	auto entry = new EntryField (field, this, [this] () { finishTextEntry (true); });
	entry->setFont (font (12));
	entry->setFontColor (colors::text);
	entry->setBackColor (colors::panelRaised);
	entry->setFrameColor (colors::accent);
	entry->setStyle (CParamDisplay::kRoundRectStyle);
	entry->setRoundRectRadius (4);
	entry->setHoriAlign (kCenterText);
	entry->setText (formatValue (tag, ctrl ()->getParamNormalized (tag)).c_str ());
	textEntry = entry;
	frame->addView (entry);
	frame->setFocusView (entry);
}

void EditorBase::finishTextEntry (bool commit)
{
	if (!textEntry || !frame)
		return;
	auto entry = textEntry;
	textEntry = nullptr;

	if (commit)
	{
		if (auto s = ctrl ()->spec (static_cast<uint32_t> (textEntryTag)))
		{
			double plain = 0.0;
			if (s->parse (entry->getText ().getString (), plain))
				ctrl ()->editPlain (static_cast<uint32_t> (textEntryTag), plain);
		}
	}
	textEntryTag = -1;
	auto f = frame;
	f->doAfterEventProcessing ([f, entry] () { f->removeView (entry); });
}

} // namespace suite::ui
