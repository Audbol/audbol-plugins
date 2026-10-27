#include "EqEditor.h"

#include "EqGraph.h"

#include "suite/Brand.h"

#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cframe.h"

#include <string>

using namespace VSTGUI;
using namespace suite::ui;
using suite::dsp::FilterType;

namespace eq {

namespace {
constexpr int kWidth = 1000;
constexpr int kHeight = 600;
} // namespace

// Small numbered button for choosing which band the bottom panel edits.
// Click selects, double-click toggles the band on/off.
class BandTab : public CView
{
public:
	BandTab (const CRect& r, int band, EqEditor* editor) : CView (r), band (band), editor (editor) {}

	void setState (bool on, bool sel)
	{
		if (on != enabled || sel != selected)
		{
			enabled = on;
			selected = sel;
			invalid ();
		}
	}

	void draw (CDrawContext* ctx) override
	{
		ctx->setDrawMode (kAntiAliasing | kNonIntegralMode);
		const CRect r = getViewSize ();
		const CColor color = bandColor (band);
		fillRoundRect (ctx, r, 5, enabled ? withAlpha (color, selected ? 255 : 150) : colors::control);
		if (selected)
			strokeRoundRect (ctx, r, 5, enabled ? colors::text : color, 1.5);
		drawText (ctx, std::to_string (band + 1), r, 11,
		          enabled ? colors::background : (selected ? color : colors::textDim), 1, true);
		setDirty (false);
	}

	CMouseEventResult onMouseDown (CPoint&, const CButtonState& buttons) override
	{
		if (!buttons.isLeftButton ())
			return kMouseEventNotHandled;
		editor->selectBand (band);
		if (buttons.isDoubleClick ())
		{
			auto id = bandParam (band, kBandOn);
			editor->eqCtrl ()->editPlain (id, editor->eqCtrl ()->getPlain (id) > 0.5 ? 0.0 : 1.0);
		}
		return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
	}

private:
	int band;
	EqEditor* editor;
	bool enabled {false};
	bool selected {false};
};

EqEditor::EqEditor (EqController* controller) : EditorBase (controller, kWidth, kHeight) {}

void EqEditor::build (CFrame* f)
{
	selected = static_cast<int> (ctrl ()->uiValue ("selectedBand", 0));

	// Header
	add (new HeaderBar (CRect (0, 0, kWidth, 44), SUITE_NAME, "EQ"));
	addPresetBar (CRect (170, 10, 420, 34));
	add (new Label (CRect (440, 12, 516, 32), "Analyzer", 11, colors::textDim, 2));
	bind (add (new Segmented (CRect (522, 11, 722, 33), this, kAnalyzerMode, this)), kAnalyzerMode);
	add (new Label (CRect (734, 12, 774, 32), "Range", 11, colors::textDim, 2));
	bind (add (new Segmented (CRect (780, 11, 900, 33), this, kDisplayRange, this)), kDisplayRange);
	bind (add (new Toggle (CRect (912, 11, 986, 33), this, kBypass, "BYPASS")), kBypass)->setAccent (colors::warning);

	// Graph
	graph = add (new EqGraph (CRect (12, 54, kWidth - 12, 420), eqCtrl ()));
	graph->onSelect = [this] (int b) { selectBand (b); };

	// Bottom panel
	add (new Panel (CRect (12, 430, 640, 588), colors::panel, 8));
	add (new Panel (CRect (650, 430, kWidth - 12, 588), colors::panel, 8, "OUTPUT"));

	for (int b = 0; b < kNumBands; ++b)
	{
		const CCoord x = 24 + b * 38;
		tabs[b] = add (new BandTab (CRect (x, 442, x + 34, 466), b, this));
	}
	bandTitle = add (new Label (CRect (24, 476, 324, 492), "", 11, colors::textDim, 0, true));

	bandOn = add (new Toggle (CRect (24, 500, 84, 524), this, -1, "ON"));
	bandType = add (new ChoiceMenu (CRect (92, 500, 324, 524), this, -1, this));
	bandSlope = add (new ChoiceMenu (CRect (24, 534, 170, 558), this, -1, this));
	bandPlacement = add (new ChoiceMenu (CRect (178, 534, 324, 558), this, -1, this));

	bandFreq = add (new Knob (CRect (346, 448, 436, 548), this, -1, this));
	bandGain = add (new Knob (CRect (442, 448, 532, 548), this, -1, this));
	bandGain->setBipolar (true);
	bandQ = add (new Knob (CRect (538, 448, 628, 548), this, -1, this));

	auto scale = bind (add (new Knob (CRect (720, 456, 810, 556), this, kGainScale, this)), kGainScale);
	(void)scale;
	auto out = bind (add (new Knob (CRect (840, 456, 930, 556), this, kOutputGain, this)), kOutputGain);
	out->setBipolar (true);

	add (new Label (CRect (24, 566, 630, 582),
	                "Double-click graph to add  |  drag to move  |  wheel for Q  |  right-click for options",
	                10, colors::textFaint, 0));

	selectBand (selected);
}

void EqEditor::teardown ()
{
	graph = nullptr;
	for (auto& t : tabs)
		t = nullptr;
	bandOn = nullptr;
	bandType = bandSlope = bandPlacement = nullptr;
	bandFreq = bandGain = bandQ = nullptr;
	bandTitle = nullptr;
}

void EqEditor::selectBand (int band)
{
	selected = std::clamp (band, 0, kNumBands - 1);
	ctrl ()->setUiValue ("selectedBand", selected);
	if (!graph)
		return;
	graph->setSelected (selected);

	const CColor color = bandColor (selected);
	bind (bandOn, bandParam (selected, kBandOn))->setAccent (color);
	bind (bandType, bandParam (selected, kBandType));
	bind (bandSlope, bandParam (selected, kBandSlope));
	bind (bandPlacement, bandParam (selected, kBandPlacement));
	bind (bandFreq, bandParam (selected, kBandFreq))->setAccent (color);
	bind (bandGain, bandParam (selected, kBandGain))->setAccent (color);
	bind (bandQ, bandParam (selected, kBandQ))->setAccent (color);
	bandTitle->setText ("BAND " + std::to_string (selected + 1));
	updateBandControls ();
}

void EqEditor::updateBandControls ()
{
	if (!graph)
		return;
	for (int b = 0; b < kNumBands; ++b)
		tabs[b]->setState (ctrl ()->getPlain (bandParam (b, kBandOn)) > 0.5, b == selected);

	const auto type = static_cast<FilterType> (static_cast<int> (ctrl ()->getPlain (bandParam (selected, kBandType))));
	const bool hasGain = suite::dsp::filterTypeHasGain (type);
	const bool hasSlope = suite::dsp::filterTypeHasSlope (type);
	if (bandGain->getMouseEnabled () != hasGain)
	{
		bandGain->setMouseEnabled (hasGain);
		bandGain->setAlphaValue (hasGain ? 1.0f : 0.35f);
	}
	if (bandSlope->getMouseEnabled () != hasSlope)
	{
		bandSlope->setMouseEnabled (hasSlope);
		bandSlope->setAlphaValue (hasSlope ? 1.0f : 0.35f);
	}
}

void EqEditor::tick ()
{
	if (!graph)
		return;
	graph->refresh (0.033f);
	if (graph->getSelected () != selected)
		selectBand (graph->getSelected ());
	updateBandControls ();
}

} // namespace eq
