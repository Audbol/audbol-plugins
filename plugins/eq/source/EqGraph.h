#pragma once

#include "EqController.h"
#include "EqParams.h"

#include "suite/dsp/Svf.h"

#include "vstgui/lib/cview.h"

#include <functional>
#include <vector>

namespace eq {

// Interactive response display: frequency grid, live analyzer, per-band and
// summed curves, and draggable band nodes.
//
//  drag node        frequency + gain (cut filters: frequency + resonance)
//  Shift-drag       fine adjustment
//  wheel on node    Q / bandwidth
//  double-click     empty area adds a band, a node removes it
//  right-click      node menu (type, slope, stereo placement, remove)
class EqGraph : public VSTGUI::CView
{
public:
	EqGraph (const VSTGUI::CRect& r, EqController* controller);

	void setSelected (int band);
	int getSelected () const { return selected; }
	std::function<void (int)> onSelect;

	// Called by the editor on its refresh timer.
	void refresh (float dtSeconds);

	void draw (VSTGUI::CDrawContext* ctx) override;
	void setViewSize (const VSTGUI::CRect& rect, bool invalid = true) override;
	VSTGUI::CMouseEventResult onMouseDown (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseMoved (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseUp (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseCancel () override;
	VSTGUI::CMouseEventResult onMouseExited (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	void onMouseWheelEvent (VSTGUI::MouseWheelEvent& event) override;

private:
	struct BandView
	{
		bool on {false};
		suite::dsp::FilterType type {suite::dsp::FilterType::Bell};
		double freq {1000}, gain {0}, q {1};
		int slope {3};
		suite::dsp::BandDesign design;
	};

	void layout ();
	bool readBands ();
	void rebuildCurves ();

	double freqToX (double f) const;
	double xToFreq (double x) const;
	double dbToY (double db) const;
	double yToDb (double y) const;
	double analyzerDbToY (double db) const;
	VSTGUI::CPoint nodePos (int band) const;
	int hitNode (const VSTGUI::CPoint& p) const;

	void drawGrid (VSTGUI::CDrawContext* ctx);
	void drawAnalyzer (VSTGUI::CDrawContext* ctx, const std::vector<float>& db, VSTGUI::CColor fill,
	                   VSTGUI::CColor line);
	void drawCurves (VSTGUI::CDrawContext* ctx);
	void drawNodes (VSTGUI::CDrawContext* ctx);
	void drawTooltip (VSTGUI::CDrawContext* ctx, int band);

	void perform (uint32_t id, double plain);
	void addBandAt (const VSTGUI::CPoint& p);
	void showMenu (int band, const VSTGUI::CPoint& where);
	void endDrag ();

	EqController* ctrl;
	VSTGUI::CRect plot;
	BandView bands[kNumBands];
	std::vector<double> bandDb[kNumBands];
	std::vector<double> sumDb;
	std::vector<float> columnFreqs;
	std::vector<float> preDb, postDb;
	std::vector<double> lastSnapshot;
	double rangeDb {24.0};
	double curveSampleRate {0.0};
	AnalyzerMode analyzer {AnalyzerMode::Post};

	int selected {0};
	int hover {-1};
	int dragging {-1};
	VSTGUI::CPoint lastMouse;
	double dragLogFreq {0}, dragGain {0}, dragLogQ {0};
};

} // namespace eq
