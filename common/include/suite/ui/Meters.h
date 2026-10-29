#pragma once

#include "suite/ui/Theme.h"

#include "vstgui/lib/cview.h"

#include <string>
#include <vector>

namespace suite::ui {

// Vertical bar meter with peak hold. Level mode fills upward from the floor
// (dBFS); Reduction mode fills downward from the top (dB of gain reduction).
// Click resets the peak hold.
class LevelMeter : public VSTGUI::CView
{
public:
	enum class Mode
	{
		Level,
		Reduction
	};

	LevelMeter (const VSTGUI::CRect& r, std::string label, Mode mode, double rangeDb);

	// Feed the newest value (or the floor when no audio arrived) once per UI tick.
	void update (float valueDb, float dtSeconds);
	float displayed () const { return display; }

	void draw (VSTGUI::CDrawContext* ctx) override;
	VSTGUI::CMouseEventResult onMouseDown (VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;

private:
	double toFraction (double db) const; // 0 = empty, 1 = full

	std::string label;
	Mode mode;
	double range;
	float display;
	float peak;
	float peakAge {0};
};

// Scrolling history of input level, output level and gain reduction, with a
// threshold marker. Newest data appears on the right.
class DynamicsHistory : public VSTGUI::CView
{
public:
	DynamicsHistory (const VSTGUI::CRect& r, double rangeDb = 60.0);

	void push (float inDb, float outDb, float grDb);
	void setThreshold (double db);
	void setThresholdLabel (std::string label) { thresholdLabel = std::move (label); }

	void draw (VSTGUI::CDrawContext* ctx) override;
	void setViewSize (const VSTGUI::CRect& rect, bool invalid = true) override;
	bool hitTest (const VSTGUI::CPoint&, const VSTGUI::Event&) override { return false; }

private:
	struct Column
	{
		float in, out, gr;
	};

	VSTGUI::CRect plotRect () const;
	double levelToY (const VSTGUI::CRect& plot, double db) const;

	double range;
	double threshold {-18.0};
	std::string thresholdLabel {"Threshold"};
	std::vector<Column> columns; // ring buffer
	size_t head {0};
};

// The IN / GR / OUT meters plus history that dynamics plug-ins share.
// Call update() once per UI tick with the peaks gathered since the last one.
struct DynamicsDisplays
{
	DynamicsHistory* history {nullptr};
	LevelMeter* in {nullptr};
	LevelMeter* gr {nullptr};
	LevelMeter* out {nullptr};

	void update (float inPeak, float outPeak, float grDb, bool fresh, float dtSeconds);
	void clear () { *this = {}; }
};

} // namespace suite::ui
